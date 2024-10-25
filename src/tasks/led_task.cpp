#include "pico/stdlib.h"
#include <iostream>

#include "tasks/led_task.h"
#include "utils/encode_data.h"

#include "FreeRTOS.h"
#include "task.h"

using namespace std;

void ledTask(void *pvParameters) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(800));

        string test = encodeData(15);

        cout << test << endl;
    }
}