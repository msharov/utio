// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdmain.h"

//----------------------------------------------------------------------

/// Runs an interactive screen with a moveable box.
class CGCDemo {
private:
		CGCDemo (void);
	       ~CGCDemo (void);
public:
		DECLARE_SINGLETON (CGCDemo)
    void	Run (void);
private:
    void	Draw (CGC& gc);
    void	DrawBox (CGC& gc, Point2d pos);
private:
    CTerminfo	m_TI;	///< Terminfo access object.
    CKeyboard	m_Kb;	///< Keyboard driver.
    EColor	m_Fg;	///< Foreground color of the moveable box.
    EColor	m_Bg;	///< Background color of the moveable box.
    Point2d	m_Pos;	///< Position of the moveable box.
};

//----------------------------------------------------------------------

/// Default constructor.
CGCDemo::CGCDemo (void)
: m_TI (),
  m_Kb (),
  m_Fg (green),
  m_Bg (black),
  m_Pos (0, 0)
{
}

/// Destructor to clean up UI state.
CGCDemo::~CGCDemo (void)
{
    cout << m_TI.Reset();	// Restores normal terminal state.
}

/// Draws a box with a hole on the screen.
void CGCDemo::DrawBox (CGC& gc, Point2d pos)
{
    gc.Bar (pos[0], pos[1], 12, 4, acsv_Board);		// acsv special character constants are in ticonst.h
    gc.Box (pos[0], pos[1], 12, 4);
    gc.Bar (pos[0], pos[1] + 4, 4, 4, acsv_Board);
    gc.Bar (pos[0] + 8, pos[1] + 4, 4, 4, acsv_Board);
    gc.Bar (pos[0], pos[1] + 8, 12, 4, acsv_Board);
    gc.Text (pos[0] + 1, pos[1] + 1, "GC demo");
    gc.FgColor (lightcyan);				// The background is set by the caller
    gc.Char (pos[0] + 1, pos[1] + 2, acsv_LeftArrow);
    gc.Char (pos[0] + 2, pos[1] + 2, acsv_DownArrow);
    gc.Char (pos[0] + 3, pos[1] + 2, acsv_UpArrow);
    gc.Char (pos[0] + 4, pos[1] + 2, acsv_RightArrow);
    gc.FgColor (green);
    gc.Text (pos[0] + 6, pos[1] + 2, "Move");
    gc.Text (pos[0] + 1, pos[1] + 3, "q to quit");
}

/// Draws two boxes on the screen, one moveable.
void CGCDemo::Draw (CGC& gc)
{
    gc.Color (green, black);
    gc.Clear();				// Clears with current color; green on black.
    gc.Color (black, brown);		// Stationary, background box is brown,
    DrawBox (gc, Point2d (10, 5));	// ... and constantly at (10,5).
    gc.Color (m_Fg, m_Bg);		// Moveable box is of custom color,
    DrawBox (gc, m_Pos);		// ... wherever it currently is.
}

/// Runs the event loop for the demo.
void CGCDemo::Run (void)
{
    m_TI.Load();	// Loads the terminfo database (using $TERM)
    m_Kb.Open (m_TI);	// Also places the terminal in UI-friendly mode.

    CGC gc;		// This is where the code draws.
    CGC screen;		// This contains the current contents of the screen.
    gc.Resize (m_TI.Width(), m_TI.Height());	// Full screen buffers.
    screen.Resize (m_TI.Width(), m_TI.Height());

    cout << m_TI.HideCursor();	// Don't want the cursor to blink unless in a text box.

    wchar_t key = 0;
    while (key != 'q') {
	Draw (gc);		// Draws the boxes at current positions.

	// Only the differences need to be written, so find them.
	gc.MakeDiffFrom (screen);
	// gc now has only new stuff, the rest is zeroed out, and isn't drawn.
	cout << m_TI.Image (0, 0, gc.Width(), gc.Height(), gc.Canvas().begin());
	cout.flush();
	screen.Image (gc);	// Now apply the same diff to the screen cache.
	gc.Image (screen);	// ... and copy it back for a fresh start.

	key = m_Kb.GetKey();	// Synchronous call.

	// Moving one of the boxes with bounds checking.
	if ((key == kv_Up || key == 'k') && m_Pos[1] > 0)
	    -- m_Pos[1];
	else if ((key == kv_Down || key == 'j') && m_Pos[1] < gc.Height() - 12)
	    ++ m_Pos[1];
	else if ((key == kv_Left || key == 'h') && m_Pos[0] > 0)
	    -- m_Pos[0];
	else if ((key == kv_Right || key == 'l') && m_Pos[0] < gc.Width() - 12)
	    ++ m_Pos[0];
	// And color cycling to verify they all work.
	else if (key == 'f')
	    m_Fg = EColor ((m_Fg + 1) % color_Last);
	else if (key == 'b')
	    m_Bg = EColor ((m_Bg + 1) % color_Last);
    }
}

//----------------------------------------------------------------------

StdDemoMain (CGCDemo)

//----------------------------------------------------------------------

