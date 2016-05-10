// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "kb.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace utio {

//----------------------------------------------------------------------

// One per process, just like the terminal.
/*static*/ bool CKeyboard::s_bTermInUIMode = false;

//----------------------------------------------------------------------

/// Constructs node with id \p nodeId.
CKeyboard::CKeyboard (void)
:_keymap()
,_keydata()
,_initialTermios()
,_keypadoffstr("")
,_curB (0)
{
    memset (&_initialTermios, 0, sizeof(struct termios));
    _keydata.reserve (64);
}

/*static*/ void CKeyboard::Error (const char* f)
{
    throw file_exception (f, "stdin");
}

//----------------------------------------------------------------------

/// Loads the keymap and enters UI mode.
void CKeyboard::Open (const CTerminfo& rti)
{
    LoadKeymap (rti);
    EnterUIMode();
    cin.set_nonblock();
    cout << rti.GetString (ti::keypad_xmit);
    _keypadoffstr = rti.GetString (ti::keypad_local);
}

/// Leaves UI mode.
void CKeyboard::Close (void)
{
    LeaveUIMode();
    cout << _keypadoffstr;
    cin.set_nonblock (false);
}

/// Reads a key from stdin.
wchar_t CKeyboard::GetKey (bool bBlock)
{
    wchar_t key = 0;
    istream is;
    do {
	if (_keydata.empty() && bBlock)
	    WaitForKeyData();
	ReadKeyData();
	is.link (_keydata);
    } while (!(key = DecodeKey(is)) && bBlock);
    _keydata.erase (_keydata.begin(), is.pos());
    return key;
}

/// Reads all available stdin data (nonblocking)
void CKeyboard::ReadKeyData (void)
{
    ostream os (_keydata.end(), _keydata.capacity() - _keydata.size());
    errno = 0;
    while (os.remaining()) {
	auto br = read (STDIN_FILENO, os.ipos(), os.remaining());
	if (br > 0)
	    os.skip (br);
	else if (br < 0 && errno != EAGAIN && errno != EINTR)
	    Error ("read");
	else
	    break;
    }
    _keydata.resize (_keydata.size() + os.pos());
}

/// Blocks until something is available on stdin. Returns false on \p timeout.
bool CKeyboard::WaitForKeyData (long timeout) const
{
    fd_set fds;
    FD_ZERO (&fds);
    FD_SET (STDIN_FILENO, &fds);
    struct timeval tv = { 0, timeout };
    struct timeval* ptv = timeout ? &tv : nullptr;
    errno = 0;
    int rv;
    do {
	errno = 0;
	rv = select (1, &fds, nullptr, nullptr, ptv);
    } while (errno == EINTR);
    if (rv < 0)
	Error ("select");
    return rv;
}

//----------------------------------------------------------------------

/// Enters UI mode.
///
/// This turns off various command-line stuff, like buffering, echoing,
/// scroll lock, shift-pgup/dn, etc., which can be very ugly or annoying
/// in a GUI application.
///
void CKeyboard::EnterUIMode (void)
{
    if (s_bTermInUIMode || !isatty (STDIN_FILENO))
	return;

    int flag;
    if ((flag = fcntl (STDIN_FILENO, F_GETFL)) < 0)
	Error ("F_GETFL");
    if (fcntl (STDIN_FILENO, F_SETFL, flag | O_NONBLOCK))
	Error ("F_SETFL");

    if (0 > tcgetattr (STDIN_FILENO, &_initialTermios))
	Error ("tcgetattr");
    struct termios tios (_initialTermios);
    tios.c_lflag &= ~(ICANON | ECHO);	// No by-line buffering, no echo.
    tios.c_iflag &= ~(IXON | IXOFF);	// No ^s scroll lock
    tios.c_cc[VMIN] = 1;		// Read at least 1 character on each read().
    tios.c_cc[VTIME] = 0;		// Disable time-based preprocessing (Esc sequences)
    tios.c_cc[VQUIT] = 0xff;		// Disable ^\. Root window will handle.
    tios.c_cc[VSUSP] = 0xff;		// Disable ^z. Suspends in UI mode result in garbage.

    if (0 > tcflush (STDIN_FILENO, TCIFLUSH))	// Flush the input queue; who knows what was pressed.
	Error ("tcflush");

    s_bTermInUIMode = true;		// Cleanup is needed after the next statement.
    signal (SIGTSTP, SIG_IGN);		// Disable ^z suspend.
    if (0 > tcsetattr (STDIN_FILENO, TCSAFLUSH, &tios))
	Error ("tcsetattr");
}

/// Leaves UI mode.
void CKeyboard::LeaveUIMode (void)
{
    if (!s_bTermInUIMode)
	return;
    tcflush (STDIN_FILENO, TCIFLUSH);	// Should not leave any garbage for the shell
    if (tcsetattr (STDIN_FILENO, TCSANOW, &_initialTermios))
	Error ("tcsetattr");
    signal (SIGTSTP, SIG_DFL);		// Re-enable ^z suspend.
    s_bTermInUIMode = false;
}

//----------------------------------------------------------------------

/// Decodes a keystring in \p str that was read from stdin into an eventcode.
wchar_t CKeyboard::DecodeKey (istream& is)
{
    wchar_t kv = 0;
    if (!is.remaining())
	return kv;

    // Find the longest match in the keymap.
    size_t matchedSize = 0, kss, ki = 0;
    for (auto ks = _keymap.begin(); ki < kv_nKeys; ++ki, ks += kss + 1) {
	if ((kss = strlen(ks)) <= is.remaining() && kss > matchedSize && strncmp (is.ipos(), ks, kss) == 0) {
	    kv = ki + kv_First;
	    matchedSize = kss;
	}
    }
    is.skip (matchedSize);

    // Read the keystring as UTF-8 if enough bytes are available,
    if ((!kv || kv == kv_Esc) && is.remaining() && (matchedSize = min (Utf8SequenceBytes(*is.ipos()), is.remaining()))) {
	auto kc = *is.ipos();
	if (isalpha (kc + 0x60) && kc != '\t' && kc != '\n') {
	    kc += 0x60;
	    kv = (isupper(kc) ? kf_Alt : kf_Ctrl) | tolower(kc);
	} else
	    kv = (((kv!=kv_Esc)-1) & kf_Alt) | *utf8in(is.ipos());
	is.skip (matchedSize);
    }
    if (kv == 0x7F)	// xterms are not always consistent with this...
	kv = kv_Backspace;
    if (kv == kv_KPEnter)	// always use kv_Enter instead
	kv = kv_Enter;

    // Decode mouse data
    if (kv == kv_Mouse && is.remaining() >= 3) {
	uint8_t x, y, b, bs;
	b = is.ipos()[0]; x = is.ipos()[1]; y = is.ipos()[2];
	x -= '!'; y -= '!'; b = (b + 1) & 3;			// Coordinates are '!'-based, buttons are 0,1,2 with motion and drag flags, which I mask off.
	bs = b - _curB; bs = (bs != 0) + (int8_t(bs) < 0);	// Convert to "down" or "up" values.
	kv = kf_Mouse | (bs << 18) | (max(b,_curB) << 16) | (x << 8) | y;	// Packing should match UMouseEvent structure (doing it manually here generates considerably less code)
	_curB = b;
	is.skip (3);
    }
    return kv;
}

} // namespace utio
