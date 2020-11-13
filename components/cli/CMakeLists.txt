idf_component_register(SRC_DIRS "."
                    INCLUDE_DIRS "include"
                    REQUIRES console spi_flash nvs_flash vfs fatfs i2c LedController uart mic)