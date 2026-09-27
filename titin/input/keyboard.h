#ifndef TITIN_KEYBOARD_H
#define TITIN_KEYBOARD_H

#define KEY_UP 0x01
#define KEY_DOWN 0x02
#define KEY_TAB 0x03
#define KEY_LEFT 0x04
#define KEY_RIGHT 0x05
#define KEY_VT3 0x06
#define KEY_NEW_TERMINAL 0x07
#define KEY_CLOSE_TERMINAL 0x08

char keyboard_poll(void);

char keyboard_read(void);

#endif
