// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ti.h"
#include <unistd.h>
#include <sys/ioctl.h>

//----------------------------------------------------------------------

enum { TERMINFO_MAGIC = 0432 };

/// The header of the terminfo file
struct STerminfoHeader {
    uint16_t	magic;		///< Equal to TERMINFO_MAGIC constant above.
    uint16_t	namesSize;
    uint16_t	nBooleans;
    uint16_t	nNumbers;
    uint16_t	nStrings;
    uint16_t	strtableSize;
};

INTEGRAL_STREAMABLE(STerminfoHeader);

namespace utio {

//----------------------------------------------------------------------

const char CTerminfo::no_value[1] = "";

/// Default constructor.
CTerminfo::CTerminfo (void)
:_name()
,_booleans()
,_numbers()
,_stringOffsets()
,_stringTable()
,_acsMap()
,_ctx()
,_nColors (16)
,_nPairs (64)
,_nColumns (80)
,_nRows (24)
{
}

/// Default constructor.
CTerminfo::CContext::CContext (void)
: output()
, progStack()
, pos (-1, -1)
, attrs (0)
, fg (lightgray)
, bg (black)
{
}

//----------------------------------------------------------------------
// Terminfo loading
//----------------------------------------------------------------------

/// Loads terminfo entry \p termname into \p buf
void CTerminfo::LoadEntry (memblock& buf, const char* termname) const
{
    string tipath;
    if (!termname)
	termname = "linux";
    const char* defTiPath = getenv ("TERMINFO");
    if (!defTiPath)
	defTiPath = "/usr/share/terminfo";
    if (access (defTiPath, R_OK | X_OK))
	throw runtime_error ("could not find the terminfo database; please set $TERMINFO environment variable to point to it");
    tipath.format ("%s/%c/%s", defTiPath, termname[0], termname);
    if (access (tipath.c_str(), R_OK))
	throw runtime_error ("could not find the terminfo description for your terminal; please update your terminfo database");
    buf.read_file (tipath.c_str());
}

/// Reads the terminfo entry from stream \p is.
void CTerminfo::read (istream& is)
{
    // First the header.
    STerminfoHeader h;
#if WANT_STREAM_BOUNDS_CHECKING
    memset (&h, 0, sizeof(h));
#else
    is.verify_remaining ("read","terminfo",stream_size_of(h));
#endif
    is >> h;
#if BYTE_ORDER == BIG_ENDIAN
    h.magic = le_to_native (h.magic);
    h.namesSize = le_to_native (h.namesSize);
    h.nBooleans = le_to_native (h.nBooleans);
    h.nNumbers = le_to_native (h.nNumbers);
    h.nStrings = le_to_native (h.nStrings);
    h.strtableSize = le_to_native (h.strtableSize);
#endif
    if (h.magic != TERMINFO_MAGIC)
	throw domain_error ("corrupt terminfo file");

    // The names section
    is.read_strz (_name);

    // The boolean section
    is.verify_remaining ("read","terminfo",sizeof(boolvec_t::size_type)+h.nBooleans*sizeof(boolvec_t::value_type));
    _booleans.resize (h.nBooleans);
    nr_container_read (is, _booleans);
    is >> ios::talign<number_t>();

    // The numbers section
    is.verify_remaining ("read","terminfo",sizeof(numvec_t::size_type)+h.nNumbers*sizeof(numvec_t::value_type));
    _numbers.resize (h.nNumbers);
    nr_container_read (is, _numbers);

    // The string offsets section
    is.verify_remaining ("read","terminfo",sizeof(stroffvec_t::size_type)+h.nStrings*sizeof(stroffvec_t::value_type));
    _stringOffsets.resize (h.nStrings);
    nr_container_read (is, _stringOffsets);

    // The stringtable
    is.verify_remaining ("read","terminfo",h.strtableSize);
    _stringTable.resize (h.strtableSize);
    is.read (_stringTable.begin(), _stringTable.size());

#if BYTE_ORDER == BIG_ENDIAN
    foreach (numvec_t::iterator, i, _numbers) *i = le_to_native (*i);
    foreach (stroffvec_t::iterator, i, _stringOffsets) *i = le_to_native (*i);
#endif
}

/// Writes a terminfo entry into stream \p os
void CTerminfo::write (ostream& os) const
{
    STerminfoHeader h;
    h.magic = native_to_le<uint16_t> (TERMINFO_MAGIC);
    h.namesSize = native_to_le (_name.size() + 1);
    h.nBooleans = native_to_le (_booleans.size());
    h.nNumbers = native_to_le (_numbers.size());
    h.nStrings = native_to_le (_stringOffsets.size());
    h.strtableSize = native_to_le (_stringTable.size());
    os << h;
    os.write_strz (_name);
    nr_container_write (os, _booleans);
    os << ios::talign<number_t>();
    foreach (numvec_t::const_iterator, i, _numbers) os << native_to_le (*i);
    foreach (stroffvec_t::const_iterator, i, _stringOffsets) os << native_to_le (*i);
    os.write (_stringTable.begin(), _stringTable.size());
}

/// Returns the written size of the terminfo entry.
size_t CTerminfo::stream_size (void) const
{
    return (stream_size_of(STerminfoHeader()) +
	    Align (_name.size() + _booleans.size(), __alignof__(number_t)) +
	    _numbers.size() * sizeof(number_t) +
	    _stringOffsets.size() * sizeof(stroffset_t) +
	    _stringTable.size());
}

/// Loads the terminfo entry \p termname.
void CTerminfo::Load (const char* termname)
{
    if (!termname || !*termname)
	termname = getenv ("TERM");
    if (!termname || !*termname)
	termname = "linux";
    string tibuf;
    LoadEntry (tibuf, termname);
    istream is (tibuf);
    read (is);
    CacheFrequentValues();
    ObtainTerminalParameters();
}

/// Caches frequently used, but badly formatted caps.
void CTerminfo::CacheFrequentValues (void)
{
    // Color stuff.
    _nColors = GetNumber (ti::max_colors);
    _nPairs = GetNumber (ti::max_pairs);
    if (_nColors == uint16_t(ti::no_value))
	_nColors = color_Last;
    if (_nPairs == uint16_t(ti::no_value))
	_nPairs = 64;

    // Decode the ACS capability, if present.
    const string acsString (GetString (ti::acs_chars));
    for (uoff_t i = 0; i < acs_Last; ++ i)
	_acsMap[i] = c_AcscInfo[i].m_Default;
    if (!acsString.empty()) {
	const SAcscInfo* cFirst = c_AcscInfo;
	const SAcscInfo* cLast = cFirst + acs_Last;
	for (string::const_iterator i = acsString.begin(); i < acsString.end(); i += 2)
	    for (const SAcscInfo* cFound = cFirst; cFound < cLast; ++ cFound)
		if (cFound->m_vt100Code == *i)
		    _acsMap [distance (cFirst, cFound)] = *(i + 1);
    }
}

/// Queries the terminal parameters (such as the screen size)
void CTerminfo::ObtainTerminalParameters (void)
{
    _nRows = _nColumns = 0;
    // The environment variables seem to be the ones that lie least often.
    const char* sp;
    if ((sp = getenv("LINES")))
	_nRows = atoi(sp);
    if ((sp = getenv("COLUMNS")))
	_nColumns = atoi(sp);
    // Next, try asking the VT
    if (!_nRows || !_nColumns) {
	struct winsize ws;
	if (!ioctl (STDIN_FILENO, TIOCGWINSZ, &ws)) {
	    _nColumns = ws.ws_col;
	    _nRows = ws.ws_row;
	}
    }
    // Try to fallback to the terminfo entries, or to 80x24.
    if (!_nRows || !_nColumns) {
	if ((_nRows = GetNumber (ti::lines)) == dim_t(ti::no_value))
	    _nRows = 24;
	if ((_nColumns = GetNumber (ti::columns)) == dim_t(ti::no_value))
	    _nColumns = 80;
    }
}

//----------------------------------------------------------------------
// Terminfo values access interface
//----------------------------------------------------------------------

/// Gets boolean value \p i.
bool CTerminfo::GetBool (ti::EBooleans i) const
{
    return (size_t(i) < _booleans.size() ? (_booleans[i] > 0) : false);
}

/// Gets numeral value \p i.
CTerminfo::number_t CTerminfo::GetNumber (ti::ENumbers i) const
{
    return (size_t(i) < _numbers.size() ? _numbers[i] : number_t(ti::no_value));
}

/// Gets string value \p i.
CTerminfo::capout_t CTerminfo::GetString (ti::EStrings i) const
{
    if (size_t(i) >= _stringOffsets.size() || _stringOffsets[i] == stroffset_t(ti::no_value))
	return (no_value);
    return (_stringTable.begin() + _stringOffsets[i]);
}

/// Pops a value from the program stack.
CTerminfo::progvalue_t CTerminfo::PSPop (void) const
{
    if (_ctx.progStack.empty())
	return (0);
    const progvalue_t v = _ctx.progStack.back();
    _ctx.progStack.pop_back();
    return (v);
}

/// Pushes \p v onto the program stack.
void CTerminfo::PSPush (progvalue_t v) const
{
    _ctx.progStack.push_back (v);
}

/// Runs the % opcodes in \p program and appends to \p result.
void CTerminfo::RunStringProgram (const char* program, string& result, progargs_t args) const
{
    bool bCondValue = false;
    const string prgstr (program);
    foreach (string::const_iterator, i, prgstr) {
	if (*i != '%') {			// Output normal data
	    result += *i;
	    continue;
	}
	int width = 0, base = 0;
	switch (*++i) {	// beware the excessive use of fallthrough :)
	    case '%': result += *i;		break;	// %% outputs %.
	    case 'i': ++args[0]; ++args[1];	break;	// %i adds 1 to i two arguments.
	    case 'c': result += char(PSPop());	break;
	    case 'x': base = 16; continue;
	    case '0': if (!base) base = 8;
	    case '1': case '2': case '3': case '4':	// part of %d width field
	    case '5': case '6': case '7': case '8':
	    case '9': if (!base) base = 10;
		      width = width * base + (*i - '0');
		      continue;
	    case '\\': base = 0;
	    case '{': continue;				// %{number}
	    case '\'': if (*(i - 1) == '%') {		// %'A' or %'\017'
		          if (*(i + 1) != '\\')
		 	      width = *++i;
		          continue;
		      }
 	    case '}': PSPush (width);			break;
	    // Binary operands are in infix (reversed) order
	    case '+': PSPush (PSPop() + PSPop());	break;
	    case '-': PSPush (-PSPop() + PSPop());	break;
	    case '*': PSPush (PSPop() * PSPop());	break;
	    case '/': PSPush (PSPop() / PSPopNonzero());break;
	    case 'm': PSPush (PSPop() % PSPopNonzero());break;
	    case '|': PSPush (PSPop() | PSPop());	break;
	    case '&': PSPush (PSPop() & PSPop());	break;
	    case '^': PSPush (PSPop() ^ PSPop());	break;
	    case '>': PSPush (PSPop() < PSPop());	break;
	    case '<': PSPush (PSPop() > PSPop());	break;
	    case '=': PSPush (PSPop() == PSPop());	break;
	    case 'A': PSPush (PSPop() && PSPop());	break;
	    case 'O': PSPush (PSPop() || PSPop());	break;
	    case '!': PSPush (!PSPop());		break;
	    case '~': PSPush (~PSPop());		break;
	    case 't': bCondValue = PSPop();
	    case 'e': if ((bCondValue = !bCondValue)) { // this also supports elsif
			  uoff_t elseLoc = prgstr.find ("%e", i - prgstr.begin());
			  uoff_t endLoc = prgstr.find ("%;", i - prgstr.begin());
			  --(i = prgstr.iat (min (elseLoc, endLoc)));
		      }
	    case '?':
	    case ';': break;
	    case 'p': PSPush (args [min (uoff_t(*++i - '1'), args.size() - 1)]); break; // %p[0-9] pushes numbered parameter.
	    case 'd': {		// %d prints the top of the stack and pops the stack.
		progvalue_t n = PSPop();
		const size_t iSize = result.size();
		do {
		    result += string::value_type('0' + (n % 10));
		} while ((n /= 10) || --width > 0);
		reverse (result.begin() + iSize, result.end());
		break; }
	};
    }
}

/// Replaces \p c with a terminal-specific accelerated value, if available.
wchar_t CTerminfo::SubstituteChar (wchar_t c) const
{
    for (uoff_t i = 0; i < acs_Last; ++ i)
	if (c_AcscInfo[i].m_Unicode == c)
	    return (_acsMap[i]);
    return (c);
}

/// Loads terminal strings produced by special keys into \p ksv.
void CTerminfo::LoadKeystrings (keystrings_t& ksv) const
{
    ksv.clear();
    for (uoff_t i = 0; i < VectorSize(c_KeyToStringMap); ++i) {
	const char* ksvp = GetString (ti::EStrings (c_KeyToStringMap [i]));
	if (!*ksvp && i == kv_Esc - kv_First)
	    ksvp = "\x1B";
	ksv += ksvp;
	ksv += '\0';
    }
}

//----------------------------------------------------------------------
// Screen output interface
//----------------------------------------------------------------------

/// Clears the screen.
CTerminfo::capout_t CTerminfo::Clear (void) const
{
    _ctx.pos[0] = 0;
    _ctx.pos[1] = 0;
    return (GetString (ti::clear_screen));
}

/// Resets the saved terminal state without doing anything to the terminal.
void CTerminfo::ResetState (void) const
{
    _ctx.attrs = 0;
    _ctx.fg = lightgray;
    _ctx.bg = black;
    _ctx.pos[0] = 0;
    _ctx.pos[1] = 0;
}

/// Resets the terminal to a sane state.
CTerminfo::capout_t CTerminfo::Reset (void) const
{
    ResetState();
    return (GetString (ti::reset_1string));
}

/// Stops all attributes, including color.
CTerminfo::capout_t CTerminfo::AllAttrsOff (void) const
{
    _ctx.attrs = 0;
    _ctx.fg = lightgray;
    _ctx.bg = black;
    return (GetString (ti::exit_attribute_mode));
}

/// Updates cached screen information.
void CTerminfo::Update (void)
{
    ObtainTerminalParameters();
}

/// Appends move(x,y) string to s.
void CTerminfo::MoveTo (coord_t x, coord_t y, rstrbuf_t s) const
{
    RunStringProgram (GetString (ti::cursor_address), s, progargs_t(y, x));
    _ctx.pos[0] = x;
    _ctx.pos[1] = y;
}

/// Moves the cursor to \p x, \p y.
CTerminfo::strout_t CTerminfo::MoveTo (coord_t x, coord_t y) const
{
    _ctx.output.clear();
    MoveTo (x, y, _ctx.output);
    return (_ctx.output);
}

/// Sets the color to \p fg on \p bg.
CTerminfo::strout_t CTerminfo::Color (EColor fg, EColor bg) const
{
    _ctx.output.clear();
    Color (fg, bg, _ctx.output);
    return (_ctx.output);
}

/// Truncates color values to supported range and sets attributes to compensate.
void CTerminfo::NormalizeColor (EColor& fg, EColor& bg, uint16_t& attrs) const
{
    if (fg > color_Last)
	fg = lightgray;
    if (bg > color_Last)
	bg = black;
    if (fg < 8)
	attrs &= ~(1 << a_bold);
    else if (fg >= 8 && fg < color_Last) {
	attrs |= (1 << a_bold);
	fg = EColor(fg - 8);
    }
    if (bg < 8)
	attrs &= ~(1 << a_blink);
    else if (bg >= 8 && bg < color_Last) {
	attrs |= (1 << a_blink);
	bg = EColor(bg - 8);
    }
}

/// Sets the color using normalized values (i.e. no attribute setting)
void CTerminfo::NColor (EColor fg, EColor bg, rstrbuf_t s) const
{
    if (_ctx.fg != fg && fg != color_Preserve)
	RunStringProgram (GetString (ti::set_a_foreground), s, progargs_t(fg));
    if (_ctx.bg != bg && bg != color_Preserve)
	RunStringProgram (GetString (ti::set_a_background), s, progargs_t(bg));
    _ctx.fg = fg;
    _ctx.bg = bg;
}

/// Sets the color to \p fg on \p bg, appending result to \p s.
void CTerminfo::Color (EColor fg, EColor bg, rstrbuf_t s) const
{
    uint16_t newAttrs = _ctx.attrs;
    NormalizeColor (fg, bg, newAttrs);
    if (_ctx.attrs != newAttrs)
	Attrs (newAttrs, s);
    NColor (fg, bg, s);
}

/// Starts attribute \p a.
CTerminfo::capout_t CTerminfo::AttrOn (EAttribute a) const
{
    static const ti::EStrings as [attr_Last] = {
	/* a_standout */	ti::enter_standout_mode,
	/* a_underline */	ti::enter_underline_mode,
	/* a_reverse */		ti::enter_reverse_mode,
	/* a_blink */		ti::enter_blink_mode,
	/* a_halfbright */	ti::enter_dim_mode,
	/* a_bold */		ti::enter_bold_mode,
	/* a_invisible */	ti::enter_secure_mode,
	/* a_protect */		ti::enter_protected_mode,
	/* a_altcharset */	ti::enter_alt_charset_mode,
	/* a_italic */		ti::enter_italics_mode,
	/* a_subscript */	ti::enter_subscript_mode,
	/* a_superscript */	ti::enter_superscript_mode,
    };
    _ctx.attrs |= (1 << a);
    return (a < attr_Last ? GetString (as[a]) : no_value);
}

/// \brief Stops attribute \p a.
/// Note that this is not too reliable and will quite likely
/// turn off all your attributes (including colors) instead.
///
CTerminfo::capout_t CTerminfo::AttrOff (EAttribute a) const
{
    static const ti::EStrings as [attr_Last] = {
	/* a_standout */	ti::exit_standout_mode,
	/* a_underline */	ti::exit_underline_mode,
	/* a_reverse */		ti::exit_attribute_mode,
	/* a_blink */		ti::exit_attribute_mode,
	/* a_halfbright */	ti::exit_attribute_mode,
	/* a_bold */		ti::exit_attribute_mode,
	/* a_invisible */	ti::exit_attribute_mode,
	/* a_protect */		ti::exit_attribute_mode,
	/* a_altcharset */	ti::exit_alt_charset_mode,
	/* a_italic */		ti::exit_italics_mode,
	/* a_subscript */	ti::exit_subscript_mode,
	/* a_superscript */	ti::exit_superscript_mode,
    };
    const uint16_t newAttrs (_ctx.attrs & ~(1 << a));
    if (a >= attr_Last || (as[a] == ti::exit_attribute_mode && GetString(ti::set_attributes) != no_value))
	return (Attrs (newAttrs));
    _ctx.attrs = newAttrs;
    return (GetString (as[a]));
}

/// Same as Attrs, but it appends to output
void CTerminfo::Attrs (uint16_t a, rstrbuf_t s) const
{
    if (_ctx.attrs == a)
	return;
    const capout_t sgr = GetString (ti::set_attributes);
    if (sgr == no_value) {
	size_t nToOff (0), nToOn (0);
	uint16_t mask (1);
	for (uoff_t i = 0; i < attr_Last; ++ i, mask <<= 1) {
	    nToOff += (_ctx.attrs & mask) && !(a & mask);
	    nToOn  += !(_ctx.attrs & mask) && (a & mask);
	}
	const uint16_t oldAttrs (_ctx.attrs);
	if (nToOff) {
	    s += GetString (ti::exit_attribute_mode);
	    s += GetString (ti::exit_alt_charset_mode);
	    _ctx.fg = lightgray;
	    _ctx.bg = black;
	}
	mask = 1;
	for (uoff_t i = 0; i < attr_Last; ++ i, mask <<= 1)
	    if ((a & mask) && (nToOff || !(oldAttrs & mask)))
		s += AttrOn (EAttribute (i));
    } else {
	progargs_t pa;
	for (uoff_t i = 0; i < pa.size(); ++ i)
	    pa[i] = (a >> i) & 1;
	RunStringProgram (sgr, s, pa);
	_ctx.fg = lightgray;
	_ctx.bg = black;
    }
    _ctx.attrs = a;
}

/// Sets all attributes to values in \p a (masked by EAttribute)
CTerminfo::strout_t CTerminfo::Attrs (uint16_t a) const
{
    _ctx.output.clear();
    Attrs (a, _ctx.output);
    return (_ctx.output);
}

/// Draws a box in the given location using ACS characters.
CTerminfo::strout_t CTerminfo::Box (coord_t x, coord_t y, dim_t w, dim_t h) const
{
    _ctx.output = AttrOn (a_altcharset);
    MoveTo (x, y, _ctx.output);

    _ctx.output += AcsChar (acs_UpperLeftCorner);
    fill_n (back_inserter(_ctx.output), w - 2, AcsChar (acs_HLine));
    _ctx.output += AcsChar (acs_UpperRightCorner);

    for (dim_t yi = 1; yi < h - 1; ++ yi) {
	MoveTo (x, y + yi, _ctx.output);
	_ctx.output += AcsChar (acs_VLine);
	MoveTo (x + w - 1, y + yi, _ctx.output);
	_ctx.output += AcsChar (acs_VLine);
    }

    MoveTo (x, y + h - 1, _ctx.output);
    _ctx.output += AcsChar (acs_LowerLeftCorner);
    fill_n (back_inserter(_ctx.output), w - 2, AcsChar (acs_HLine));
    _ctx.output += AcsChar (acs_LowerRightCorner);

    Attrs ((_ctx.attrs & ~(1 << a_altcharset)), _ctx.output);
    return (_ctx.output);
}

/// Draws a rectangle filled with \p c.
CTerminfo::strout_t CTerminfo::Bar (coord_t x, coord_t y, dim_t w, dim_t h, char c) const
{
    _ctx.output = AttrOn (a_altcharset);
    for (dim_t yi = 0; yi < h; ++ yi) {
	MoveTo (x, y + yi, _ctx.output);
	fill_n (back_inserter(_ctx.output), w, c);
    }
    Attrs ((_ctx.attrs & ~(1 << a_altcharset)), _ctx.output);
    return (_ctx.output);
}

/// Draws a horizontal line.
CTerminfo::strout_t CTerminfo::HLine (coord_t x, coord_t y, dim_t w) const
{
    return (Bar (x, y, w, 1, AcsChar(acs_HLine)));
}

/// Draws a vertical line.
CTerminfo::strout_t CTerminfo::VLine (coord_t x, coord_t y, dim_t h) const
{
    return (Bar (x, y, 1, h, AcsChar(acs_VLine)));
}

/// Draws character \p data into the given box. 0-valued characters are transparent.
CTerminfo::strout_t CTerminfo::Image (coord_t x, coord_t y, dim_t w, dim_t h, const CCharCell* data) const
{
    assert (data && "Image should only be called with valid data");
    assert (x >= 0 && y >= 0 && x + w <= Width() && y + h <= Height() && "Clip the image data before passing it in. CGC::Clip can do it.");

    const uint16_t oldAttrs (_ctx.attrs);
    const EColor oldFg (EColor(_ctx.fg)), oldBg (EColor(_ctx.bg));

    _ctx.output = GetString(ti::ena_acs);
    for (coord_t j = y; j < y + h; ++ j) {
	for (coord_t i = x; i < x + w; ++ i, ++ data) {
	    wchar_t dc = data->c;
	    if (!dc)
		continue;
	    if (i != _ctx.pos[0] || j != _ctx.pos[1]) {
		if (i == 0 && j == _ctx.pos[1] + 1) {
		    _ctx.output += '\n';
		    _ctx.pos[0] = 0;
		    ++ _ctx.pos[1];
		} else
		    MoveTo (i, j, _ctx.output);
	    }
	    uint16_t dattr (data->attrs & BitMask(uint16_t,attr_Last));
	    if (dc > CHAR_MAX) {
		dc = SubstituteChar(dc);
		dattr |= (1 << a_altcharset);
	    }
	    if (!(dattr & (1 << a_altcharset)) && !isprint(dc))
		dc = ' ';
	    EColor fg (EColor(data->fg)), bg (EColor(data->bg));
	    NormalizeColor (fg, bg, dattr);
	    Attrs (dattr, _ctx.output);
	    NColor (fg, bg, _ctx.output);
	    _ctx.output += char(dc);
	    ++ _ctx.pos[0];
	}
    }
    Attrs (oldAttrs, _ctx.output);
    NColor (oldFg, oldBg, _ctx.output);
    return (_ctx.output);
}

//----------------------------------------------------------------------

// First two values are from the terminfo manpage.
// The unicode value is from UnicodeData.txt from www.unicode.org
const CTerminfo::SAcscInfo CTerminfo::c_AcscInfo [acs_Last] = {
    { '}', 'f', acsv_PoundSign },
    { '.', 'v', acsv_DownArrow },
    { ',', '<', acsv_LeftArrow },
    { '+', '>', acsv_RightArrow },
    { '-', '^', acsv_UpArrow },
    { 'h', '#', acsv_Board },
    { '~', 'o', acsv_Bullet },
    { 'a', ':', acsv_Checkerboard },
    { 'f','\\', acsv_Degree },
    { '`', '+', acsv_Diamond },
    { 'z', '>', acsv_GreaterEqual },
    { '{', '*', acsv_Pi },
    { 'q', '-', acsv_HLine },
    { 'i', '#', acsv_Lantern },
    { 'n', '+', acsv_Plus },
    { 'y', '<', acsv_LessEqual },
    { 'm', '+', acsv_LowerLeftCorner },
    { 'j', '+', acsv_LowerRightCorner },
    { '|', '!', acsv_NotEqual },
    { 'g', '#', acsv_PlusMinus },
    { 'o', '~', acsv_Scanline1 },
    { 'p', '-', acsv_Scanline3 },
    { 'r', '-', acsv_Scanline7 },
    { 's', '_', acsv_Scanline9 },
    { '0', '#', acsv_Block },
    { 'w', '+', acsv_TopTee },
    { 'u', '+', acsv_RightTee },
    { 't', '+', acsv_LeftTee },
    { 'v', '+', acsv_BottomTee },
    { 'l', '+', acsv_UpperLeftCorner },
    { 'k', '+', acsv_UpperRightCorner },
    { 'x', '|', acsv_VLine }
};

const int16_t CTerminfo::c_KeyToStringMap [kv_nKeys] = {
    /* kv_Esc */		ti::key_command,
    /* kv_Backspace */		ti::key_backspace,
    /* kv_Backtab */		ti::key_btab,
    /* kv_Begin */		ti::key_beg,
    /* kv_CATab */		ti::key_catab,
    /* kv_CTab */		ti::key_ctab,
    /* kv_Cancel */		ti::key_cancel,
    /* kv_Center */		ti::key_b2,
    /* kv_Clear */		ti::key_clear,
    /* kv_ClearToEOL */		ti::key_eol,
    /* kv_ClearToEOS */		ti::key_eos,
    /* kv_Close */		ti::key_close,
    /* kv_Command */		ti::key_command,
    /* kv_Copy */		ti::key_copy,
    /* kv_Create */		ti::key_create,
    /* kv_Delete */		ti::key_dc,
    /* kv_DeleteLine */		ti::key_dl,
    /* kv_Down */		ti::key_down,
    /* kv_DownLeft */		ti::key_c1,
    /* kv_DownRight */		ti::key_c3,
    /* kv_End */		ti::key_end,
    /* kv_Exit */		ti::key_exit,
    /* kv_F0 */			ti::key_f0,
    /* kv_F1 */			ti::key_f1,
    /* kv_F2 */			ti::key_f2,
    /* kv_F3 */			ti::key_f3,
    /* kv_F4 */			ti::key_f4,
    /* kv_F5 */			ti::key_f5,
    /* kv_F6 */			ti::key_f6,
    /* kv_F7 */			ti::key_f7,
    /* kv_F8 */			ti::key_f8,
    /* kv_F9 */			ti::key_f9,
    /* kv_F10 */		ti::key_f10,
    /* kv_F11 */		ti::key_f11,
    /* kv_F12 */		ti::key_f12,
    /* kv_F13 */		ti::key_f13,
    /* kv_F14 */		ti::key_f14,
    /* kv_F15 */		ti::key_f15,
    /* kv_F16 */		ti::key_f16,
    /* kv_F17 */		ti::key_f17,
    /* kv_F18 */		ti::key_f18,
    /* kv_F19 */		ti::key_f19,
    /* kv_F20 */		ti::key_f20,
    /* kv_F21 */		ti::key_f21,
    /* kv_F22 */		ti::key_f22,
    /* kv_F23 */		ti::key_f23,
    /* kv_F24 */		ti::key_f24,
    /* kv_F25 */		ti::key_f25,
    /* kv_F26 */		ti::key_f26,
    /* kv_F27 */		ti::key_f27,
    /* kv_F28 */		ti::key_f28,
    /* kv_F29 */		ti::key_f29,
    /* kv_F30 */		ti::key_f30,
    /* kv_F31 */		ti::key_f31,
    /* kv_F32 */		ti::key_f32,
    /* kv_F33 */		ti::key_f33,
    /* kv_F34 */		ti::key_f34,
    /* kv_F35 */		ti::key_f35,
    /* kv_F36 */		ti::key_f36,
    /* kv_F37 */		ti::key_f37,
    /* kv_F38 */		ti::key_f38,
    /* kv_F39 */		ti::key_f39,
    /* kv_F40 */		ti::key_f40,
    /* kv_F41 */		ti::key_f41,
    /* kv_F42 */		ti::key_f42,
    /* kv_F43 */		ti::key_f43,
    /* kv_F44 */		ti::key_f44,
    /* kv_F45 */		ti::key_f45,
    /* kv_F46 */		ti::key_f46,
    /* kv_F47 */		ti::key_f47,
    /* kv_F48 */		ti::key_f48,
    /* kv_F49 */		ti::key_f49,
    /* kv_F50 */		ti::key_f50,
    /* kv_F51 */		ti::key_f51,
    /* kv_F52 */		ti::key_f52,
    /* kv_F53 */		ti::key_f53,
    /* kv_F54 */		ti::key_f54,
    /* kv_F55 */		ti::key_f55,
    /* kv_F56 */		ti::key_f56,
    /* kv_F57 */		ti::key_f57,
    /* kv_F58 */		ti::key_f58,
    /* kv_F59 */		ti::key_f59,
    /* kv_F60 */		ti::key_f60,
    /* kv_F61 */		ti::key_f61,
    /* kv_F62 */		ti::key_f62,
    /* kv_F63 */		ti::key_f63,
    /* kv_Find */		ti::key_find,
    /* kv_Help */		ti::key_help,
    /* kv_Home */		ti::key_home,
    /* kv_Insert */		ti::key_ic,
    /* kv_InsertLine */		ti::key_il,
    /* kv_Left */		ti::key_left,
    /* kv_Mark */		ti::key_mark,
    /* kv_Message */		ti::key_message,
    /* kv_Mouse */		ti::key_mouse,
    /* kv_Move */		ti::key_move,
    /* kv_Next */		ti::key_next,
    /* kv_Open */		ti::key_open,
    /* kv_Options */		ti::key_options,
    /* kv_PageDown */		ti::key_npage,
    /* kv_PageUp */		ti::key_ppage,
    /* kv_Previous */		ti::key_previous,
    /* kv_Print */		ti::key_print,
    /* kv_Redo */		ti::key_redo,
    /* kv_Reference */		ti::key_reference,
    /* kv_Refresh */		ti::key_refresh,
    /* kv_Replace */		ti::key_replace,
    /* kv_Restart */		ti::key_restart,
    /* kv_Resume */		ti::key_resume,
    /* kv_Right */		ti::key_right,
    /* kv_Save */		ti::key_save,
    /* kv_Select */		ti::key_select,
    /* kv_ShiftBegin */		ti::key_sbeg,
    /* kv_ShiftCancel */	ti::key_scancel,
    /* kv_ShiftCommand */	ti::key_scommand,
    /* kv_ShiftCopy */		ti::key_scopy,
    /* kv_ShiftCreate */	ti::key_screate,
    /* kv_ShiftDelete */	ti::key_sdc,
    /* kv_ShiftDeleteLine */	ti::key_sdl,
    /* kv_ShiftEnd */		ti::key_send,
    /* kv_ShiftEndOfLine */	ti::key_seol,
    /* kv_ShiftExit */		ti::key_sexit,
    /* kv_ShiftFind */		ti::key_sfind,
    /* kv_ShiftHelp */		ti::key_shelp,
    /* kv_ShiftHome */		ti::key_shome,
    /* kv_ShiftInsert */	ti::key_sic,
    /* kv_ShiftLeft */		ti::key_sleft,
    /* kv_ShiftMessage */	ti::key_smessage,
    /* kv_ShiftMove */		ti::key_smove,
    /* kv_ShiftNext */		ti::key_snext,
    /* kv_ShiftOptions */	ti::key_soptions,
    /* kv_ShiftPrevious */	ti::key_sprevious,
    /* kv_ShiftPrint */		ti::key_sprint,
    /* kv_ShiftRedo */		ti::key_sredo,
    /* kv_ShiftReplace */	ti::key_sreplace,
    /* kv_ShiftResume */	ti::key_srsume,
    /* kv_ShiftRight */		ti::key_sright,
    /* kv_ShiftSave */		ti::key_ssave,
    /* kv_ShiftSuspend */	ti::key_ssuspend,
    /* kv_ShiftTab */		ti::key_stab,
    /* kv_ShiftUndo */		ti::key_sundo,
    /* kv_Suspend */		ti::key_suspend,
    /* kv_Undo */		ti::key_undo,
    /* kv_Up */			ti::key_up,
    /* kv_UpLeft */		ti::key_a1,
    /* kv_UpRight */		ti::key_a3
};

//----------------------------------------------------------------------

} // namespace utio
