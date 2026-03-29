#include "freertos/idf_additions.h"
#include <st7789_draw.h>
#include <fonts.h>
#include <st7789_init.h>
#include <stdint.h>
#include <sys/types.h>


// const uint8_t* st7789_get_char(char c){
//     if (c >= 0x20 && c <= 0x3f) {
//         return Font1[c - 0x20]; // Get index of Char
//     }
//     if (c >= 0x40 && c <= 0x5f) {
//         return Font2[c - 0x20];
//     }
//     if (c >= 0x60 && c <= 0x7f) {
//         return Font3[c - 0x20];
//     }
//     return Font1[0];
// }

const uint16_t* get_char(FontDef_t *font, char c) {
    if (c < 0x20 || c > 0x7F) {
        c = '?';
    }

    return &font->data[(c - 0x20) * font->FontHeight];
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color){
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t data[4];
    st7789_send_cmd(0x2A);
    // Ex: x is 2 bite (uint16), 300 = 0x012C; first bite = 01, second = 2C;
    // in binar = 012c = 0000 0001 | 0010 1100
    // >>8 get first bite
    // & 0xFF get second bite 0..1 0010 1100 & 0..0 1111 1111 = 00101100
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

void st7789_send_buffer(uint16_t x, uint16_t y, uint16_t ex, uint16_t ey, uint16_t *buffer, int len){
    uint8_t data[4];
    // X range
    st7789_send_cmd(0x2A);
    data[0] = x >> 8;
    data[1] = x & 0xFF;
    data[2] = ex >> 8;
    data[3] = ex & 0xFF;
    st7789_send_data(data, 4);

    // Y range
    st7789_send_cmd(0x2B);
    data[0] = y >> 8;
    data[1] = y & 0xFF;
    data[2] = ey >> 8;
    data[3] = ey & 0xFF;
    st7789_send_data(data, 4);

    // Memory write
    st7789_send_cmd(0x2C);
    st7789_send_data((uint8_t*)buffer, len * 2);

}


void draw_char(uint16_t x, uint16_t y, char c, FontDef_t *font, uint16_t color, uint16_t bg) {
    
    const uint16_t *ch = get_char(font, c);
    uint16_t buffer[26][16];
    //uint16_t swap_color = (color >> 8) | (color << 8);

    for (uint8_t row = 0; row < font->FontHeight; row++) {
        for (uint8_t col = 0; col < font->FontWidth; col++) {
            buffer[row][col] = ((ch[row] >> (15 - col)) & 0x01) ? color : bg;
        }
    }   

    st7789_send_buffer(x, y, x + font->FontWidth -1, y + font->FontHeight -1, (uint16_t*)buffer, font->FontHeight * font->FontWidth);

}
void draw_string(uint16_t x, uint16_t y, const char *str, FontDef_t *font, uint16_t color, uint16_t bg) {
    while (*str) {
        draw_char(x, y, *str, font, color, bg);
        x += font->FontWidth;
        str++;
    }
}
//void st7789_draw_char(uint16_t x, uint16_t y, uint16_t color){}

// void st7789_draw_string(uint16_t x, uint16_t y, uint16_t color){}

void draw_thick_pixel(uint16_t x, uint16_t y, uint16_t color, uint8_t size)
{ 
    for (int dx = -size; dx <= size; dx++) { 
        for (int dy = -size; dy <= size; dy++) { 
            st7789_draw_pixel(x + dx, y + dy, color); 
        } 
    } 
}

void draw_circle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t color, uint8_t thickness) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x) {

        // use thick pixels instead of single pixels
        draw_thick_pixel(xc + x, yc + y, color, thickness);
        draw_thick_pixel(xc - x, yc + y, color, thickness);
        draw_thick_pixel(xc + x, yc - y, color, thickness);
        draw_thick_pixel(xc - x, yc - y, color, thickness);

        draw_thick_pixel(xc + y, yc + x, color, thickness);
        draw_thick_pixel(xc - y, yc + x, color, thickness);
        draw_thick_pixel(xc + y, yc - x, color, thickness);
        draw_thick_pixel(xc - y, yc - x, color, thickness);

        x++;

        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }
}