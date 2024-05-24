#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "scd41_driver.h"
#include "as7262_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "nvs_flash.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "nvs_service.h"
#include "uart_commands.h" // Include the new uart_commands header
#include <stdio.h> // Include for printf

#define TAG "Main"

// Global device handles
i2c_master_dev_handle_t scd41_dev; // Global to access from uart_commands.c
i2c_master_dev_handle_t as7262_dev; // Global to access from uart_commands.c

// Initialize the console
void initialize_console() {
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
        .hint_color = atoi(LOG_COLOR_CYAN),
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    // Configure UART
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    // Initialize linenoise
    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(100);
    linenoiseAllowEmpty(false);
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);
    linenoiseHistoryLoad("/spiffs/console_history.txt");

    // Register commands
    register_commands();
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_service_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize console
    initialize_console();

    // Initialize I2C master bus
    i2c_master_bus_handle_t i2c_bus;
    ret = initialize_i2c_master(&i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        return;
    }

    // Add the SCD41 device on the I2C bus
    ret = scd41_init(i2c_bus, &scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SCD41 device: %s", esp_err_to_name(ret));
        return;
    }

    // Add the AS7262 device on the I2C bus
    ret = as7262_init(i2c_bus, &as7262_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AS7262 device: %s", esp_err_to_name(ret));
        return;
    }

    // Start periodic measurement for SCD41
    ret = scd41_start_periodic_measurement(scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start periodic measurement: %s", esp_err_to_name(ret));
        return;
    }

    // Start automatic self-calibration for SCD41
    ret = scd41_start_automatic_self_calibration(scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start automatic self-calibration: %s", esp_err_to_name(ret));
        return;
    }

    // Configure AS7262 integration time and start measurement
    ret = as7262_set_integration_time(as7262_dev, 50); // 50 * 2.8ms = 140ms
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AS7262 integration time: %s", esp_err_to_name(ret));
        return;
    }
    ret = as7262_start_measurement(as7262_dev, 2); // Continuous Measurement Mode
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start AS7262 measurement: %s", esp_err_to_name(ret));
        return;
    }

    // The device is now ready to accept UART commands
    printf("Device is ready. Type 'help' to see available commands.\n");

    // Main loop to handle console input
    while (true) {
        char* line = linenoise("> ");
        if (line != NULL) { // Check for NULL in case of error or EOF
            linenoiseHistoryAdd(line); // Add to command history
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err != ESP_OK) {
                printf("Error or unknown command: %s\n", esp_err_to_name(err));
            }
            linenoiseFree(line); // Free the memory allocated by linenoise
        }
    }
}
