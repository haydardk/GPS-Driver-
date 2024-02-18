add_executable(gps_pio_d1)

pico_generate_pio_header(gps_pio_d1 ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)
target_sources(gps_pio_d1 gps_pio_d1.c)

pico_enable_stdio_usb(gps_pio_d1 1)
pico_enable_stdio_uart(gps_pio_d1 1)

target_link_libraries(gps_pio_d1
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_uart
        )
pico_add_extra_outputs(gps_pio_d1)

