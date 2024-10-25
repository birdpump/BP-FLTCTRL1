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


void setup() {
    // todo test radio
    sleep_ms(5000);

    testRadio();

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
