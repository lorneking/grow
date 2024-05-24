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
#include "tds_sensor.h"

#define TAG "Main"

// Global device handles
i2c_master_dev_handle_t scd41_dev; // Global to access from uart_commands.c
i2c_master_dev_handle_t as7262_dev; // Global to access from uart_commands.c

void initialize_console() {
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
        .hint_color = atoi(LOG_COLOR_CYAN),
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

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

    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(100);
    linenoiseAllowEmpty(false);
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);
    linenoiseHistoryLoad("/spiffs/console_history.txt");

    register_commands();
}

void console_task(void *arg) {
    while (true) {
        char *line = linenoise("> ");
        if (line != NULL) {
            linenoiseHistoryAdd(line);
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err != ESP_OK) {
                printf("Error or unknown command: %s\n", esp_err_to_name(err));
            }
            linenoiseFree(line);
        }
    }
}

void scd41_task(void *arg) {
    // Start periodic measurement for SCD41
    esp_err_t ret = scd41_start_periodic_measurement(scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start periodic measurement: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    // Start automatic self-calibration for SCD41
    ret = scd41_start_automatic_self_calibration(scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start automatic self-calibration: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    while (true) {
        uint16_t co2;
        float temperature, humidity;
        ret = scd41_read_measurement(scd41_dev, &co2, &temperature, &humidity);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "SCD41 - CO2: %u ppm, Temperature: %.2f °C, Humidity: %.2f %%", co2, temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Error reading SCD41 measurement, code: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
    }
}

void as7262_task(void *arg) {
    // Load AS7262 calibration parameters
    as7262_calibration_t calibration;
    esp_err_t ret = load_as7262_calibration(&calibration);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load AS7262 calibration parameters: %s", esp_err_to_name(ret));
        for (int i = 0; i < 6; i++) {
            calibration.correction_factors[i] = 1.0f;
        }
    }

    // Configure AS7262 integration time and start measurement
    ret = as7262_set_integration_time(as7262_dev, 50); // 50 * 2.8ms = 140ms
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AS7262 integration time: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }
    ret = as7262_start_measurement(as7262_dev, 2); // Continuous Measurement Mode
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start AS7262 measurement: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    while (true) {
        uint16_t raw_channels[6];
        ret = as7262_read_measurement(as7262_dev, raw_channels);
        if (ret == ESP_OK) {
            float calibrated_data[6];
            for (int i = 0; i < 6; i++) {
                calibrated_data[i] = (float)raw_channels[i];
            }
            apply_correction_factors(calibrated_data);
            ESP_LOGI(TAG, "AS7262 - Corrected: V=%.2f, B=%.2f, G=%.2f, Y=%.2f, O=%.2f, R=%.2f",
                     calibrated_data[0], calibrated_data[1], calibrated_data[2],
                     calibrated_data[3], calibrated_data[4], calibrated_data[5]);
        } else {
            ESP_LOGE(TAG, "Error reading AS7262 measurement, code: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
    }
}

void app_main(void) {
    esp_err_t ret = nvs_service_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return;
    }

    initialize_console();

    i2c_master_bus_handle_t i2c_bus;
    ret = initialize_i2c_master(&i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        return;
    }

    ret = scd41_init(i2c_bus, &scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SCD41 device: %s", esp_err_to_name(ret));
        return;
    }

    ret = as7262_init(i2c_bus, &as7262_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AS7262 device: %s", esp_err_to_name(ret));
        return;
    }

    ret = tds_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TDS sensor: %s", esp_err_to_name(ret));
        return;
    }

    printf("Device is ready. Type 'help' to see available commands.\n");

    xTaskCreate(console_task, "console_task", 4096, NULL, 5, NULL);
    xTaskCreate(scd41_task, "scd41_task", 4096, NULL, 5, NULL);
    xTaskCreate(as7262_task, "as7262_task", 4096, NULL, 5, NULL);
}
