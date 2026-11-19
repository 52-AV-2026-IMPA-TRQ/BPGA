idf_component_register(
    SRCS
        "bpga_modbus_map.c"
        "bpga_modbus_master.c"

    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp-modbus
        esp_driver_uart
        esp_driver_gpio
        esp_timer
)
