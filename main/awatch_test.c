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

    st7789_fill_color(0xF800);

    st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(1000));
    st7789_fill_color(0x001F);

    // draw_circle(120, 120, 50, 0xFFFF, 2);
    draw_string(50, 50, "HELLO!", &Font_16x26, 0xFFFF, 0x0000);
    while (1)
    {

    }
}
