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
#include "uart_commands.h"
#include <stdio.h>
#include "tds_sensor.h"

#undef TAG
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

// Task to handle console input
void console_task(void *arg) {
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

// Task to initialize sensors
void sensor_init_task(void *arg) {
    esp_err_t ret;

    // Initialize I2C master bus
    i2c_master_bus_handle_t i2c_bus;
    ret = initialize_i2c_master(&i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    // Add the SCD41 device on the I2C bus
    ret = scd41_init(i2c_bus, &scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SCD41 device: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    // Add the AS7262 device on the I2C bus
    ret = as7262_init(i2c_bus, &as7262_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AS7262 device: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    // Initialize the TDS sensor ADC
    ret = initialize_tds_sensor();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TDS sensor ADC: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Sensor initialization complete.");
    vTaskDelete(NULL);
}

// Task to read SCD41 data
void read_scd41_task(void *arg) {
    while (true) {
        uint16_t co2;
        float temperature, humidity;
        esp_err_t ret = scd41_read_measurement(scd41_dev, &co2, &temperature, &humidity);
        if (ret == ESP_OK) {
            printf("SCD41 - CO2: %u ppm, Temperature: %.2f °C, Humidity: %.2f %%\n", co2, temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Error reading SCD41 measurement, code: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
}

// Task to read AS7262 data
void read_as7262_task(void *arg) {
    while (true) {
        uint16_t raw_channels[6];
        esp_err_t ret = as7262_read_measurement(as7262_dev, raw_channels);
        if (ret == ESP_OK) {
            float calibrated_data[6];
            for (int i = 0; i < 6; i++) {
                calibrated_data[i] = (float)raw_channels[i];
            }
            apply_correction_factors(calibrated_data);
            printf("AS7262 - Corrected: V=%.2f, B=%.2f, G=%.2f, Y=%.2f, O=%.2f, R=%.2f\n",
                   calibrated_data[0], calibrated_data[1], calibrated_data[2],
                   calibrated_data[3], calibrated_data[4], calibrated_data[5]);
        } else {
            ESP_LOGE(TAG, "Error reading AS7262 measurement, code: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
}

// Task to read TDS data
void read_tds_task(void *arg) {
    while (true) {
        float tds_value = read_tds_sensor();
        if (tds_value >= 0) {
            printf("TDS Value: %.2f ppm\n", tds_value);
        } else {
            ESP_LOGE(TAG, "Failed to read TDS value.");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
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

    // Create sensor initialization task
    xTaskCreate(sensor_init_task, "sensor_init_task", 4096, NULL, 5, NULL);

    // Delay to ensure sensors are initialized before starting the read tasks
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second

    // Create tasks for periodic sensor readings
    xTaskCreate(read_scd41_task, "read_scd41_task", 4096, NULL, 5, NULL);
    xTaskCreate(read_as7262_task, "read_as7262_task", 4096, NULL, 5, NULL);
    xTaskCreate(read_tds_task, "read_tds_task", 4096, NULL, 5, NULL);

    // Create console task
    xTaskCreate(console_task, "console_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "App main complete.");
}
