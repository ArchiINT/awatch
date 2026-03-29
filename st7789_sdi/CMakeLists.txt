idf_component_register(
    SRCS 
        "st7789_init.c"
        "st7789_draw.c"
        "font5x7.c"
        "fonts.c"
        
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        esp_driver_gpio
        esp_driver_spi
        freertos
        hal
)
