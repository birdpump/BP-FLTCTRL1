# Add the executable
add_executable(${PROJECT_NAME}
        main.cpp
        tasks/led_task.cpp
        tasks/telemetry_radio.cpp
        utils/encode_data.cpp
        config/hw_config.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib
        include
)

# setup NanoPB
find_package(Nanopb REQUIRED)
nanopb_generate_cpp(TARGET proto config/proto/simple.proto)

# Link libraries
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_gpio
        hardware_spi
        hardware_timer
        hardware_pwm
        FreeRTOS-Kernel-Heap4
        RadioLib
        proto
        FreeRTOS+FAT+CLI
)

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} "0.1")

target_compile_options(${PROJECT_NAME} PUBLIC
        -Wall
        -Wextra
        -Wshadow
        -Wstack-usage=2048
        -fanalyzer
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
        # pico-sdk
        #PARAM_ASSERTIONS_ENABLE_ALL=1
        PICO_USE_STACK_GUARDS=1
        PICO_MALLOC_PANIC=1
        PICO_MAX_SHARED_IRQ_HANDLERS=6
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        USE_PRINTF
)

set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS
        "-Wl,--print-memory-usage"
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})