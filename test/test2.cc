// q
// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "stdmain.h"

//----------------------------------------------------------------------

/// Runs an interactive screen with a moveable box.
class CGCTest {
private:
		CGCTest (void);
	       ~CGCTest (void);
public:
		DECLARE_SINGLETON (CGCTest)
    void	Run (void);
private:
    void	Draw (CGC& gc);
    void	DrawBox (CGC& gc, Point2d pos);
private:
    CTerminfo	_ti;	///< Terminfo access object.
    CKeyboard	_kb;	///< Keyboard driver.
    EColor	_fg;	///< Foreground color of the moveable box.
    EColor	_bg;	///< Background color of the moveable box.
    Point2d	_pos;	///< Position of the moveable box.
};

//----------------------------------------------------------------------

/// Default constructor.
CGCTest::CGCTest (void)
:_ti()
,_kb()
,_fg (green)
,_bg (black)
,_pos (0, 0)
{
}

/// Destructor to clean up UI state.
CGCTest::~CGCTest (void)
{
    // Restores normal terminal state.
    // CKeyboard destructor will already reset the keyboard state, but
    // CGCTest::Draw also hides the cursor. Finally, the UI output
    // should be removed.
    cout << _ti.Reset() << _ti.Clear();
}

/// Draws a box with a hole on the screen.
void CGCTest::DrawBox (CGC& gc, Point2d pos)
{
    gc.Bar (pos[0], pos[1], 12, 4, ' ');
    gc.Box (pos[0], pos[1], 12, 4);
    gc.Bar (pos[0], pos[1] + 4, 4, 4, acsv_Checkerboard);	// acsv special character constants are in ticonst.h
    gc.Bar (pos[0] + 8, pos[1] + 4, 4, 4, acsv_Checkerboard);	// Checkerboard is the one available in most terms;
    gc.Bar (pos[0], pos[1] + 8, 12, 4, acsv_Checkerboard);	// Board works on the console only.
    gc.Text (pos[0] + 1, pos[1] + 1, "GC demo");
    gc.FgColor (lightcyan);					// The background is set by the caller
    gc.Char (pos[0] + 1, pos[1] + 2, acsv_LeftArrow);		// Arrow chars may or may not be available on your term.
    gc.Char (pos[0] + 2, pos[1] + 2, acsv_DownArrow);		// xterm, for example doesn't have them.
    gc.Char (pos[0] + 3, pos[1] + 2, acsv_UpArrow);
    gc.Char (pos[0] + 4, pos[1] + 2, acsv_RightArrow);
    gc.FgColor (green);
    gc.Text (pos[0] + 6, pos[1] + 2, "Move");
    gc.Text (pos[0] + 1, pos[1] + 3, "q to quit");
}

/// Draws two boxes on the screen, one moveable.
void CGCTest::Draw (CGC& gc)
{
    gc.Color (green, black);
    gc.Clear();				// Clears with current color; green on black.
    gc.Color (black, brown);		// Stationary, background box is brown,
    DrawBox (gc, Point2d (10, 5));	// ... and constantly at (10,5).
    gc.Color (_fg, _bg);		// Moveable box is of custom color,
    DrawBox (gc, _pos);			// ... wherever it currently is.
}

/// Runs the event loop for the demo.
void CGCTest::Run (void)
{
    _ti.Load();		// Loads the terminfo database (using $TERM)
    _kb.Open (_ti);	// Also places the terminal in UI-friendly mode.

    CGC gc;		// This is where the code draws.
    CGC screen;		// This contains the current contents of the screen.
    gc.Resize (_ti.Width(), _ti.Height());	// Full screen buffers.
    screen.Resize (_ti.Width(), _ti.Height());

    cout << _ti.HideCursor();	// Don't want the cursor to blink unless in a text box.
    cout << _ti.Clear();	// Avoid watching the screen scroll.

    for (wchar_t key = 0; key != 'q';) {
	Draw (gc);		// Draws the boxes at current positions.

	// Only the differences need to be written, so find them.
	if (gc.MakeDiffFrom (screen)) {
	    // gc now has only new stuff, the rest is zeroed out, and isn't drawn.
	    cout << _ti.Image (0, 0, gc.Width(), gc.Height(), gc.Canvas().begin());
	    cout.flush();
	    screen.Image (gc);	// Now apply the same diff to the screen cache.
	    gc.Image (screen);	// ... and copy it back for a fresh start.
	}

	key = _kb.GetKey();	// Synchronous call.

	// Moving one of the boxes with bounds checking.
	if ((key == kv_Up || key == 'k') && _pos[1] > 0)
	    --_pos[1];
	else if ((key == kv_Down || key == 'j') && _pos[1] < gc.Height() - 12)
	    ++_pos[1];
	else if ((key == kv_Left || key == 'h') && _pos[0] > 0)
	    --_pos[0];
	else if ((key == kv_Right || key == 'l') && _pos[0] < gc.Width() - 12)
	    ++_pos[0];
	// And color cycling to verify they all work.
	else if (key == 'f')
	    _fg = EColor ((_fg + 1) % color_Last);
	else if (key == 'b')
	    _bg = EColor ((_bg + 1) % color_Last);
    }
}

//----------------------------------------------------------------------

StdTestMain (CGCTest)
