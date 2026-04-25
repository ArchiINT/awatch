idf_component_register(
    SRCS 
        "st7789_init.c"
        "st7789_draw.c"
        "fonts.c"
        "style.c"
        "wifi_conn.c"
        "icons.c"
        
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        esp_driver_gpio
        esp_driver_spi
        freertos
        hal
        esp_wifi
        nvs_flash
)
