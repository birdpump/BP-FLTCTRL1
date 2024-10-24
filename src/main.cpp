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

#include <pb_encode.h>
#include <pb_decode.h>
#include "simple.pb.h"

#include "tasks/led_task.h"

using namespace std;


void setup() {
    // Add any setup code here, if needed
}

int main() {
    stdio_init_all(); // Initialize standard I/O

    setup();

    // Create LED task
    if (xTaskCreate(ledTask, "led_task", 256, NULL, 1, NULL) != pdPASS) {
        printf("Failed to create LED task\n");
        while (1); // Halt if task creation failed
    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    printf("Scheduler failed to start\n");
    while (1);
}
