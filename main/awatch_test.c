#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "st7789_init.h"
#include "st7789_draw.h"
#include <stdint.h>

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
        vTaskDelay(pdMS_TO_TICKS(20));

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

void app_main(void)
{
    st7789_init_spi();     // SPI START
    st7789_backlight_on();
    st7789_init();

    vTaskDelay(pdMS_TO_TICKS(1000));

        st7789_fill_color(0xF800);

    vTaskDelay(pdMS_TO_TICKS(1000));
        st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(1000));
        st7789_fill_color(0x001F);

    draw_circle(120, 120, 50, 0xFFFF, 2);     
    while (1)
    {
    }
}
