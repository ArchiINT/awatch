idf_component_register(SRCS "awatch_test.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        st7789_sdi
                        esp_system
                        heap)
