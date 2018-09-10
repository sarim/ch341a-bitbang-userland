#include "ch341a.h"
#include <unistd.h>

int testblink() {
    int32_t ret;

    uint8_t gpio_dir_mask = 0b00111111;
    uint8_t gpio_data = 0;

    ret = ch341a_gpio_configure(&gpio_dir_mask);
    if (ret < 0) return -1;

    int i = 0;
    for (i = 0 ; i < 10 ; i++) {
        gpio_data ^= 1;
        usleep(200000);
        ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
        if (ret < 0) return -1;
        printf("data: %x\n", gpio_data);
    }
    return ret;
}

int write_bits(uint8_t *tx, uint8_t *rx, uint8_t len) {
    int32_t ret;

    uint8_t gpio_dir_mask = 0b00111111;
    uint8_t gpio_data = 0;

    ret = ch341a_gpio_configure(&gpio_dir_mask);
    if (ret < 0) return -1;

    // PIN 22 - D7 - MISO
    // PIN 21 - D6 -
    // PIN 20 - D5 - MOSI
    // PIN 19 - D4 -
    // PIN 18 - D3 - CLK
    // PIN 17 - D2 -
    // PIN 16 - D1 - 
    // PIN 15 - D0 - CS

    // lets put CLK to 0.
    gpio_data = 0b00000000;
    ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
    if (ret < 0) return -1;

    usleep(10 * 1000);

    // put CS to 1 and CLK to 0.
    gpio_data = 0b00000001;
    ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
    if (ret < 0) return -1;

    usleep(10 * 1000);

    //put CS to 0 and CLK to 0.
    gpio_data = 0b00000000;
    ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
    if (ret < 0) return -1;

    // wait 20ms for avr
    usleep(20 * 1000);

    // start writing and reading bits
    for (int p = 0; p < len; p++) {
        rx[p] = 0;
        for (int i = 7 ; i >= 0; i--) {
            uint8_t bit = (tx[p] >> i) & 1;
            uint8_t MISO = 0;


            // CLK HIGH
                if (bit) {
                    gpio_data = 0b00100000; // CLK to 0, MOSI to 1;
                } else {
                    gpio_data = 0b00000000; // CLK to 0, MOSI to 0;
                }
                ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
                if (ret < 0) return -1;

                gpio_data |= 0b00001000; // CLK to 1, keep MOSI;
                ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
                if (ret < 0) return -1;

                // reading MISO on raising edge of CLK
                MISO = ((gpio_data & 0x80) ? 1 : 0);


            //CLK to 0 and read MISO
                gpio_data &= ~0b00001000;
                ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
                if (ret < 0) return -1;
                
                // can also read MOSI at falling edge of CLK
                // MISO = ((gpio_data & 0x80) ? 1 : 0);

            rx[p] |= (MISO << i);
        }
    }

    //reset CS
    gpio_data = 0b00000001;
    ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
    if (ret < 0) return -1;

    return 0;
}

int testavr() {
    int32_t ret;

    uint8_t instructions[24] = {
        0b10101100,0b01010011,0b00000000,0b00000000, // programming enable, set avr in program mode
        0b00110000,0b00010000,0b00000000,0b00000000, // read avr signature at byte 00
        0b00110000,0b00010000,0b00000001,0b00000000, // read avr signature at byte 01
        0b00110000,0b00010000,0b00000010,0b00000000, // read avr signature at byte 10
        0b01010000,0b00000000,0b00000000,0b00000000, // read low fuse 
        0b01011000,0b00001000,0b00000000,0b00000000  // read high fuse
    }, replies[24];

    ret = write_bits(instructions, replies, 24);

    if (ret > -1)
    for (int i = 0 ; i < 24 ; i += 4) {
        printf ("%02x %02x %02x %02x\n", replies[i], replies[i+1], replies[i+2], replies[i+3]);
    }
    return ret;
}

int main() {
    int ret;

    ret = ch341a_configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
    if (ret < 0) return -1;

    // ret = testblink();
    ret = testavr();

    printf("\nreturn: %d\n", ret);
    return ch341a_release();
}