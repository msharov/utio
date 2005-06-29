// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdmain.h"

//----------------------------------------------------------------------

/// Demonstrates the capabilities of the CKeyboard class.
class CKeyboardDemo {
    SINGLETON (CKeyboardDemo)
public:
    void		Run (void);
private:
    CTerminfo		m_TI;
    CKeyboard		m_Kb;
private:
    static const char*	c_KeyNameMap [kv_nKeys];
    static const char*	c_MetaBitNames [mksbit_Last];
};

//----------------------------------------------------------------------

/// Default constructor.
CKeyboardDemo::CKeyboardDemo (void)
: m_TI (),
  m_Kb ()
{
}

/// Prints pressed keys until told to stop.
void CKeyboardDemo::Run (void)
{
    m_TI.Load();	// Just loads the terminfo (using $TERM)
    m_Kb.Open (m_TI);	// Also places the terminal in UI-friendly mode.

    cout << "Keyboard demo. Press keys to print their value, 'q' to quit." << endl;

    wchar_t key = 0;
    while (key != 'q') {
	cout.flush();
	CKeyboard::metastate_t meta;
	key = m_Kb.GetKey (&meta);

	cout << "Got key: ";
	if (key >= kv_First && key < kv_Last)	// Special key
	    cout << c_KeyNameMap [key - kv_First];
	else if (key == kv_Tab)			// Tab is just the \t character
	    cout << "Tab";
	else if (key == kv_Space)		// So is space.
	    cout << "Space";
	else					// Otherwise it's a normal key.
	    cout << key;
	for (uoff_t i = 0; i < mksbit_Last; ++ i)
	    if (meta[i])
		cout << ", " << c_MetaBitNames[i];
	cout << endl;
    }
}

//----------------------------------------------------------------------

StdDemoMain (CKeyboardDemo)

//----------------------------------------------------------------------

/// Names for all the special keycodes.
const char* CKeyboardDemo::c_KeyNameMap [kv_nKeys] = {
    /* kv_Esc */		"Esc",
    /* kv_Backspace */		"Backspace",
    /* kv_Backtab */		"Backtab",
    /* kv_Begin */		"Begin",
    /* kv_CATab */		"CATab",
    /* kv_CTab */		"CTab",
    /* kv_Cancel */		"Cancel",
    /* kv_Center */		"Center",
    /* kv_Clear */		"Clear",
    /* kv_ClearToEOL */		"ClearToEOL",
    /* kv_ClearToEOS */		"ClearToEOS",
    /* kv_Close */		"Close",
    /* kv_Command */		"Command",
    /* kv_Copy */		"Copy",
    /* kv_Create */		"Create",
    /* kv_Delete */		"Delete",
    /* kv_DeleteLine */		"DeleteLine",
    /* kv_Down */		"Down",
    /* kv_DownLeft */		"DownLeft",
    /* kv_DownRight */		"DownRight",
    /* kv_End */		"End",
    /* kv_Enter */		"Enter",
    /* kv_Exit */		"Exit",
    /* kv_F0 */			"F0",
    /* kv_F1 */			"F1",
    /* kv_F2 */			"F2",
    /* kv_F3 */			"F3",
    /* kv_F4 */			"F4",
    /* kv_F5 */			"F5",
    /* kv_F6 */			"F6",
    /* kv_F7 */			"F7",
    /* kv_F8 */			"F8",
    /* kv_F9 */			"F9",
    /* kv_F10 */		"F10",
    /* kv_F11 */		"F11",
    /* kv_F12 */		"F12",
    /* kv_F13 */		"F13",
    /* kv_F14 */		"F14",
    /* kv_F15 */		"F15",
    /* kv_F16 */		"F16",
    /* kv_F17 */		"F17",
    /* kv_F18 */		"F18",
    /* kv_F19 */		"F19",
    /* kv_F20 */		"F20",
    /* kv_F21 */		"F21",
    /* kv_F22 */		"F22",
    /* kv_F23 */		"F23",
    /* kv_F24 */		"F24",
    /* kv_F25 */		"F25",
    /* kv_F26 */		"F26",
    /* kv_F27 */		"F27",
    /* kv_F28 */		"F28",
    /* kv_F29 */		"F29",
    /* kv_F30 */		"F30",
    /* kv_F31 */		"F31",
    /* kv_F32 */		"F32",
    /* kv_F33 */		"F33",
    /* kv_F34 */		"F34",
    /* kv_F35 */		"F35",
    /* kv_F36 */		"F36",
    /* kv_F37 */		"F37",
    /* kv_F38 */		"F38",
    /* kv_F39 */		"F39",
    /* kv_F40 */		"F40",
    /* kv_F41 */		"F41",
    /* kv_F42 */		"F42",
    /* kv_F43 */		"F43",
    /* kv_F44 */		"F44",
    /* kv_F45 */		"F45",
    /* kv_F46 */		"F46",
    /* kv_F47 */		"F47",
    /* kv_F48 */		"F48",
    /* kv_F49 */		"F49",
    /* kv_F50 */		"F50",
    /* kv_F51 */		"F51",
    /* kv_F52 */		"F52",
    /* kv_F53 */		"F53",
    /* kv_F54 */		"F54",
    /* kv_F55 */		"F55",
    /* kv_F56 */		"F56",
    /* kv_F57 */		"F57",
    /* kv_F58 */		"F58",
    /* kv_F59 */		"F59",
    /* kv_F60 */		"F60",
    /* kv_F61 */		"F61",
    /* kv_F62 */		"F62",
    /* kv_F63 */		"F63",
    /* kv_Find */		"Find",
    /* kv_Help */		"Help",
    /* kv_Home */		"Home",
    /* kv_Insert */		"Insert",
    /* kv_InsertLine */		"InsertLine",
    /* kv_Left */		"Left",
    /* kv_Mark */		"Mark",
    /* kv_Message */		"Message",
    /* kv_Mouse */		"Mouse",
    /* kv_Move */		"Move",
    /* kv_Next */		"Next",
    /* kv_Open */		"Open",
    /* kv_Options */		"Options",
    /* kv_PageDown */		"PageDown",
    /* kv_PageUp */		"PageUp",
    /* kv_Previous */		"Previous",
    /* kv_Print */		"Print",
    /* kv_Redo */		"Redo",
    /* kv_Reference */		"Reference",
    /* kv_Refresh */		"Refresh",
    /* kv_Replace */		"Replace",
    /* kv_Restart */		"Restart",
    /* kv_Resume */		"Resume",
    /* kv_Right */		"Right",
    /* kv_Save */		"Save",
    /* kv_Select */		"Select",
    /* kv_ShiftBegin */		"ShiftBegin",
    /* kv_ShiftCancel */	"ShiftCancel",
    /* kv_ShiftCommand */	"ShiftCommand",
    /* kv_ShiftCopy */		"ShiftCopy",
    /* kv_ShiftCreate */	"ShiftCreate",
    /* kv_ShiftDelete */	"ShiftDelete",
    /* kv_ShiftDeleteLine */	"ShiftDeleteLine",
    /* kv_ShiftEnd */		"ShiftEnd",
    /* kv_ShiftEndOfLine */	"ShiftEndOfLine",
    /* kv_ShiftExit */		"ShiftExit",
    /* kv_ShiftFind */		"ShiftFind",
    /* kv_ShiftHelp */		"ShiftHelp",
    /* kv_ShiftHome */		"ShiftHome",
    /* kv_ShiftInsert */	"ShiftInsert",
    /* kv_ShiftLeft */		"ShiftLeft",
    /* kv_ShiftMessage */	"ShiftMessage",
    /* kv_ShiftMove */		"ShiftMove",
    /* kv_ShiftNext */		"ShiftNext",
    /* kv_ShiftOptions */	"ShiftOptions",
    /* kv_ShiftPrevious */	"ShiftPrevious",
    /* kv_ShiftPrint */		"ShiftPrint",
    /* kv_ShiftRedo */		"ShiftRedo",
    /* kv_ShiftReplace */	"ShiftReplace",
    /* kv_ShiftResume */	"ShiftResume",
    /* kv_ShiftRight */		"ShiftRight",
    /* kv_ShiftSave */		"ShiftSave",
    /* kv_ShiftSuspend */	"ShiftSuspend",
    /* kv_ShiftTab */		"ShiftTab",
    /* kv_ShiftUndo */		"ShiftUndo",
    /* kv_Suspend */		"Suspend",
    /* kv_Undo */		"Undo",
    /* kv_Up */			"Up",
    /* kv_UpLeft */		"UpLeft",
    /* kv_UpRight */		"UpRight"
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

