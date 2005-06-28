// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// kb.cc
//

#include "kb.h"
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace utio {

//----------------------------------------------------------------------

// For some reason there is no header for these constants.
#define SHIFT_TABLE		1
#define PAGE_UP_KEYCODE		104
#define PAGE_DOWN_KEYCODE	109
#define PAGE_UP_ACTION		0x0118
#define PAGE_DOWN_ACTION	0x0119

//----------------------------------------------------------------------

/// Constructs node with id \p nodeId.
CKeyboard::CKeyboard (void)
: m_Keymap (),
#if UTIO_WANT_GETKEY
  m_Keydata (),
#endif
  m_InitialTermios (),
  m_KbeScrollForward (0),
  m_KbeScrollBackward (0),
  m_bTermInUIMode (false)
{
    fill_n ((void*) &m_InitialTermios, sizeof(struct termios), '\x0');
    #if UTIO_WANT_GETKEY
	m_Keydata.reserve (64);
    #endif
}

/// Destructor cleans up keyboard in case of abnormal termination.
CKeyboard::~CKeyboard (void)
{
    Close();
}

//----------------------------------------------------------------------

/// Loads the keymap and enters UI mode.
void CKeyboard::Open (const CTerminfo& rti)
{
    LoadKeymap (rti);
    EnterUIMode();
    #if UTIO_WANT_GETKEY
	int flag;
	if ((flag = fcntl (STDIN_FILENO, F_GETFL)) < 0)
	    throw file_exception ("fcntl", "stdin");
	if ((flag = fcntl (STDIN_FILENO, F_SETFL, flag | O_NONBLOCK)) < 0)
	    throw file_exception ("fcntl", "stdin");
    #endif
}

/// Leaves UI mode.
void CKeyboard::Close (void)
{
    LeaveUIMode();
    #if UTIO_WANT_GETKEY
	int flag;
	if ((flag = fcntl (STDIN_FILENO, F_GETFL)) >= 0)
	    fcntl (STDIN_FILENO, F_SETFL, flag & ~O_NONBLOCK);
    #endif
}

#if UTIO_WANT_GETKEY
/// Reads a key from stdin.
wchar_t CKeyboard::GetKey (metastate_t* pMeta, bool bBlock) const
{
    wchar_t key = 0;
    metastate_t meta;
    istream is;
    do {
	if (m_Keydata.empty() && bBlock)
	    WaitForKeyData();
	ReadKeyData();
	is.link (m_Keydata);
    } while (!DecodeKey (is, key, meta) && bBlock);
    m_Keydata.erase (m_Keydata.begin(), is.pos());
    if (pMeta)
	*pMeta = meta;
    return (key);
}

/// Reads all available stdin data (nonblocking)
void CKeyboard::ReadKeyData (void) const
{
    ostream os (m_Keydata.end(), m_Keydata.capacity() - m_Keydata.size());
    errno = 0;
    while (os.remaining()) {
	ssize_t br = read (STDIN_FILENO, os.ipos(), os.remaining());
	if (br > 0)
	    os.skip (br);
	else if (br < 0 && errno != EAGAIN && errno != EINTR)
	    throw file_exception ("read", "stdin");
	else
	    break;
    }
    m_Keydata.resize (m_Keydata.size() + os.pos());
}

/// Blocks until something is available on stdin. Returns false on \p timeout.
bool CKeyboard::WaitForKeyData (suseconds_t timeout) const
{
    fd_set fds;
    FD_ZERO (&fds);
    FD_SET (STDIN_FILENO, &fds);
    struct timeval tv = { 0, timeout };
    errno = 0;
    int rv;
    do { rv = select (1, &fds, NULL, NULL, timeout ? &tv : NULL); } while (errno == EINTR);
    if (rv < 0)
	throw file_exception ("select", "stdin");
    return (rv);
}
#endif

//----------------------------------------------------------------------

/// Enters UI mode.
///
/// This turns off various command-line stuff, like buffering, echoing,
/// scroll lock, shift-pgup/dn, etc., which can be very ugly or annoying
/// in a GUI application.
///
void CKeyboard::EnterUIMode (void)
{
    if (m_bTermInUIMode)
	return;
    if (!isatty (STDIN_FILENO))
	throw domain_error ("This application only works on a tty.");

    int flag;
    if ((flag = fcntl (STDIN_FILENO, F_GETFL)) < 0)
	throw file_exception ("F_GETFL", "stdin");
    if (fcntl (STDIN_FILENO, F_SETFL, flag | O_NONBLOCK))
	throw file_exception ("F_SETFL", "stdin");

    if (-1 == tcgetattr (STDIN_FILENO, &m_InitialTermios))
	throw libc_exception ("tcgetattr");
    struct termios tios (m_InitialTermios);
    tios.c_lflag &= ~(ICANON | ECHO);	// No by-line buffering, no echo.
    tios.c_iflag &= ~(IXON | IXOFF);	// No ^s scroll lock (whose dumb idea was it?)
    tios.c_cc[VMIN] = 1;		// Read at least 1 character on each read().
    tios.c_cc[VTIME] = 0;		// Disable time-based preprocessing (Esc sequences)
    tios.c_cc[VQUIT] = 0xFF;		// Disable ^\. Root window will handle.
    tios.c_cc[VSUSP] = 0xFF;		// Disable ^z. Suspends in UI mode result in garbage.

    if (-1 == tcflush (STDIN_FILENO, TCIFLUSH))	// Flush the input queue; who knows what was pressed.
	throw libc_exception ("tcflush");

    m_bTermInUIMode = true;		// Cleanup is needed after the next statement.
    if (-1 == tcsetattr (STDIN_FILENO, TCSAFLUSH, &tios))
	throw libc_exception ("tcsetattr");
    #ifdef NDEBUG
	SetKeyboardEntry (SHIFT_TABLE, PAGE_UP_KEYCODE, PAGE_UP_ACTION, &m_KbeScrollBackward);
	SetKeyboardEntry (SHIFT_TABLE, PAGE_DOWN_KEYCODE, PAGE_DOWN_ACTION, &m_KbeScrollForward);
    #endif
}

/// Manipulates terminal keymap (what loadkeys does)
void CKeyboard::SetKeyboardEntry (uint8_t table, uint8_t keycode, uint16_t value, uint16_t* oldValue)
{
    struct kbentry kbe;
    kbe.kb_table = table;
    kbe.kb_index = keycode;
    if (oldValue)
	*oldValue = 0;
    if (ioctl (STDIN_FILENO, KDGKBENT, &kbe))
	return;
    if (oldValue && !*oldValue)
	*oldValue = kbe.kb_value;
    if (kbe.kb_value != value) {
	kbe.kb_value = value;
	if (ioctl (STDIN_FILENO, KDSKBENT, &kbe))
	    throw file_exception ("KDGKBENT", "stdin");
    }
}

/// Leaves UI mode.
void CKeyboard::LeaveUIMode (void)
{
    if (!m_bTermInUIMode)
	return;
    tcflush (STDIN_FILENO, TCIFLUSH);	// Should not leave any garbage for the shell
    if (tcsetattr (STDIN_FILENO, TCSANOW, &m_InitialTermios))
	throw file_exception ("tcsetattr", "stdin");
    m_bTermInUIMode = false;
    if (m_KbeScrollBackward)
	SetKeyboardEntry (SHIFT_TABLE, PAGE_UP_KEYCODE, m_KbeScrollBackward);
    if (m_KbeScrollForward)
	SetKeyboardEntry (SHIFT_TABLE, PAGE_DOWN_KEYCODE, m_KbeScrollForward);
}

/// Reads the updated terminfo database.
void CKeyboard::LoadKeymap (const CTerminfo& rti)
{
    rti.LoadKeystrings (m_Keymap);
}

//----------------------------------------------------------------------

/// Decodes a keystring in \p str that was read from stdin into an eventcode.
bool CKeyboard::DecodeKey (istream& is, wchar_t& kv, metastate_t& kf) const
{
    kf.reset();
    kv = 0;
    if (!is.remaining())
	return (false);

    // Find the longest match in the keymap.
    size_t matchedSize = 0;
    const string::const_iterator sfirst (is.ipos());
    for (uoff_t i = 0; i < m_Keymap.size(); ++ i) {
	const CTerminfo::capout_t ks = m_Keymap[i];
	const size_t kss = strlen (ks);
	if (kss > is.remaining() || kss < matchedSize)
	    continue;
	const string::const_iterator slast (is.ipos() + kss);
	if (equal (sfirst, slast, ks)) {
	    kv = i + kv_First;
	    matchedSize = kss;
	}
    }

    // Alt+key produces Esc plus that key. Sometimes.
    if (kv == kv_Esc) {
	kf.set (mksbit_Alt);
	is.skip (matchedSize);
	matchedSize = 0;
    }

    // Read the keystring as UTF-8 if enough bytes are available,
    if (!matchedSize && is.remaining()) {
	matchedSize = Utf8SequenceBytes(*is.ipos());
	if (matchedSize <= is.remaining())
	    kv = *utf8in(is.ipos());
	else { // it is some weird 8-bit value that is reported as is.
	    kv = *is.ipos();
	    matchedSize = 1;
	}
	// Ctrl+key produces key-0x60. Sometimes. Except for tab, which is useful as it is.
	if (isalpha (kv + 0x60) && kv != '\t') {
	    kf.set (mksbit_Ctrl);
	    kv += 0x60;
	}
	if (isupper (kv))
	    kf.set (mksbit_Shift);
    }

    // If it is really an ESC key, then reset variables.
    if (!matchedSize && kv == kv_Esc) {
	matchedSize = strlen (m_Keymap [kv - kv_First]);
	is.seek (is.pos() - matchedSize);
	kf.reset (mksbit_Alt);
    }

    // Try to report shift state.
    if (matchedSize) {
	// Warning: this is unreliable and only works on a vt.
	#define TIOCL_GETSHIFTSTATE	6 // This is some internal kernel constant.
	int sstate = TIOCL_GETSHIFTSTATE;
	if (!ioctl (STDIN_FILENO, TIOCLINUX, &sstate)) {
	    if (sstate & 1)
		kf.set (mksbit_Shift);
	    if (sstate & 4)
		kf.set (mksbit_Ctrl);
	    if (sstate & 10)
		kf.set (mksbit_Alt);
	}
	is.skip (matchedSize);
    }
    return (matchedSize);
}

} // namespace utio

