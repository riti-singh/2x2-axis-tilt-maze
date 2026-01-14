#include "SevenSeg.h"
#include "i2c.h" // assumes you have i2c1_byteWrite and i2c1_memWrite functions

// lookup table for digits 0-9 and letters a-f (7-segment encoding)
static const uint8_t numbertable[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // a
    0x7C, // b
    0x39, // c
    0x5E, // d
    0x79, // e
    0x71  // f
};

// initialize ht16k33 display
void SevenSeg_init(void) {
    i2c1_byteWrite(HT16K33_ADDR, HT16K33_SYSTEM_CMD | HT16K33_OSC_ON); // turn on oscillator
    i2c1_byteWrite(HT16K33_ADDR, HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_ON | HT16K33_BLINK_OFF); // display on, no blink
    i2c1_byteWrite(HT16K33_ADDR, HT16K33_BRIGHT_CMD | 0x0F); // max brightness
}

// set display blink rate
void SevenSeg_blink(uint8_t rate) {
    i2c1_byteWrite(HT16K33_ADDR, HT16K33_DISPLAY_CMD | HT16K33_DISPLAY_ON | rate);
}

// set brightness (0-15)
void SevenSeg_dim(uint8_t brightness) {
    i2c1_byteWrite(HT16K33_ADDR, HT16K33_BRIGHT_CMD | (brightness & 0x0F));
}

// write 10 bytes (5 digits × 2 bytes) to ht16k33 ram
void SevenSeg_write(uint8_t *display_buffer) {
    i2c1_memWrite(HT16K33_ADDR, HT16K33_ADDR_PTR, 2 * HT16K33_NBUF, display_buffer);
}

// convert 0-9999 into digit buffer for display
void SevenSeg_number(uint16_t num, uint8_t *buffer) {
    if (num > 9999) {
        // show error "eeee"
        for (uint8_t i = 0; i < HT16K33_NBUF; i++) {
            if (i == 2) {
                buffer[2*i]     = 0x00; // skip colon position
                buffer[2*i + 1] = 0x00;
            } else {
                buffer[2*i]     = numbertable[0xE]; // e character
                buffer[2*i + 1] = 0x00;
            }
        }
        return;
    }

    // split number into digits
    buffer[0] = numbertable[num / 1000]; // thousands
    buffer[1] = 0x00;

    num %= 1000;
    buffer[2] = numbertable[num / 100]; // hundreds
    buffer[3] = 0x00;

    buffer[4] = 0x00; // skip colon
    buffer[5] = 0x00;

    num %= 100;
    buffer[6] = numbertable[num / 10]; // tens
    buffer[7] = 0x00;

    num %= 10;
    buffer[8] = numbertable[num]; // ones
    buffer[9] = 0x00;
}

void SevenSeg_write_number(uint16_t num) {
    uint8_t display_buffer[2 * HT16K33_NBUF];
    SevenSeg_number(num, display_buffer);
    SevenSeg_write(display_buffer);
}

void SevenSeg_writeHex(uint8_t val) {
    uint8_t display[10] = {0};  // full buffer, all cleared

    // Write digits to D1 and D2, skipping colon (RAM[4])
    display[2 * 1] = numbertable[(val >> 4) & 0x0F]; // high nibble → RAM[2]
    display[2 * 3] = numbertable[val & 0x0F];        // low nibble  → RAM[6]

    SevenSeg_write(display);
}

void SevenSeg_number_signed(int16_t num, uint8_t *buffer) {
    for (int i = 0; i < 10; i++) buffer[i] = 0x00;

    uint16_t abs_val = (num < 0) ? -num : num;

    uint8_t thousands = (abs_val / 1000) % 10;
    uint8_t hundreds  = (abs_val / 100) % 10;
    uint8_t tens      = (abs_val / 10) % 10;
    uint8_t ones      = abs_val % 10;

    if (num < 0) {
        // negative sign in leftmost digit, then values
        buffer[0] = 0x40; // minus sign
        buffer[2] = numbertable[hundreds];
        buffer[6] = numbertable[tens];
        buffer[8] = numbertable[ones];
    } else {
        if (abs_val >= 1000) {
            buffer[0] = numbertable[thousands];
            buffer[2] = numbertable[hundreds];
            buffer[6] = numbertable[tens];
            buffer[8] = numbertable[ones];
        } else if (abs_val >= 100) {
            buffer[2] = numbertable[hundreds];
            buffer[6] = numbertable[tens];
            buffer[8] = numbertable[ones];
        } else if (abs_val >= 10) {
            buffer[6] = numbertable[tens];
            buffer[8] = numbertable[ones];
        } else {
            buffer[8] = numbertable[ones];
        }
    }
}
