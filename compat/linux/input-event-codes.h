/* SPDX-License-Identifier: MIT */
/*
 * FreeBSD compatibility shim for linux/input-event-codes.h
 *
 * FreeBSD does not provide the Linux kernel input event codes.
 * This provides the minimal definitions needed for keyboard keycodes.
 *
 * Copyright (c) 2026 REVYTECH, Inc.
 */

#ifndef _LINUX_INPUT_EVENT_CODES_H
#define _LINUX_INPUT_EVENT_CODES_H

#include <stdint.h>

/* Mouse buttons - from linux/input-event-codes.h */
#define BTN_LEFT      0x110
#define BTN_RIGHT     0x111
#define BTN_MIDDLE    0x112
#define BTN_SIDE      0x113
#define BTN_EXTRA     0x114

/* Keycodes - Linux input event codes (evdev) */
/* These match the Linux kernel keycode values */

/* Misc keys */
#define KEY_RESERVED     0x00
#define KEY_ESC         0x01
#define KEY_LEFTMETA    0x7b
#define KEY_1           0x02
#define KEY_2           0x03
#define KEY_3           0x04
#define KEY_4           0x05
#define KEY_5           0x06
#define KEY_6           0x07
#define KEY_7           0x08
#define KEY_8           0x09
#define KEY_9           0x0a
#define KEY_0           0x0b
#define KEY_MINUS       0x0c
#define KEY_EQUAL       0x0d
#define KEY_BACKSPACE   0x0e
#define KEY_TAB         0x0f
#define KEY_Q           0x10
#define KEY_W           0x11
#define KEY_E           0x12
#define KEY_R           0x13
#define KEY_T           0x14
#define KEY_Y           0x15
#define KEY_U           0x16
#define KEY_I           0x17
#define KEY_O           0x18
#define KEY_P           0x19
#define KEY_LEFTBRACE   0x1a
#define KEY_RIGHTBRACE  0x1b
#define KEY_ENTER       0x1c
#define KEY_LEFTCTRL    0x1d
#define KEY_A           0x1e
#define KEY_S           0x1f
#define KEY_D           0x20
#define KEY_F           0x21
#define KEY_G           0x22
#define KEY_H           0x23
#define KEY_J           0x24
#define KEY_K           0x25
#define KEY_L           0x26
#define KEY_SEMICOLON   0x27
#define KEY_APOSTROPHE  0x28
#define KEY_GRAVE       0x29
#define KEY_LEFTSHIFT   0x2a
#define KEY_BACKSLASH   0x2b
#define KEY_Z           0x2c
#define KEY_X           0x2d
#define KEY_C           0x2e
#define KEY_V           0x2f
#define KEY_B           0x30
#define KEY_N           0x31
#define KEY_M           0x32
#define KEY_COMMA       0x33
#define KEY_DOT         0x34
#define KEY_SLASH       0x35
#define KEY_RIGHTSHIFT  0x36
#define KEY_KPASTERISK  0x37
#define KEY_LEFTALT     0x38
#define KEY_SPACE       0x39
#define KEY_CAPSLOCK    0x3a
#define KEY_F1          0x3b
#define KEY_F2          0x3c
#define KEY_F3          0x3d
#define KEY_F4          0x3e
#define KEY_F5          0x3f
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_NUMLOCK     0x45
#define KEY_SCROLLLOCK  0x46
#define KEY_KP7         0x47
#define KEY_KP8         0x48
#define KEY_KP9         0x49
#define KEY_KPMINUS     0x4a
#define KEY_KP4         0x4b
#define KEY_KP5         0x4c
#define KEY_KP6         0x4d
#define KEY_KPPLUS      0x4e
#define KEY_KP1         0x4f
#define KEY_KP2         0x50
#define KEY_KP3         0x51
#define KEY_KP0         0x52
#define KEY_KPDOT       0x53
#define KEY_ZENKAKUHANKAKU 0x57
#define KEY_102ND       0x56
#define KEY_F11         0x57
#define KEY_F12         0x58
#define KEY_RO          0x73
#define KEY_KATAKANAJISHIRO 0x76
#define KEY_HIRAGANA    0x77
#define KEY_HENKAN      0x79
#define KEY_KATAKANAHIRAGANA 0x70
#define KEY_MUHENKAN    0x7b
#define KEY_KPJPCOMMA   0x7e
#define KEY_KPENTER     0x9c
#define KEY_RIGHTCTRL   0x9d
#define KEY_KPSLASH     0xb5
#define KEY_SYSRQ       0xb7
#define KEY_RIGHTALT    0xb8
#define KEY_LINEFEED    0xc5
#define KEY_HOME        0xc7
#define KEY_UP          0xc8
#define KEY_PAGEUP      0xc9
#define KEY_LEFT        0xcb
#define KEY_RIGHT       0xcd
#define KEY_END         0xcf
#define KEY_DOWN        0xd0
#define KEY_PAGEDOWN    0xd1
#define KEY_INSERT      0xd2
#define KEY_DELETE      0xd3
#define KEY_MACRO       0xd4
#define KEY_MUTE        0xa0
#define KEY_VOLUMEDOWN  0xae
#define KEY_VOLUMEUP    0xb0
#define KEY_POWER       0xde
#define KEY_KPEQUAL     0xbd
#define KEY_KPPLUSMINUS 0xb1
#define KEY_PAUSE       0xc6
#define KEY_SCALE       0xdc
#define KEY_KPCOMMA     0x7e
#define KEY_HANGEUL     0xf2
#define KEY_HANGUEL     KEY_HANGEUL
#define KEY_HANJA       0xf3
#define KEY_YEN         0x7d
#define KEY_COMPOSE     0x7c
#define KEY_STOP        0x78
#define KEY_AGAIN       0x79
#define KEY_PROPS       0x7a
#define KEY_UNDO        0x7b
#define KEY_FRONT       0x7c
#define KEY_COPY        0x7d
#define KEY_OPEN        0x7e
#define KEY_PASTE       0x7f
#define KEY_FIND        0x80
#define KEY_CUT         0x81
#define KEY_HELP        0x82
#define KEY_MENU        0xdd
#define KEY_CALC        0xd9
#define KEY_SETUP       0xd7
#define KEY_SLEEP       0xdf
#define KEY_WAKEUP      0xe3
#define KEY_FILE        0xe6
#define KEY_SENDFILE    0xe9
#define KEY_DELETEFILE  0xea
#define KEY_XFER        0xeb
#define KEY_PROG1       0xec
#define KEY_PROG2       0xed
#define KEY_WWW         0xee
#define KEY_MSDOS       0xef
#define KEY_COFFEE      0xf0
#define KEY_DIRECTION    0xf5
#define KEY_CYCLEWINDOWS 0xf6
#define KEY_MAIL         0xea
#define KEY_BOOKMARKS   0xe6
#define KEY_COMPUTER    0xeb
#define KEY_BACK        0x9e
#define KEY_FORWARD     0xa0
#define KEY_CLOSECD     0xa1
#define KEY_EJECTCD     0xa2
#define KEY_EJECTCLOSECD 0xa2
#define KEY_NEXTSONG    0xaf
#define KEY_PLAYPAUSE   0xa4
#define KEY_PREVIOUSSONG 0xab
#define KEY_STOPCD      0xa5
#define KEY_RECORD      0xa7
#define KEY_REWIND      0xa8
#define KEY_PHONE       0x9f
#define KEY_ISO         0xa9
#define KEY_CONFIG      0xe1
#define KEY_HOMEPAGE    0xac
#define KEY_REFRESH     0xad
#define KEY_EXIT        0xad
#define KEY_MOVE        0xa8
#define KEY_EDIT        0xa7
#define KEY_SCROLLUP    0xa9
#define KEY_SCROLLDOWN  0xa6
#define KEY_KPLEFTPAREN 0xb6
#define KEY_KPRIGHTPAREN 0xb7
#define KEY_F13         0xd8
#define KEY_F14         0xd9
#define KEY_F15         0xda
#define KEY_F16         0xdb
#define KEY_F17         0xdc
#define KEY_F18         0xdd
#define KEY_F19         0xde
#define KEY_F20         0xdf
#define KEY_F21         0xe0
#define KEY_F22         0xe1
#define KEY_F23         0xe2
#define KEY_F24         0xe3

/* LED codes */
#define LED_NUML        0x00
#define LED_CAPSL       0x01
#define LED_SCROLLL     0x02
#define LED_COMPOSE     0x03
#define LED_KANA        0x04
#define LED_SLEEP       0x05
#define LED_SUSPEND     0x06
#define LED_MUTE        0x07
#define LED_MISC        0x08
#define LED_MAIL        0x09
#define LED_CHARGING    0x0a

#endif /* _LINUX_INPUT_EVENT_CODES_H */
