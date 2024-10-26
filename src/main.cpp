#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <RadioLib.h>

#include "f_util.h"
#include "ff.h"
// #include "rtc.h" // TODO figure out what is using this header
#include "hw_config.h"

#include "tasks/led_task.h"
#include "utils/encode_data.h"
#include "utils/telemetry_radio.h"

using namespace std;

void test_SD() {
    sleep_ms(10000);

    puts("Hello, world!");

    FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (FR_OK != fr) {
        panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    // Open a file and write to it
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }

    // Close the file
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount("");
}

void setup() {
    // todo test radio
     sleep_ms(15000);
    //
     testRadio();
//    test_SD();

}

int main() {
    stdio_init_all();

    setup();

    if (xTaskCreate(ledTask, "led_task", 256, NULL, 1, NULL) != pdPASS) {
        printf("Failed to create LED task\n");
        while (1);
    }

    vTaskStartScheduler();

    printf("Scheduler failed to start\n");
    while (1);
}
