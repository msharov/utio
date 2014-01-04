// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "ti.h"
#include <termios.h>

namespace utio {

/// Takes raw codes from the input port and translates them into keycode events.
class CKeyboard {
public:
    typedef CTerminfo::keystrings_t	keymap_t;
public:
			CKeyboard (void);
		       ~CKeyboard (void);
    void		Open (const CTerminfo& rti);
    void		Close (void);
    void		EnterUIMode (void);
    void		LeaveUIMode (void);
    wchar_t		DecodeKey (istream& is) const;
    inline bool		IsInUIMode (void) const			{ return (s_bTermInUIMode); }
    inline void		LoadKeymap (const CTerminfo& rti)	{ rti.LoadKeystrings (m_Keymap); }
    wchar_t		GetKey (bool bBlock = true) const;
    bool		WaitForKeyData (long timeout = 0) const;
private:
    void		ReadKeyData (void) const;
private:
    keymap_t		m_Keymap;		///< Currently loaded keymap.
    mutable string	m_Keydata;		///< Buffered keydata.
    struct termios	m_InitialTermios;	///< What it was before we munged it.
    static bool		s_bTermInUIMode;	///< Current terminal state, static because the terminal is process-global.
};
} // namespace utio
