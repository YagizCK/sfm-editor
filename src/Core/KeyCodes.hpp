/*
 * Copyright 2026 Yağız Cem Kocabıyık
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// The unknown key
#define SFM_KEY_UNKNOWN            -1

// Printable keys
#define SFM_KEY_SPACE              32
#define SFM_KEY_APOSTROPHE         39  /* ' */
#define SFM_KEY_COMMA              44  /* , */
#define SFM_KEY_MINUS              45  /* - */
#define SFM_KEY_PERIOD             46  /* . */
#define SFM_KEY_SLASH              47  /* / */
#define SFM_KEY_0                  48
#define SFM_KEY_1                  49
#define SFM_KEY_2                  50
#define SFM_KEY_3                  51
#define SFM_KEY_4                  52
#define SFM_KEY_5                  53
#define SFM_KEY_6                  54
#define SFM_KEY_7                  55
#define SFM_KEY_8                  56
#define SFM_KEY_9                  57
#define SFM_KEY_SEMICOLON          59  /* ; */
#define SFM_KEY_EQUAL              61  /* = */
#define SFM_KEY_A                  65
#define SFM_KEY_B                  66
#define SFM_KEY_C                  67
#define SFM_KEY_D                  68
#define SFM_KEY_E                  69
#define SFM_KEY_F                  70
#define SFM_KEY_G                  71
#define SFM_KEY_H                  72
#define SFM_KEY_I                  73
#define SFM_KEY_J                  74
#define SFM_KEY_K                  75
#define SFM_KEY_L                  76
#define SFM_KEY_M                  77
#define SFM_KEY_N                  78
#define SFM_KEY_O                  79
#define SFM_KEY_P                  80
#define SFM_KEY_Q                  81
#define SFM_KEY_R                  82
#define SFM_KEY_S                  83
#define SFM_KEY_T                  84
#define SFM_KEY_U                  85
#define SFM_KEY_V                  86
#define SFM_KEY_W                  87
#define SFM_KEY_X                  88
#define SFM_KEY_Y                  89
#define SFM_KEY_Z                  90
#define SFM_KEY_LEFT_BRACKET       91  /* [ */
#define SFM_KEY_BACKSLASH          92  /* \ */
#define SFM_KEY_RIGHT_BRACKET      93  /* ] */
#define SFM_KEY_GRAVE_ACCENT       96  /* ` */
#define SFM_KEY_WORLD_1            161 /* non-US #1 */
#define SFM_KEY_WORLD_2            162 /* non-US #2 */

// Function keys
#define SFM_KEY_ESCAPE             256
#define SFM_KEY_ENTER              257
#define SFM_KEY_TAB                258
#define SFM_KEY_BACKSPACE          259
#define SFM_KEY_INSERT             260
#define SFM_KEY_DELETE             261
#define SFM_KEY_RIGHT              262
#define SFM_KEY_LEFT               263
#define SFM_KEY_DOWN               264
#define SFM_KEY_UP                 265
#define SFM_KEY_PAGE_UP            266
#define SFM_KEY_PAGE_DOWN          267
#define SFM_KEY_HOME               268
#define SFM_KEY_END                269
#define SFM_KEY_CAPS_LOCK          280
#define SFM_KEY_SCROLL_LOCK        281
#define SFM_KEY_NUM_LOCK           282
#define SFM_KEY_PRINT_SCREEN       283
#define SFM_KEY_PAUSE              284
#define SFM_KEY_F1                 290
#define SFM_KEY_F2                 291
#define SFM_KEY_F3                 292
#define SFM_KEY_F4                 293
#define SFM_KEY_F5                 294
#define SFM_KEY_F6                 295
#define SFM_KEY_F7                 296
#define SFM_KEY_F8                 297
#define SFM_KEY_F9                 298
#define SFM_KEY_F10                299
#define SFM_KEY_F11                300
#define SFM_KEY_F12                301
#define SFM_KEY_F13                302
#define SFM_KEY_F14                303
#define SFM_KEY_F15                304
#define SFM_KEY_F16                305
#define SFM_KEY_F17                306
#define SFM_KEY_F18                307
#define SFM_KEY_F19                308
#define SFM_KEY_F20                309
#define SFM_KEY_F21                310
#define SFM_KEY_F22                311
#define SFM_KEY_F23                312
#define SFM_KEY_F24                313
#define SFM_KEY_F25                314

// Keypad keys
#define SFM_KEY_KP_0               320
#define SFM_KEY_KP_1               321
#define SFM_KEY_KP_2               322
#define SFM_KEY_KP_3               323
#define SFM_KEY_KP_4               324
#define SFM_KEY_KP_5               325
#define SFM_KEY_KP_6               326
#define SFM_KEY_KP_7               327
#define SFM_KEY_KP_8               328
#define SFM_KEY_KP_9               329
#define SFM_KEY_KP_DECIMAL         330
#define SFM_KEY_KP_DIVIDE          331
#define SFM_KEY_KP_MULTIPLY        332
#define SFM_KEY_KP_SUBTRACT        333
#define SFM_KEY_KP_ADD             334
#define SFM_KEY_KP_ENTER           335
#define SFM_KEY_KP_EQUAL           336

// Modifier keys
#define SFM_KEY_LEFT_SHIFT         340
#define SFM_KEY_LEFT_CONTROL       341
#define SFM_KEY_LEFT_ALT           342
#define SFM_KEY_LEFT_SUPER         343
#define SFM_KEY_RIGHT_SHIFT        344
#define SFM_KEY_RIGHT_CONTROL      345
#define SFM_KEY_RIGHT_ALT          346
#define SFM_KEY_RIGHT_SUPER        347
#define SFM_KEY_MENU               348

#define SFM_KEY_LAST               SFM_KEY_MENU

// Mouse buttons
#define SFM_MOUSE_BUTTON_1         0
#define SFM_MOUSE_BUTTON_2         1
#define SFM_MOUSE_BUTTON_3         2
#define SFM_MOUSE_BUTTON_4         3
#define SFM_MOUSE_BUTTON_5         4
#define SFM_MOUSE_BUTTON_6         5
#define SFM_MOUSE_BUTTON_7         6
#define SFM_MOUSE_BUTTON_8         7

#define SFM_MOUSE_BUTTON_LAST      SFM_MOUSE_BUTTON_8
#define SFM_MOUSE_BUTTON_LEFT      SFM_MOUSE_BUTTON_1
#define SFM_MOUSE_BUTTON_RIGHT     SFM_MOUSE_BUTTON_2
#define SFM_MOUSE_BUTTON_MIDDLE    SFM_MOUSE_BUTTON_3
