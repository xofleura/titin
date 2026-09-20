#include <stdint.h>
#include "keyboard.h"

static const char keyboard_map[128] = {
    [2] = '1',
    [3] = '2',
    [4] = '3',
    [5] = '4',
    [6] = '5',
    [7] = '6',
    [8] = '7',
    [9] = '8',
    [10] = '9',
    [11] = '0',
    [12] = '-',
    [13] = '=',
    [14] = '\b',
    [15] = '\t',
    [16] = 'q',
    [17] = 'w',
    [18] = 'e',
    [19] = 'r',
    [20] = 't',
    [21] = 'y',
    [22] = 'u',
    [23] = 'i',
    [24] = 'o',
    [25] = 'p',
    [26] = '[',
    [27] = ']',
    [28] = '\n',
    [30] = 'a',
    [31] = 's',
    [32] = 'd',
    [33] = 'f',
    [34] = 'g',
    [35] = 'h',
    [36] = 'j',
    [37] = 'k',
    [38] = 'l',
    [39] = ';',
    [40] = '\'',
    [41] = '`',
    [43] = '\\',
    [44] = 'z',
    [45] = 'x',
    [46] = 'c',
    [47] = 'v',
    [48] = 'b',
    [49] = 'n',
    [50] = 'm',
    [51] = ',',
    [52] = '.',
    [53] = '/',
    [57] = ' '
};

static const char keyboard_shift_map[128] = {
    [2] = '!',
    [3] = '@',
    [4] = '#',
    [5] = '$',
    [6] = '%',
    [7] = '^',
    [8] = '&',
    [9] = '*',
    [10] = '(',
    [11] = ')',
    [12] = '_',
    [13] = '+',
    [14] = '\b',
    [15] = '\t',
    [16] = 'Q',
    [17] = 'W',
    [18] = 'E',
    [19] = 'R',
    [20] = 'T',
    [21] = 'Y',
    [22] = 'U',
    [23] = 'I',
    [24] = 'O',
    [25] = 'P',
    [26] = '{',
    [27] = '}',
    [28] = '\n',
    [30] = 'A',
    [31] = 'S',
    [32] = 'D',
    [33] = 'F',
    [34] = 'G',
    [35] = 'H',
    [36] = 'J',
    [37] = 'K',
    [38] = 'L',
    [39] = ':',
    [40] = '"',
    [41] = '~',
    [43] = '|',
    [44] = 'Z',
    [45] = 'X',
    [46] = 'C',
    [47] = 'V',
    [48] = 'B',
    [49] = 'N',
    [50] = 'M',
    [51] = '<',
    [52] = '>',
    [53] = '?',
    [57] = ' '
};

static uint8_t keyboard_scancode(void)
{
    uint8_t status;
    uint8_t value;

    do {
        __asm__ volatile (
            "inb $0x64, %0"
            : "=a"(status)
        );
    } while (!(status & 1));

    __asm__ volatile (
        "inb $0x60, %0"
        : "=a"(value)
    );

    return value;
}

char keyboard_read(void)
{
    static int shift = 0;
    static int ctrl = 0;
    static int caps_lock = 0;

    for (;;) {
        uint8_t scancode = keyboard_scancode();

        if (scancode == 0xE0) {
            uint8_t extended = keyboard_scancode();

            if (extended == 0x48) {
                return KEY_UP;
            }

            if (extended == 0x50) {
                return KEY_DOWN;
            }

            if (extended == 0x4B) {
                return KEY_LEFT;
            }

            if (extended == 0x4D) {
                return KEY_RIGHT;
            }

            continue;
        }

        if (scancode == 42 || scancode == 54) {
            shift = 1;
            continue;
        }

        if (scancode == 170 || scancode == 182) {
            shift = 0;
            continue;
        }

        if (scancode == 29) {
            ctrl = 1;
            continue;
        }

        if (scancode == 157) {
            ctrl = 0;
            continue;
        }

        if (scancode == 58) {
            caps_lock = !caps_lock;
            continue;
        }

        if (scancode & 0x80) {
            continue;
        }

        if (scancode >= 128) {
            continue;
        }

        char normal = keyboard_map[scancode];

        if (normal == 0) {
            continue;
        }

        if (ctrl && (normal == 'l' || normal == 'L')) {
            return '\f';
        }

        if (normal == '\t') {
            return KEY_TAB;
        }

        if (normal >= 'a' && normal <= 'z') {
            if (shift ^ caps_lock) {
                return keyboard_shift_map[scancode];
            }

            return normal;
        }

        if (shift) {
            return keyboard_shift_map[scancode];
        }

        return normal;
    }
}
