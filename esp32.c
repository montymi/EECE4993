#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

// SPI configurations
#define SPI_HOST HSPI_HOST
#define SPI_DMA_CHAN 1
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Function to initialize SPI communication
void init_spi() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,           // Clock out at 1 MHz
        .mode = 0,                            // SPI mode 0
        .spics_io_num = PIN_NUM_CS,           // CS pin
        .queue_size = 1,                      // We want to be able to queue 1 transaction at a time
        .pre_cb = NULL,                       // Specify pre-transfer callback to handle CS setting
    };

    // Initialize the SPI bus
    spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CHAN);

    // Attach the Longan Nano board to the SPI bus
    spi_bus_add_device(SPI_HOST, &devcfg, NULL);
}

// Function to send data through SPI
void send_spi_data(const uint8_t *data, size_t len) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;         // Length is in bits, transaction length is in bytes
    t.tx_buffer = data;         // Data to send
    spi_device_transmit(SPI_HOST, &t);  // Transmit!
}

// Function to process user input
void process_input(const char *input) {
    // Implement your input processing logic here
    // For example, you can perform calculations or modify data based on user input
    // Here is just a simple example of echoing the input back
    printf("Received input: %s\n", input);
}

// Task to handle user input
void user_input_task(void *pvParameters) {
    char input_buffer[256];

    while (1) {
        // Wait for user input
        printf("Enter your input: ");
        fgets(input_buffer, sizeof(input_buffer), stdin);

        // Remove newline character from input
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        // Process the input
        process_input(input_buffer);

        // Send the processed data through SPI
        send_spi_data((uint8_t *)input_buffer, strlen(input_buffer));

        vTaskDelay(100 / portTICK_PERIOD_MS); // Delay to avoid busy-waiting
    }
}

void app_main(void) {
    // Initialize SPI communication
    init_spi();

    // Create a task to handle user input
    xTaskCreate(user_input_task, "user_input_task", 2048, NULL, 5, NULL);
}

