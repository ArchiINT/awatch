#include <st7789_draw.h>
#include <font5x7.h>
#include <st7789_init.h>


const uint8_t* st7789_get_char(char c){
    if (c >= 0x20 && c <= 0x3f) {
        return Font1[c - 0x20]; // Get index of Char
    }
    if (c >= 0x40 && c <= 0x5f) {
        return Font2[c - 0x20];
    }
    if (c >= 0x60 && c <= 0x7f) {
        return Font3[c - 0x20];
    }
    return Font1[0];
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color){
    uint8_t data[4];
    st7789_send_cmd(0x2A);
    data[0] = x >> 8;
    data[1] = x & 0xFF;
    data[2] = x >> 8;
    data[3] = x & 0xFF;
    st7789_send_data(data, 4);

    st7789_send_cmd(0x2B);
    data[0] = y >> 8;
    data[1] = y & 0xFF;
    data[2] = y >> 8;
    data[3] = y & 0xFF;
    st7789_send_data(data, 4);

    st7789_send_cmd(0x2C); // Memory write

    uint16_t swap_color = (color >> 8) | (color << 8);
    st7789_send_data((uint8_t*)&swap_color, 2);

}

//void st7789_draw_char(uint16_t x, uint16_t y, uint16_t color){}

// void st7789_draw_string(uint16_t x, uint16_t y, uint16_t color){}

