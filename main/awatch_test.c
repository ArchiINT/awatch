#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "st7789_init.h"
#include "st7789_draw.h"
#include <stdint.h>

void app_main(void)
{
    st7789_init_spi();     // SPI START
    st7789_backlight_on();
    st7789_init();

    vTaskDelay(pdMS_TO_TICKS(550));

        st7789_fill_color(0xF800);

    vTaskDelay(pdMS_TO_TICKS(350));
        st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(150));
        st7789_fill_color(0x001F);

    uint16_t x = 50;
    uint16_t y = 50;

    while (1)
    {
        st7789_draw_pixel(x, y, 0xFFFF);
        x++;
        y++;
        y++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
