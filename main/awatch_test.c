#include "fonts.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "st7789_init.h"
#include "st7789_draw.h"
#include <stdint.h>
#include <time.h>


void app_main(void)
{
    st7789_init_spi();     // SPI START
    st7789_backlight_on();
    st7789_init();

    st7789_fill_color(0xF800);

    st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(1000));
    st7789_fill_color(0x16fc);

   
    //draw_string(50, 50, "HELLO!", &Font_16x26, 0xFFFF, 0x0000);

    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    while(1)
    {
        // time(&now);
        // localtime_r(&now, &timeinfo);
        // strftime(strftime_buf, sizeof(strftime_buf), "%d-%m-%Y", &timeinfo);
        // draw_string(20, 80, strftime_buf, &Font_11x18, 0xFFFF, 0x16fc);

        // strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
        // draw_string(20, 110, strftime_buf, &Font_16x26, 0xFFFF, 0x001F);
        
        // vTaskDelay(pdMS_TO_TICKS(1000));


        draw_circle(120, 120, 50, 0xFFFF, 2);
    }
}


