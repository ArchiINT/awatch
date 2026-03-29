#include <stdint.h>

void st7789_init_spi();

void st7789_reset();
void st7789_send_cmd(uint8_t cmd);
void st7789_send_data(const uint8_t *data, int len);

void st7789_init();
void st7789_backlight_on();

void st7789_fill_color(uint16_t color);
