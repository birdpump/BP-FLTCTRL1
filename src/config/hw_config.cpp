/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

/*
This file should be tailored to match the hardware design.

See
https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration
*/

#include "hw_config.h"

/* Configuration of hardware SPI object */
static spi_t spi = {
    .hw_inst = spi0,  // SPI component
    .miso_gpio = 4,
    .mosi_gpio = 3,
    .sck_gpio = 2,    // GPIO number (not Pico pin number)

    .baud_rate = 150 * 1000 * 1000 / 6,
    // .baud_rate = 150 * 1000 * 1000 / 6  // 25000000 Hz, clk_div = 1.5
    //
    // .spi_mode = 0,
    // .no_miso_gpio_pull_up = false,
    // .set_drive_strength = false,
    // .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    // .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    // .use_static_dma_channels = false,
    // .tx_dma = 0,
    // .rx_dma = 0,
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
    .spi = &spi,  // Pointer to the SPI driving this card
    .ss_gpio = 5  // The SPI slave select GPIO for this SD card
};

/* Configuration of the SD Card socket object */
static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if  // Pointer to the SPI interface driving this card
};

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        // The number 0 is a valid SD card number.
        // Return a pointer to the sd_card object.
        return &sd_card;
    } else {
        // The number is invalid. Return @c NULL.
        return NULL;
    }
}

/* [] END OF FILE */