cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(project_name)

pico_sdk_init()

add_executable(GPSwKalman
	main.c
	
)

#Add pico_stdlib library ,add more if used
target_link_libraries(GPSwKalman pico_stdlib hardware_uart) # hardware_i2c eklendi

#enable usb output, disable uart output
pico_enable_stdio_usb(GPSwKalman 1)
pico_enable_stdio_uart(GPSwKalman 1)

#Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(GPSwKalman)