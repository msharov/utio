// This file is part of the utio library, an terminal I/O library.
// Copyright (C) 2004 by Mike Sharov <msharov@talentg.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
// Boston, MA  02111-1307  USA.
//
// kb.h
//

#ifndef KB_H_70C9E93B05A3B0D527D942E351FEEE4B
#define KB_H_70C9E93B05A3B0D527D942E351FEEE4B

#include "ti.h"
#include <termios.h>

#ifndef TI_H_2B8E70FA3501DD2B50EF36C24E2DC717
#error Include ti.h
#endif
#ifndef TICONST_H_668685586A306EC93FCA2A4B233066A9
#error include ticonst.h
#endif

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
    inline bool		IsInUIMode (void) const	{ return (m_bTermInUIMode); }
#if WANT_GETKEY
    wchar_t		GetKey (metastate_t* pMeta = NULL, bool bBlock = true) const;
    void		WaitForKeyData (suseconds_t timeout = 0) const;
private:
    void		ReadKeyData (void) const;
#endif
protected:
    void		EnterUIMode (void);
    void		LeaveUIMode (void);
    void		LoadKeymap (const CTerminfo& rti);
    bool		DecodeKey (istream& is, wchar_t& kv, metastate_t& kf) const;
    void		SetKeyboardEntry (uint8_t table, uint8_t keycode, uint16_t value, uint16_t* oldValue = NULL);
private:
    keymap_t		m_Keymap;
#if WANT_GETKEY
    mutable string	m_Keydata;
#endif
    struct termios	m_InitialTermios;
    uint16_t		m_KbeScrollForward;
    uint16_t		m_KbeScrollBackward;
    bool		m_bTermInUIMode;
};

} // namespace utio

#endif


