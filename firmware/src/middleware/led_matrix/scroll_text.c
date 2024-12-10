#include "scroll_text.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Array of characters printable through scroll text */
static const uint8_t FONT_5X5[39][5] = {
    // A
    {
        0b11110,
        0b10001,
        0b11111,
        0b10001,
        0b10001,
    },
    // B
    {
        0b11110,
        0b10001,
        0b11110,
        0b10001,
        0b11111,
    },
    // C
    {
        0b11111,
        0b10000,
        0b10000,
        0b10000,
        0b11111,
    },
    // D
    {
        0b11110,
        0b10001,
        0b10001,
        0b10001,
        0b11110,
    },
    // E
    {
        0b11111,
        0b10000,
        0b11110,
        0b10000,
        0b11111,
    },
    // F
    {
        0b11111,
        0b10000,
        0b11100,
        0b10000,
        0b10000,
    },
    // G
    {
        0b11111,
        0b10000,
        0b10011,
        0b10001,
        0b11111,
    },
    // H
    {
        0b10001,
        0b10001,
        0b11111,
        0b10001,
        0b10001,
    },
    // I
    {
        0b01110,
        0b00100,
        0b00100,
        0b00100,
        0b01110,
    },
    // J
    {
        0b11111,
        0b00010,
        0b00010,
        0b10010,
        0b11110,
    },
    // K
    {
        0b10001,
        0b10010,
        0b11100,
        0b10010,
        0b10001,
    },
    // L
    {
        0b10000,
        0b10000,
        0b10000,
        0b10000,
        0b11111,
    },
    // M
    {
        0b10001,
        0b11011,
        0b10101,
        0b10001,
        0b10001,
    },
    // N
    {
        0b10001,
        0b11001,
        0b10101,
        0b10011,
        0b10001,
    },
    // O
    {
        0b11111,
        0b10001,
        0b10001,
        0b10001,
        0b11111,
    },
    // P
    {
        0b11111,
        0b10001,
        0b11111,
        0b10000,
        0b10000,
    },
    // Q
    {
        0b11110,
        0b10001,
        0b10001,
        0b10011,
        0b11111,
    },
    // R
    {
        0b11111,
        0b10001,
        0b11111,
        0b10100,
        0b10011,
    },
    // S
    {
        0b11111,
        0b10000,
        0b11111,
        0b00001,
        0b11111,
    },
    // T
    {
        0b11111,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
    },
    // U
    {
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b11111,
    },
    // V
    {
        0b10001,
        0b10001,
        0b10001,
        0b01010,
        0b00100,
    },
    // W
    {
        0b10001,
        0b10001,
        0b10101,
        0b11011,
        0b10001,
    },
    // X
    {
        0b10001,
        0b01010,
        0b00100,
        0b01010,
        0b10001,
    },
    // Y
    {
        0b10001,
        0b10001,
        0b11111,
        0b00100,
        0b00100,
    },
    // Z
    {
        0b11111,
        0b00010,
        0b00100,
        0b01000,
        0b11111,
    },
    // 0
    {
        0b11100,
        0b10100,
        0b10100,
        0b10100,
        0b11100,
    },
    // 1
    {
        0b01000,
        0b11000,
        0b01000,
        0b01000,
        0b11100,
    },
    // 2
    {
        0b11100,
        0b00100,
        0b11100,
        0b10000,
        0b11100,
    },
    // 3
    {
        0b11100,
        0b00100,
        0b11100,
        0b00100,
        0b11100,
    },
    // 4
    {
        0b10100,
        0b10100,
        0b11100,
        0b00100,
        0b00100,
    },
    // 5
    {
        0b11100,
        0b10000,
        0b11100,
        0b00100,
        0b11100,
    },
    // 6
    {
        0b11100,
        0b10000,
        0b11100,
        0b10100,
        0b11100,
    },
    // 7
    {
        0b11100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
    },
    // 8
    {
        0b11100,
        0b10100,
        0b11100,
        0b10100,
        0b11100,
    },
    // 9
    {
        0b11100,
        0b10100,
        0b11100,
        0b00100,
        0b00100,
    },
    // Space
    {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
    },
    // Exclamation Mark
    {
        0b01000,
        0b01000,
        0b01000,
        0b00000,
        0b01000,
    },
    // Hyphen
    {
        0b00000,
        0b00000,
        0b11111,
        0b00000,
        0b00000,
    },
};

// Function to map a character to its corresponding 5x5 matrix pattern
const uint8_t* get_char_5x5(char c) {
    if (c >= 'A' && c <= 'Z') {
        return FONT_5X5[c - 'A'];  // Return pattern for characters A-Z
    } else if (c >= '0' && c <= '9') {
        return FONT_5X5[c - '0' + 26];
    } else if (c == '!') {
        return FONT_5X5[37];  // Return pattern for exclamation mark character
    } else if (c == '-') {
        return FONT_5X5[38];  // Return pattern for hyphen character
    } else {  // Use space for space character and any unsupported character
        return FONT_5X5[36];  // Return pattern for space character
    }
}

// Function to generate a 7x7 matrix frame for the Charlieplex driver with
// centered 5x5 characters
void generate_frame(const char* text, size_t len, int scroll_position,
                    uint8_t frame[7][7]) {
    // Clear the frame
    memset(frame, 0, sizeof(uint8_t) * 7 * 7);

    // Loop through each column of the frame
    for (int col = 0; col < 7; ++col) {
        int char_index =
            (scroll_position + col) / 7;  // Determine which character to use
        int char_offset =
            (scroll_position + col) % 7;  // Column offset within the character

        if (char_index >= 0 && char_index < len) {
            // Get the corresponding 5x5 character pattern
            const uint8_t* char_pattern = get_char_5x5(text[char_index]);

            // Copy the appropriate column of the character pattern into the
            // frame, centered in the middle 5 rows of the 7x7 matrix (from row
            // 1 to row 5)
            if (char_offset < 5) {  // Only copy within the 5x5 bounds
                for (int row = 0; row < 5; ++row) {
                    if (char_pattern[row] & (1 << (4 - char_offset))) {
                        frame[row + 1][col] =
                            4;  // Center the character in the middle rows (1-5)
                    }
                }
            }
        }
    }
}

// Function to print the 7x7 matrix frame for visualization (debugging purpose)
void print_frame(uint8_t frame[7][7]) {
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            printf("%c",
                   frame[row][col] ? '#' : ' ');  // Print '#' for 1, ' ' for 0
        }
        printf("\n");
    }
}
