// "OPOS[17~[20~[24~[2~q"	This line contains test keystrokes
//
// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "stdmain.h"

//----------------------------------------------------------------------

/// Demonstrates the capabilities of the CKeyboard class.
class CKeyboardDemo {
private:
    inline		CKeyboardDemo (void) :_ti(),_kb() {}
public:
			DECLARE_SINGLETON (CKeyboardDemo)
    void		Run (void);
private:
    CTerminfo		_ti;	///< The terminfo database.
    CKeyboard		_kb;	///< The keyboard driver.
private:
    static const char*	c_KeyNameMap [kv_nKeys];
    static const char*	c_MetaBitNames [mksbit_Last];
};

//----------------------------------------------------------------------

/// Prints pressed keys until told to stop.
void CKeyboardDemo::Run (void)
{
    _ti.Load();	// Loads the terminfo database (using $TERM)
    _kb.Open (_ti);	// Also places the terminal in UI-friendly mode.

    cout << "Keyboard demo. Press keys to print their value, 'q' to quit." << endl;

    wchar_t key = 0;
    while (key != 'q') {
	cout.flush();
	key = _kb.GetKey();

	cout << "Got key: ";
	for (uoff_t i = 0; i < mksbit_Last; ++ i)
	    if (key & (1 << (i + kf_MetaBit)))
		cout << c_MetaBitNames[i] << ", ";
	key &= KV_MASK;
	if (key >= kv_First && key < kv_Last)	// Special key
	    cout << c_KeyNameMap [key - kv_First];
	else if (key == kv_Tab)			// Tab is just the \t character
	    cout << "Tab";
	else if (key == kv_Space)		// So is space.
	    cout << "Space";
	else if (key == kv_Enter)		// So is enter.
	    cout << "Enter";
	else					// Otherwise it's a normal key.
	    cout << key;
	cout << endl;
    }
}

//----------------------------------------------------------------------

StdDemoMain (CKeyboardDemo)

//----------------------------------------------------------------------
//{{{ Names for all the special keycodes.

const char* CKeyboardDemo::c_KeyNameMap [kv_nKeys] = {
    "Esc",		// kv_Esc
    "Backspace",	// kv_Backspace
    "Backtab",		// kv_Backtab
    "Begin",		// kv_Begin
    "CATab",		// kv_CATab
    "CTab",		// kv_CTab
    "Cancel",		// kv_Cancel
    "Center",		// kv_Center
    "Clear",		// kv_Clear
    "ClearToEOL",	// kv_ClearToEOL
    "ClearToEOS",	// kv_ClearToEOS
    "Close",		// kv_Close
    "Command",		// kv_Command
    "Copy",		// kv_Copy
    "Create",		// kv_Create
    "Delete",		// kv_Delete
    "DeleteLine",	// kv_DeleteLine
    "Down",		// kv_Down
    "DownLeft",		// kv_DownLeft
    "DownRight",	// kv_DownRight
    "End",		// kv_End
    "Exit",		// kv_Exit
    "F0",		// kv_F0
    "F1",		// kv_F1
    "F2",		// kv_F2
    "F3",		// kv_F3
    "F4",		// kv_F4
    "F5",		// kv_F5
    "F6",		// kv_F6
    "F7",		// kv_F7
    "F8",		// kv_F8
    "F9",		// kv_F9
    "F10",		// kv_F10
    "F11",		// kv_F11
    "F12",		// kv_F12
    "F13",		// kv_F13
    "F14",		// kv_F14
    "F15",		// kv_F15
    "F16",		// kv_F16
    "F17",		// kv_F17
    "F18",		// kv_F18
    "F19",		// kv_F19
    "F20",		// kv_F20
    "F21",		// kv_F21
    "F22",		// kv_F22
    "F23",		// kv_F23
    "F24",		// kv_F24
    "F25",		// kv_F25
    "F26",		// kv_F26
    "F27",		// kv_F27
    "F28",		// kv_F28
    "F29",		// kv_F29
    "F30",		// kv_F30
    "F31",		// kv_F31
    "F32",		// kv_F32
    "F33",		// kv_F33
    "F34",		// kv_F34
    "F35",		// kv_F35
    "F36",		// kv_F36
    "F37",		// kv_F37
    "F38",		// kv_F38
    "F39",		// kv_F39
    "F40",		// kv_F40
    "F41",		// kv_F41
    "F42",		// kv_F42
    "F43",		// kv_F43
    "F44",		// kv_F44
    "F45",		// kv_F45
    "F46",		// kv_F46
    "F47",		// kv_F47
    "F48",		// kv_F48
    "F49",		// kv_F49
    "F50",		// kv_F50
    "F51",		// kv_F51
    "F52",		// kv_F52
    "F53",		// kv_F53
    "F54",		// kv_F54
    "F55",		// kv_F55
    "F56",		// kv_F56
    "F57",		// kv_F57
    "F58",		// kv_F58
    "F59",		// kv_F59
    "F60",		// kv_F60
    "F61",		// kv_F61
    "F62",		// kv_F62
    "F63",		// kv_F63
    "Find",		// kv_Find
    "Help",		// kv_Help
    "Home",		// kv_Home
    "Insert",		// kv_Insert
    "InsertLine",	// kv_InsertLine
    "Left",		// kv_Left
    "Mark",		// kv_Mark
    "Message",		// kv_Message
    "Mouse",		// kv_Mouse
    "Move",		// kv_Move
    "Next",		// kv_Next
    "Open",		// kv_Open
    "Options",		// kv_Options
    "PageDown",		// kv_PageDown
    "PageUp",		// kv_PageUp
    "Previous",		// kv_Previous
    "Print",		// kv_Print
    "Redo",		// kv_Redo
    "Reference",	// kv_Reference
    "Refresh",		// kv_Refresh
    "Replace",		// kv_Replace
    "Restart",		// kv_Restart
    "Resume",		// kv_Resume
    "Right",		// kv_Right
    "Save",		// kv_Save
    "Select",		// kv_Select
    "ShiftBegin",	// kv_ShiftBegin
    "ShiftCancel",	// kv_ShiftCancel
    "ShiftCommand",	// kv_ShiftCommand
    "ShiftCopy",	// kv_ShiftCopy
    "ShiftCreate",	// kv_ShiftCreate
    "ShiftDelete",	// kv_ShiftDelete
    "ShiftDeleteLine",	// kv_ShiftDeleteLine
    "ShiftEnd",		// kv_ShiftEnd
    "ShiftEndOfLine",	// kv_ShiftEndOfLine
    "ShiftExit",	// kv_ShiftExit
    "ShiftFind",	// kv_ShiftFind
    "ShiftHelp",	// kv_ShiftHelp
    "ShiftHome",	// kv_ShiftHome
    "ShiftInsert",	// kv_ShiftInsert
    "ShiftLeft",	// kv_ShiftLeft
    "ShiftMessage",	// kv_ShiftMessage
    "ShiftMove",	// kv_ShiftMove
    "ShiftNext",	// kv_ShiftNext
    "ShiftOptions",	// kv_ShiftOptions
    "ShiftPrevious",	// kv_ShiftPrevious
    "ShiftPrint",	// kv_ShiftPrint
    "ShiftRedo",	// kv_ShiftRedo
    "ShiftReplace",	// kv_ShiftReplace
    "ShiftResume",	// kv_ShiftResume
    "ShiftRight",	// kv_ShiftRight
    "ShiftSave",	// kv_ShiftSave
    "ShiftSuspend",	// kv_ShiftSuspend
    "ShiftTab",		// kv_ShiftTab
    "ShiftUndo",	// kv_ShiftUndo
    "Suspend",		// kv_Suspend
    "Undo",		// kv_Undo
    "Up",		// kv_Up
    "UpLeft",		// kv_UpLeft
    "UpRight"		// kv_UpRight
};

/// Names for the meta bits on the keycodes.
const char* CKeyboardDemo::c_MetaBitNames [mksbit_Last] = {
    "Shift",
    "Alt",
    "Ctrl",
    "Meta",
    "Command",
    "NumLock",
    "CapsLock",
    "ScrollLock"
};

//}}}-------------------------------------------------------------------
