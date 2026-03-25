
#include <stdint.h>
#include <sys/types.h>


void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

const uint8_t* st7789_get_char(char c);

void st7789_draw_char(uint16_t x, uint16_t y, uint16_t color);

void st7789_draw_string(uint16_t x, uint16_t y, uint16_t color);


