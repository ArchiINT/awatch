#include "fonts.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "st7789_init.h"
#include "st7789_draw.h"
#include <stdint.h>
#include <time.h>
#include <style.c>
#include "driver/usb_serial_jtag.h"
#include "string.h"
#include <wifi_conn.h>
#include <esp_sntp.h>

#define BG_COLOR 0x3a08
#define TAG "Wifi"

#define SCREEN_HEIGHT 280
#define SCREEN_WIDTH  240

#define CENTER_X SCREEN_WIDTH / 2 // with string_center can be used for side position

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Ждём синхронизации (макс 10 секунд)
    int retry = 0;
    const int max_retry = 1;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < max_retry) {
        ESP_LOGI(TAG, "Waiting for NTP sync... (%d/%d)", retry + 1, max_retry);
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
    }

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "NTP sync OK");
    } else {
        ESP_LOGW(TAG, "NTP sync failed, time may be incorrect");
    }
}


void app_main(void)
{
    tutorial_init();
    tutorial_connect("Nonono", "Beginer2");
   
    st7789_init_spi();     // SPI START
    st7789_backlight_on();
    st7789_init();

    st7789_fill_color(0xF800);

    st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(1000));
    st7789_fill_color(BG_COLOR);

    //draw_string(50, 50, "HELLO!", &Font_16x26, 0xFFFF, 0x0000);
    draw_icon(&Battery, BG_COLOR, 0xF800);
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    initialize_sntp();

    while(1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%d-%m-%Y", &timeinfo);
        draw_string(string_center(SCREEN_WIDTH, 10, Font_11x18.FontWidth), 80, strftime_buf, &Font_11x18, 0xad55, BG_COLOR);

        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
        draw_string(string_center(SCREEN_WIDTH, 8, Font_16x26.FontWidth), 110, strftime_buf, &Font_16x26, 0xad55, BG_COLOR);
        
        vTaskDelay(pdMS_TO_TICKS(1000));


        //draw_circle(120, 120, 50, 0xFFFF, 2);
    }
}


