// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdmain.h"

//----------------------------------------------------------------------

class CHelloWorld {
    SINGLETON (CHelloWorld)	// See stdmain.h
public:
    void	Run (void);
private:
    CTerminfo	m_TI;
};

//----------------------------------------------------------------------

/// Prints a bunch of multicolored lines.
void CHelloWorld::Run (void)
{
    m_TI.Load();		// Just loads the terminfo (using $TERM)

    cout << ios::hex;
    for (int y = black; y < color_Last; ++ y) {
	for (int x = black; x < color_Last; ++ x)
	    cout << m_TI.Color (EColor(x), EColor(y)) << x;
	cout << m_TI.Color (green, black) << "\tHello World!" << endl;
    }

    cout << m_TI.AllAttrsOff();	// Not using Reset() to keep the output onscreen.
}

/// Default constructor.
CHelloWorld::CHelloWorld (void)
: m_TI ()
{
}

//----------------------------------------------------------------------

StdDemoMain (CHelloWorld)

//----------------------------------------------------------------------

