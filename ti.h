// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "ticonst.h"
#include "gdt.h"

namespace utio {

/// An interface to a terminfo entry.
///
/// This object reads and interprets terminfo files and can be used to set
/// colors and attributes, draw ACS characters, and decode keystrings, taking
/// a lot of headache from console programming.
///
class CTerminfo {
public:
    typedef int16_t		number_t;	///< Type of numeric terminfo values.
    typedef const char*		capout_t;	///< Type of values returned by all capability output functions.
    typedef const string&	strout_t;
    typedef string&		rstrbuf_t;
    typedef string		keystrings_t;	///< List of key strings corresponding to EKeyDataValue enum.
    typedef gdt::coord_t	coord_t;
    typedef gdt::dim_t		dim_t;
    static const char no_value[1];
public:
			CTerminfo (void);
    void		Load (const char* termname = NULL);
    void		LoadEntry (memblock& buf, const char* termname = NULL) const;
    strout_t		MoveTo (coord_t x, coord_t y) const;
    strout_t		Color (EColor fg, EColor bg = color_Preserve) const;
    capout_t		Clear (void) const;
    capout_t		AttrOn (EAttribute a) const;
    capout_t		AttrOff (EAttribute a) const;
    strout_t		Attrs (uint16_t a) const;
    capout_t		AllAttrsOff (void) const;
    inline capout_t	HideCursor (void) const			{ return GetString (ti::cursor_invisible); }
    inline capout_t	ShowCursor (void) const			{ return GetString (ti::cursor_normal); }
    strout_t		Image (coord_t x, coord_t y, dim_t w, dim_t h, const CCharCell* data) const;
    strout_t		Box (coord_t x, coord_t y, dim_t w, dim_t h) const;
    strout_t		Bar (coord_t x, coord_t y, dim_t w, dim_t h, char c = ' ') const;
    strout_t		HLine (coord_t x, coord_t y, dim_t w) const;
    strout_t		VLine (coord_t x, coord_t y, dim_t h) const;
    capout_t		Reset (void) const;
    void		ResetState (void) const;
    inline strout_t	Name (void) const			{ return _name; }
    inline dim_t	Width (void) const			{ return _nColumns; }
    inline dim_t	Height (void) const			{ return _nRows; }
    inline size_t	Colors (void) const			{ return _nColors; }
    inline size_t	ColorPairs (void) const			{ return _nPairs; }
    inline char		AcsChar (EGraphicChar c) const		{ return _acsMap[c]; }
    bool		GetBool (ti::EBooleans i) const;
    number_t		GetNumber (ti::ENumbers i) const;
    capout_t		GetString (ti::EStrings i) const;
    wchar_t		SubstituteChar (wchar_t c) const;
    void		LoadKeystrings (keystrings_t& ksv) const;
    void		Update (void);
    void		read (istream& is);
    void		write (ostream& os) const;
    size_t		stream_size (void) const;
private:
    typedef vector<int8_t>	boolvec_t;
    typedef vector<number_t>	numvec_t;
    typedef uint16_t		stroffset_t;
    typedef vector<stroffset_t>	stroffvec_t;
    typedef string		strtable_t;
    typedef unsigned long	progvalue_t;
    typedef vector<progvalue_t>	progstack_t;
    typedef tuple<acs_Last,char>	acsmap_t;
    typedef tuple<attr_Last,number_t>	progargs_t;
    /// Structure for describing alternate character set values.
    struct DLL_LOCAL SAcscInfo {
	char		m_vt100Code;	///< vt100 code for this character.
	char		m_Default;	///< Default value, if the terminfo does not specify.
	uint16_t	m_Unicode;	///< Unicode equivalent character value.
    };
private:
    static const SAcscInfo	c_AcscInfo [acs_Last];		///< Codes for all ACS characters.
    static const int16_t	c_KeyToStringMap [kv_nKeys];
    /// Current terminal state.
    class CContext {
    public:
			CContext (void);
    public:
	string		output;		///< Output string buffer.
	progstack_t	progStack;	///< Stack for running ti programs.
	gdt::Point2d	pos;		///< Current cursor position.
	uint16_t	attrs;		///< Text attributes.
	uint8_t		fg;		///< Foreground (text) color.
	uint8_t		bg;		///< Background color.
    };
public:
    static inline wchar_t AcsUnicodeValue (EGraphicChar c)	{ return c_AcscInfo[c].m_Unicode; }
private:
    void		CacheFrequentValues (void);
    void		ObtainTerminalParameters (void);
    void		NormalizeColor (EColor& fg, EColor& bg, uint16_t& attrs) const;
    void		NColor (EColor fg, EColor bg, rstrbuf_t s) const;
    void		MoveTo (coord_t x, coord_t y, rstrbuf_t s) const;
    void		Color (EColor fg, EColor bg, rstrbuf_t s) const;
    void		Attrs (uint16_t a, rstrbuf_t s) const;
    void		RunStringProgram (const char* program, string& result, progargs_t args) const;
    progvalue_t		PSPop (void) const;
    inline progvalue_t	PSPopNonzero (void) const	{ const progvalue_t v (PSPop()); return v ? v : 1; }
    void		PSPush (progvalue_t v) const;
private:
    string		_name;		///< Name of the terminfo entry.
    boolvec_t		_booleans;	///< Boolean caps.
    numvec_t		_numbers;	///< Numeric caps.
    stroffvec_t		_stringOffsets;	///< String caps (offsets into _stringTable)
    strtable_t		_stringTable;	///< Actual string caps values.
    acsmap_t		_acsMap;	///< Decoded ACS characters.
    mutable CContext	_ctx;		///< Current state of the terminal.
    uint16_t		_nColors;	///< Number of available colors.
    uint16_t		_nPairs;	///< Number of available color pairs (unused).
    dim_t		_nColumns;	///< Number of display columns.
    dim_t		_nRows;		///< Number of display rows.
};

} // namespace utio

STD_STREAMABLE (utio::CTerminfo);
