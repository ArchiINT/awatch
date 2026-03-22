#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "portmacro.h"

#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 2 //SDA
#define PIN_NUM_CLK  1 // SCl

#define PIN_NUM_CS   5
#define PIN_NUM_DC   4
#define PIN_NUM_RST  3
#define PIN_NUM_BCKL 6

spi_device_handle_t spi;

void st7789_init_spi(){
    
    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240*280*2+8
    };
    
    spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 80 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .pre_cb = NULL,
    };
    
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
}

void st7789_reset(){
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void st7789_send_cmd(uint8_t cmd){
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };

    gpio_set_level(PIN_NUM_DC, 0);
    spi_device_transmit(spi, &t);
}

void st7789_send_data(const uint8_t *data, int len){
    spi_transaction_t t ={
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(PIN_NUM_DC, 1);
    spi_device_transmit(spi, &t);
}

void st7789_init(){
    st7789_reset();

    // Software reset
    st7789_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    st7789_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(150));

    // RGB565 (16 bit)
    st7789_send_cmd(0x3A);
    uint8_t data = 0x55;
    st7789_send_data(&data, 1);

    // Oriantation
    st7789_send_cmd(0x36);
    data = 0x00;
    st7789_send_data(&data, 1);

    // Inversion
    //st7789_send_cmd(0x21);

    // Normal display mode
    st7789_send_cmd(0x13);

    // Display on
    st7789_send_cmd(0x29);

    vTaskDelay(pdMS_TO_TICKS(100));
}

void st7789_backlight_on()
{
    gpio_set_level(PIN_NUM_BCKL, 1);
}

void st7789_fill_color(uint16_t color)
{
    uint8_t data[4];

    st7789_send_cmd(0x2A); // Column addr set
    data[0] = 0x00; data[1] = 0x00;
    data[2] = 0x00; data[3] = 0xEF;
    st7789_send_data(data, 4);

    st7789_send_cmd(0x2B); // Row addr set
    data[0] = 0x00; data[1] = 0x12;
    data[2] = 0x01; data[3] = 0x30; //280
    st7789_send_data(data, 4);

    st7789_send_cmd(0x2C); // Memory write

    uint16_t swap_color = (color >> 8) | (color << 8);

    uint16_t buffer[1024];

    for(uint16_t *b_ptr = buffer;b_ptr < buffer + 1024; b_ptr++){
        *b_ptr = swap_color;
    }
    int pixels = 304 * 240;
    while (pixels > 0) {
        int chunk = (pixels > 1024) ? 1024 : pixels;

        st7789_send_data((uint8_t*)buffer, chunk * 2);

        pixels -= chunk;

    }
}

void app_main(void)
{
    st7789_init_spi();     // SPI START
    st7789_backlight_on();
    st7789_init();
    while (1)
    {
    vTaskDelay(pdMS_TO_TICKS(150));

        st7789_fill_color(0xF800);

    vTaskDelay(pdMS_TO_TICKS(150));
        st7789_fill_color(0x07E0);
        
    vTaskDelay(pdMS_TO_TICKS(150));
        st7789_fill_color(0x001F);
    }
}
