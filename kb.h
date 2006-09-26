// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// kb.h
//

#ifndef KB_H_70C9E93B05A3B0D527D942E351FEEE4B
#define KB_H_70C9E93B05A3B0D527D942E351FEEE4B

#include "ti.h"
#include <termios.h>

namespace utio {

/// \class CKeyboard kb.h utio/kb.h
///
/// \brief Keyboard code preprocessor for terminals.
///
/// Takes raw codes from the input port and translates them into keycode events.
///
class CKeyboard {
public:
    typedef bitset<mksbit_Last>		metastate_t;
    typedef CTerminfo::keystrings_t	keymap_t;
public:
			CKeyboard (void);
		       ~CKeyboard (void);
    void		Open (const CTerminfo& rti);
    void		Close (void);
    void		EnterUIMode (void);
    void		LeaveUIMode (void);
    void		LoadKeymap (const CTerminfo& rti);
    bool		DecodeKey (istream& is, wchar_t& kv, metastate_t& kf) const;
    inline bool		IsInUIMode (void) const	{ return (s_bTermInUIMode); }
#if UTIO_WANT_GETKEY
    wchar_t		GetKey (metastate_t* pMeta = NULL, bool bBlock = true) const;
    bool		WaitForKeyData (long timeout = 0) const;
private:
    void		ReadKeyData (void) const;
#endif
private:
    keymap_t		m_Keymap;		///< Currently loaded keymap.
#if UTIO_WANT_GETKEY
    mutable string	m_Keydata;		///< Buffered keydata.
#endif
    struct termios	m_InitialTermios;	///< What it was before we munged it.
    static bool		s_bTermInUIMode;	///< Current terminal state, static because the terminal is process-global.
};

} // namespace utio

#endif


