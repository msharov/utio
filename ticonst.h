// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

/// Terminal I/O Library namespace.
namespace utio {
using namespace ustl;

/// Text color and background values.
enum EColor {
    black,
    red,
    green,
    brown,
    blue,
    magenta,
    cyan,
    lightgray,
    darkgray,
    lightred,
    lightgreen,
    yellow,
    lightblue,
    pink,
    lightcyan,
    white,
    color_Preserve,
    color_Last = color_Preserve
};

// Ugly code to tell the compiler that color cycling REALLY IS __OK__!!!!!
inline EColor operator+= (EColor& c, int v)
{
    const uint32_t result = (static_cast<uint32_t>(c) + v) % color_Last;
    return c = static_cast<EColor>(result);
}
inline EColor operator-= (EColor& c, int v)
{
    const uint32_t result = (static_cast<uint32_t>(c) - v) % color_Last;
    return c = static_cast<EColor>(result);
}

/// Text attributes for terminals.
enum EAttribute {
    a_standout,
    a_underline,
    a_reverse,
    a_blink,
    a_halfbright,
    a_bold,
    a_invisible,
    a_protect,
    a_altcharset,
    a_italic,
    a_subscript,
    a_superscript,
    attr_Last
};

//----------------------------------------------------------------------

struct SCharCell {
    wchar_t	c;	///< The character.
    uint8_t	fg;	///< Foreground color. See #EColor for values.
    uint8_t	bg;	///< Background color. See #EColor for values.
    uint16_t	attrs;	///< Attribute bits. See #EAttribute for values.
};

/// Structure for character images
class CCharCell : public SCharCell {
public:
    using rcself_t	= const CCharCell&;
public:
    inline	CCharCell (wchar_t nv = ' ', EColor nfg = lightgray, EColor nbg = color_Preserve, uint16_t nattrs = 0)
		    { c = nv; fg = nfg; bg = nbg; attrs = nattrs; }
    inline	CCharCell (const SCharCell& sc);
    inline	CCharCell (wchar_t v, rcself_t t);
    inline bool	EqualFormat (rcself_t v) const
		    { return *noalias_cast<const uint32_t*>(&fg) == *noalias_cast<const uint32_t*>(&v.fg); }
    inline bool	operator== (rcself_t v) const;
    inline void	operator= (rcself_t v);
    inline bool	HasAttr (EAttribute a) const	{ return attrs & (1 << a); }
    inline void	SetAttr (EAttribute a)		{ attrs |= (1 << a); }
    inline void	ClearAttr (EAttribute a)	{ attrs &= ~(1 << a); }
};

//{{{ CCharCell inlines implementation

// These are required to tell the compiler that the fields ARE being changed.
// The changes are not visible because the specializations below cast to long
// and operate on those for efficiency.
#define TOUCH_CHARCELL_VALUES_R(v)	asm(""::"m"(v.c),"m"(v.fg),"m"(v.bg),"m"(v.attrs))
#define TOUCH_CHARCELL_VALUES_W(v)	asm("":"=m"(v.c),"=m"(v.fg),"=m"(v.bg),"=m"(v.attrs))

/// Returns true if equal to \p v.
inline bool CCharCell::operator== (rcself_t v) const
{
    TOUCH_CHARCELL_VALUES_R(v);
    TOUCH_CHARCELL_VALUES_R((*this));
    // Optimize to avoid word and byte comparisons, since it is basically a whole-object compare
    // Original: return (c == v.c) & EqualFormat (v);
    auto lps = noalias_cast<const u_long*>(&v.c);
    auto lpd = noalias_cast<const u_long*>(&c);
    bool bEqual = true;
    for (uoff_t i = 0; i < sizeof(CCharCell)/sizeof(u_long); ++i)
	bEqual &= (lpd[i] == lps[i]);
    return bEqual;
}

/// Assigns \p v to self.
inline void CCharCell::operator= (rcself_t v)
{
    TOUCH_CHARCELL_VALUES_R(v);
    // Optimize to copy more than one item at once.
    auto lps = noalias_cast<const u_long*>(&v.c);
    auto lpd = noalias_cast<u_long*>(&c);
    for (uoff_t i = 0; i < sizeof(CCharCell)/sizeof(u_long); ++i)
	lpd[i] = lps[i];
    TOUCH_CHARCELL_VALUES_W((*this));
}

/// Creates a char \p v cell with other fields from \p t.
inline CCharCell::CCharCell (wchar_t v, rcself_t t)
{
    operator= (t);
    c = v;
}

/// Creates a char cell from static description \p sc.
inline CCharCell::CCharCell (const SCharCell& sc)
{
    TOUCH_CHARCELL_VALUES_R(sc);
    operator= (*noalias_cast<const CCharCell*>(&sc));
}

#undef TOUCH_CHARCELL_VALUES_R
#undef TOUCH_CHARCELL_VALUES_W

//}}}-------------------------------------------------------------------

//{{{ Standard graphic characters supported by some terminals.
enum EGraphicChar {
    acs_PoundSign,
    acs_DownArrow,
    acs_LeftArrow,
    acs_RightArrow,
    acs_UpArrow,
    acs_Board,
    acs_Bullet,
    acs_Checkerboard,
    acs_Degree,
    acs_Diamond,
    acs_GreaterEqual,
    acs_Pi,
    acs_HLine,
    acs_Lantern,
    acs_Plus,
    acs_LessEqual,
    acs_LowerLeftCorner,
    acs_LowerRightCorner,
    acs_NotEqual,
    acs_PlusMinus,
    acs_Scanline1,
    acs_Scanline3,
    acs_Scanline7,
    acs_Scanline9,
    acs_Block,
    acs_TopTee,
    acs_RightTee,
    acs_LeftTee,
    acs_BottomTee,
    acs_UpperLeftCorner,
    acs_UpperRightCorner,
    acs_VLine,
    acs_Last
};
//}}}

//{{{ Unicode wchar_t values for EGraphicChar constants.
enum EGraphicCharValue {
    acsv_PoundSign		= 0x00A3,
    acsv_DownArrow		= 0x2193,
    acsv_LeftArrow		= 0x2190,
    acsv_RightArrow		= 0x2192,
    acsv_UpArrow		= 0x2191,
    acsv_Board			= 0x2593,
    acsv_Bullet			= 0x2022,
    acsv_Checkerboard		= 0x2592,
    acsv_Degree			= 0x00B0,
    acsv_Diamond		= 0x25C6,
    acsv_GreaterEqual		= 0x2265,
    acsv_Pi			= 0x03C0,
    acsv_HLine			= 0x2500,
    acsv_Lantern		= 0x2609,
    acsv_Plus			= 0x253C,
    acsv_LessEqual		= 0x2266,
    acsv_LowerLeftCorner	= 0x250C,
    acsv_LowerRightCorner	= 0x2510,
    acsv_NotEqual		= 0x2260,
    acsv_PlusMinus		= 0x00B1,
    acsv_Scanline1		= 0x23BA,
    acsv_Scanline3		= 0x23BB,
    acsv_Scanline7		= 0x23BC,
    acsv_Scanline9		= 0x23BD,
    acsv_Block			= 0x2588,
    acsv_TopTee			= 0x252C,
    acsv_RightTee		= 0x2524,
    acsv_LeftTee		= 0x251C,
    acsv_BottomTee		= 0x2534,
    acsv_UpperLeftCorner	= 0x2514,
    acsv_UpperRightCorner	= 0x2518,
    acsv_VLine			= 0x2502
};
//}}}

//{{{ Terminfo constant names ------------------------------------------
///
/// \brief Contains constants copied from term.h in enum form.
///
/// These constants are in a namespace to make them semi-private and to
/// prevent them from polluting the global namespace (if you are using
/// namespace utio) as they contain many commonplace words.
///
namespace ti {

//{{{2 Boolean caps in terminfo order ----------------------------------
enum EBooleans {
    auto_left_margin,
    auto_right_margin,
    no_esc_ctlc,
    ceol_standout_glitch,
    eat_newline_glitch,
    erase_overstrike,
    generic_type,
    hard_copy,
    has_meta_key,
    has_status_line,
    insert_null_glitch,
    memory_above,
    memory_below,
    move_insert_mode,
    move_standout_mode,
    over_strike,
    status_line_esc_ok,
    dest_tabs_magic_smso,
    tilde_glitch,
    transparent_underline,
    xon_xoff,
    needs_xon_xoff,
    prtr_silent,
    hard_cursor,
    non_rev_rmcup,
    no_pad_char,
    non_dest_scroll_region,
    can_change,
    back_color_erase,
    hue_lightness_saturation,
    col_addr_glitch,
    cr_cancels_micro_mode,
    has_print_wheel,
    row_addr_glitch,
    semi_auto_right_margin,
    cpi_changes_res,
    lpi_changes_res,
    backspaces_with_bs,
    crt_no_scrolling,
    no_correctly_working_cr,
    gnu_has_meta_key,
    linefeed_is_newline,
    has_hardware_tabs,
    return_does_clr_eol
};

//}}}2-----------------------------------------------------------------
//{{{2 Numeral caps in terminfo order

enum ENumbers {
    columns,
    init_tabs,
    lines,
    lines_of_memory,
    magic_cookie_glitch,
    padding_baud_rate,
    virtual_terminal,
    width_status_line,
    num_labels,
    label_height,
    label_width,
    max_attributes,
    maximum_windows,
    max_colors,
    max_pairs,
    no_color_video,
    buffer_capacity,
    dot_vert_spacing,
    dot_horz_spacing,
    max_micro_address,
    max_micro_jump,
    micro_col_size,
    micro_line_size,
    number_of_pins,
    output_res_char,
    output_res_line,
    output_res_horz_inch,
    output_res_vert_inch,
    print_rate,
    wide_char_size,
    buttons,
    bit_image_entwining,
    bit_image_type,
    magic_cookie_glitch_ul,
    carriage_return_delay,
    new_line_delay,
    backspace_delay,
    horizontal_tab_delay,
    number_of_function_keys,
    no_value = -1
};

//}}}2------------------------------------------------------------------
//{{{2 String caps in terminfo order

enum EStrings {
    back_tab,
    bell,
    carriage_return,
    change_scroll_region,
    clear_all_tabs,
    clear_screen,
    clr_eol,
    clr_eos,
    column_address,
    command_character,
    cursor_address,
    cursor_down,
    cursor_home,
    cursor_invisible,
    cursor_left,
    cursor_mem_address,
    cursor_normal,
    cursor_right,
    cursor_to_ll,
    cursor_up,
    cursor_visible,
    delete_character,
    delete_line,
    dis_status_line,
    down_half_line,
    enter_alt_charset_mode,
    enter_blink_mode,
    enter_bold_mode,
    enter_ca_mode,
    enter_delete_mode,
    enter_dim_mode,
    enter_insert_mode,
    enter_secure_mode,
    enter_protected_mode,
    enter_reverse_mode,
    enter_standout_mode,
    enter_underline_mode,
    erase_chars,
    exit_alt_charset_mode,
    exit_attribute_mode,
    exit_ca_mode,
    exit_delete_mode,
    exit_insert_mode,
    exit_standout_mode,
    exit_underline_mode,
    flash_screen,
    form_feed,
    from_status_line,
    init_1string,
    init_2string,
    init_3string,
    init_file,
    insert_character,
    insert_line,
    insert_padding,
    key_backspace,
    key_catab,
    key_clear,
    key_ctab,
    key_dc,
    key_dl,
    key_down,
    key_eic,
    key_eol,
    key_eos,
    key_f0,
    key_f1,
    key_f10,
    key_f2,
    key_f3,
    key_f4,
    key_f5,
    key_f6,
    key_f7,
    key_f8,
    key_f9,
    key_home,
    key_ic,
    key_il,
    key_left,
    key_ll,
    key_npage,
    key_ppage,
    key_right,
    key_sf,
    key_sr,
    key_stab,
    key_up,
    keypad_local,
    keypad_xmit,
    lab_f0,
    lab_f1,
    lab_f10,
    lab_f2,
    lab_f3,
    lab_f4,
    lab_f5,
    lab_f6,
    lab_f7,
    lab_f8,
    lab_f9,
    meta_off,
    meta_on,
    newline,
    pad_char,
    parm_dch,
    parm_delete_line,
    parm_down_cursor,
    parm_ich,
    parm_index,
    parm_insert_line,
    parm_left_cursor,
    parm_right_cursor,
    parm_rindex,
    parm_up_cursor,
    pkey_key,
    pkey_local,
    pkey_xmit,
    print_screen,
    prtr_off,
    prtr_on,
    repeat_char,
    reset_1string,
    reset_2string,
    reset_3string,
    reset_file,
    restore_cursor,
    row_address,
    save_cursor,
    scroll_forward,
    scroll_reverse,
    set_attributes,
    set_tab,
    set_window,
    tab,
    to_status_line,
    underline_char,
    up_half_line,
    init_prog,
    key_a1,
    key_a3,
    key_b2,
    key_c1,
    key_c3,
    prtr_non,
    char_padding,
    acs_chars,
    plab_norm,
    key_btab,
    enter_xon_mode,
    exit_xon_mode,
    enter_am_mode,
    exit_am_mode,
    xon_character,
    xoff_character,
    ena_acs,
    label_on,
    label_off,
    key_beg,
    key_cancel,
    key_close,
    key_command,
    key_copy,
    key_create,
    key_end,
    key_enter,
    key_exit,
    key_find,
    key_help,
    key_mark,
    key_message,
    key_move,
    key_next,
    key_open,
    key_options,
    key_previous,
    key_print,
    key_redo,
    key_reference,
    key_refresh,
    key_replace,
    key_restart,
    key_resume,
    key_save,
    key_suspend,
    key_undo,
    key_sbeg,
    key_scancel,
    key_scommand,
    key_scopy,
    key_screate,
    key_sdc,
    key_sdl,
    key_select,
    key_send,
    key_seol,
    key_sexit,
    key_sfind,
    key_shelp,
    key_shome,
    key_sic,
    key_sleft,
    key_smessage,
    key_smove,
    key_snext,
    key_soptions,
    key_sprevious,
    key_sprint,
    key_sredo,
    key_sreplace,
    key_sright,
    key_srsume,
    key_ssave,
    key_ssuspend,
    key_sundo,
    req_for_input,
    key_f11,
    key_f12,
    key_f13,
    key_f14,
    key_f15,
    key_f16,
    key_f17,
    key_f18,
    key_f19,
    key_f20,
    key_f21,
    key_f22,
    key_f23,
    key_f24,
    key_f25,
    key_f26,
    key_f27,
    key_f28,
    key_f29,
    key_f30,
    key_f31,
    key_f32,
    key_f33,
    key_f34,
    key_f35,
    key_f36,
    key_f37,
    key_f38,
    key_f39,
    key_f40,
    key_f41,
    key_f42,
    key_f43,
    key_f44,
    key_f45,
    key_f46,
    key_f47,
    key_f48,
    key_f49,
    key_f50,
    key_f51,
    key_f52,
    key_f53,
    key_f54,
    key_f55,
    key_f56,
    key_f57,
    key_f58,
    key_f59,
    key_f60,
    key_f61,
    key_f62,
    key_f63,
    clr_bol,
    clear_margins,
    set_left_margin,
    set_right_margin,
    label_format,
    set_clock,
    display_clock,
    remove_clock,
    create_window,
    goto_window,
    hangup,
    dial_phone,
    quick_dial,
    tone,
    pulse,
    flash_hook,
    fixed_pause,
    wait_tone,
    user0,
    user1,
    user2,
    user3,
    user4,
    user5,
    user6,
    user7,
    user8,
    user9,
    orig_pair,
    orig_colors,
    initialize_color,
    initialize_pair,
    set_color_pair,
    set_foreground,
    set_background,
    change_char_pitch,
    change_line_pitch,
    change_res_horz,
    change_res_vert,
    define_char,
    enter_doublewide_mode,
    enter_draft_quality,
    enter_italics_mode,
    enter_leftward_mode,
    enter_micro_mode,
    enter_near_letter_quality,
    enter_normal_quality,
    enter_shadow_mode,
    enter_subscript_mode,
    enter_superscript_mode,
    enter_upward_mode,
    exit_doublewide_mode,
    exit_italics_mode,
    exit_leftward_mode,
    exit_micro_mode,
    exit_shadow_mode,
    exit_subscript_mode,
    exit_superscript_mode,
    exit_upward_mode,
    micro_column_address,
    micro_down,
    micro_left,
    micro_right,
    micro_row_address,
    micro_up,
    order_of_pins,
    parm_down_micro,
    parm_left_micro,
    parm_right_micro,
    parm_up_micro,
    select_char_set,
    set_bottom_margin,
    set_bottom_margin_parm,
    set_left_margin_parm,
    set_right_margin_parm,
    set_top_margin,
    set_top_margin_parm,
    start_bit_image,
    start_char_set_def,
    stop_bit_image,
    stop_char_set_def,
    subscript_characters,
    superscript_characters,
    these_cause_cr,
    zero_motion,
    char_set_names,
    key_mouse,
    mouse_info,
    req_mouse_pos,
    get_mouse,
    set_a_foreground,
    set_a_background,
    pkey_plab,
    device_type,
    code_set_init,
    set0_des_seq,
    set1_des_seq,
    set2_des_seq,
    set3_des_seq,
    set_lr_margin,
    set_tb_margin,
    bit_image_repeat,
    bit_image_newline,
    bit_image_carriage_return,
    color_names,
    define_bit_image_region,
    end_bit_image_region,
    set_color_band,
    set_page_length,
    display_pc_char,
    enter_pc_charset_mode,
    exit_pc_charset_mode,
    enter_scancode_mode,
    exit_scancode_mode,
    pc_term_options,
    scancode_escape,
    alt_scancode_esc,
    enter_horizontal_hl_mode,
    enter_left_hl_mode,
    enter_low_hl_mode,
    enter_right_hl_mode,
    enter_top_hl_mode,
    enter_vertical_hl_mode,
    set_a_attributes,
    set_pglen_inch,
    termcap_init2,
    termcap_reset,
    linefeed_if_not_lf,
    backspace_if_not_bs,
    other_non_function_keys,
    arrow_key_map,
    acs_ulcorner,
    acs_llcorner,
    acs_urcorner,
    acs_lrcorner,
    acs_ltee,
    acs_rtee,
    acs_btee,
    acs_ttee,
    acs_hline,
    acs_vline,
    acs_plus,
    memory_lock,
    memory_unlock,
    box_chars_1
};
//}}}2
} // namespace ti
//}}}-------------------------------------------------------------------

//{{{ Codes returned by CKeyboard for special keys
enum EKeyDataValue {
    kv_Space = ' ',
    kv_Tab = '\t',
    kv_Enter = '\n',
    kv_First = 0xE000,		// 0xE000 - 0xF8FF private use unicode region
    kv_Esc = kv_First,
    kv_Backspace,
    kv_Backtab,
    kv_Begin,
    kv_CATab,
    kv_CTab,
    kv_Cancel,
    kv_Center,
    kv_Clear,
    kv_ClearToEOL,
    kv_ClearToEOS,
    kv_Close,
    kv_Command,
    kv_Copy,
    kv_Create,
    kv_Delete,
    kv_DeleteLine,
    kv_Down,
    kv_DownLeft,
    kv_DownRight,
    kv_End,
    kv_Exit,
    kv_F0,
    kv_F1,
    kv_F2,
    kv_F3,
    kv_F4,
    kv_F5,
    kv_F6,
    kv_F7,
    kv_F8,
    kv_F9,
    kv_F10,
    kv_F11,
    kv_F12,
    kv_F13,
    kv_F14,
    kv_F15,
    kv_F16,
    kv_F17,
    kv_F18,
    kv_F19,
    kv_F20,
    kv_F21,
    kv_F22,
    kv_F23,
    kv_F24,
    kv_F25,
    kv_F26,
    kv_F27,
    kv_F28,
    kv_F29,
    kv_F30,
    kv_F31,
    kv_F32,
    kv_F33,
    kv_F34,
    kv_F35,
    kv_F36,
    kv_F37,
    kv_F38,
    kv_F39,
    kv_F40,
    kv_F41,
    kv_F42,
    kv_F43,
    kv_F44,
    kv_F45,
    kv_F46,
    kv_F47,
    kv_F48,
    kv_F49,
    kv_F50,
    kv_F51,
    kv_F52,
    kv_F53,
    kv_F54,
    kv_F55,
    kv_F56,
    kv_F57,
    kv_F58,
    kv_F59,
    kv_F60,
    kv_F61,
    kv_F62,
    kv_F63,
    kv_Find,
    kv_Help,
    kv_Home,
    kv_Insert,
    kv_InsertLine,
    kv_Left,
    kv_Mark,
    kv_Message,
    kv_Mouse,
    kv_Move,
    kv_Next,
    kv_Open,
    kv_Options,
    kv_PageDown,
    kv_PageUp,
    kv_Previous,
    kv_Print,
    kv_Redo,
    kv_Reference,
    kv_Refresh,
    kv_Replace,
    kv_Restart,
    kv_Resume,
    kv_Right,
    kv_Save,
    kv_Select,
    kv_ShiftBegin,
    kv_ShiftCancel,
    kv_ShiftCommand,
    kv_ShiftCopy,
    kv_ShiftCreate,
    kv_ShiftDelete,
    kv_ShiftDeleteLine,
    kv_ShiftEnd,
    kv_ShiftClearToEOL,
    kv_ShiftExit,
    kv_ShiftFind,
    kv_ShiftHelp,
    kv_ShiftHome,
    kv_ShiftInsert,
    kv_ShiftLeft,
    kv_ShiftMessage,
    kv_ShiftMove,
    kv_ShiftNext,
    kv_ShiftOptions,
    kv_ShiftPrevious,
    kv_ShiftPrint,
    kv_ShiftRedo,
    kv_ShiftReplace,
    kv_ShiftResume,
    kv_ShiftRight,
    kv_ShiftSave,
    kv_ShiftSuspend,
    kv_ShiftTab,
    kv_ShiftUndo,
    kv_Suspend,
    kv_Undo,
    kv_Up,
    kv_UpLeft,
    kv_UpRight,
    kv_KPEnter,
    kv_Last,
    kv_nKeys = (kv_Last - kv_First)
};
//}}}

//{{{ Meta key state constants
enum EMetaKeyBits {
    mksbit_Shift,
    mksbit_Alt,
    mksbit_Ctrl,
    mksbit_Meta,
    mksbit_Mouse,
    mksbit_NumLock,
    mksbit_CapsLock,
    mksbit_ScrollLock,
    mksbit_Last
};

enum EMetaKeyFlag {
    kf_MetaBit	= 24,
    kf_Shift	= (1 << (kf_MetaBit + mksbit_Shift)),
    kf_Alt	= (1 << (kf_MetaBit + mksbit_Alt)),
    kf_Ctrl	= (1 << (kf_MetaBit + mksbit_Ctrl)),
    kf_Meta	= (1 << (kf_MetaBit + mksbit_Meta)),
    kf_Mouse	= (1 << (kf_MetaBit + mksbit_Mouse)),
    kf_NumLock	= (1 << (kf_MetaBit + mksbit_NumLock)),
    kf_CapsLock	= (1 << (kf_MetaBit + mksbit_CapsLock)),
  kf_ScrollLock	= (1 << (kf_MetaBit + mksbit_ScrollLock))
};
//}}}

#define KV_MASK		((1 << ::utio::kf_MetaBit) - 1)
#define KF_MASK		(~KV_MASK)

} // namespace utio

CAST_STREAMABLE (utio::EColor, uint8_t)
CAST_STREAMABLE (utio::EAttribute, uint16_t)
CAST_STREAMABLE (utio::EGraphicChar, wchar_t)
CAST_STREAMABLE (utio::ti::EBooleans, uint16_t)
CAST_STREAMABLE (utio::ti::ENumbers, uint16_t)
CAST_STREAMABLE (utio::ti::EStrings, uint16_t)
CAST_STREAMABLE (utio::EKeyDataValue, uint16_t)
INTEGRAL_STREAMABLE (utio::CCharCell)
