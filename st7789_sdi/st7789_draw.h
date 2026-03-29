#include "fonts.h"
#include <stdint.h>
#include <sys/types.h>


void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

const uint16_t* get_char(FontDef_t *font ,char c);

const uint8_t* st7789_get_char(char c);

void st7789_draw_char(uint16_t x, uint16_t y, uint16_t color);

void st7789_draw_string(uint16_t x, uint16_t y, uint16_t color);

void draw_thick_pixel(uint16_t x, uint16_t y, uint16_t color, uint8_t size);

void draw_circle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t color, uint8_t thickness);

void draw_char(uint16_t x, uint16_t y, char c, FontDef_t *font, uint16_t color, uint16_t bg);

void draw_string(uint16_t x, uint16_t y, const char *str, FontDef_t *font, uint16_t color, uint16_t bg);