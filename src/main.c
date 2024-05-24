#include "driver/i2c_master.h"
#include "driver/uart.h"        // Include the UART driver header
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
#include "esp_vfs_dev.h"        // Include VFS driver for UART
#include "linenoise/linenoise.h" // Include linenoise for command line interface

#define TAG "Main"

// Global device handles
static i2c_master_dev_handle_t scd41_dev;
static i2c_master_dev_handle_t as7262_dev;

// Function to apply correction factors
void apply_correction_factors(float* data) {
    // Example correction factors
    float correction_factors[6] = {1.05, 0.98, 1.02, 1.00, 0.99, 1.01};
    for (int i = 0; i < 6; i++) {
        data[i] *= correction_factors[i];
    }
}

// Command handler for forced recalibration
int cmd_forced_recalibration(int argc, char **argv) {
    uint16_t target_co2 = 400; // Example target CO2 concentration
    esp_err_t ret = scd41_set_forced_recalibration(scd41_dev, target_co2);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Forced recalibration successful");
    } else {
        ESP_LOGE(TAG, "Failed to set forced recalibration: %s", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for reading SCD41 measurements
int cmd_read_scd41(int argc, char **argv) {
    uint16_t co2;
    float temperature, humidity;
    esp_err_t ret = scd41_read_measurement(scd41_dev, &co2, &temperature, &humidity);
    if (ret == ESP_OK) {
        printf("SCD41 - CO2: %d ppm, Temperature: %.2f °C, Humidity: %.2f %%\n", co2, temperature, humidity);
    } else {
        ESP_LOGE(TAG, "Error reading SCD41 measurement, code: %s", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for reading AS7262 measurements
int cmd_read_as7262(int argc, char **argv) {
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
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for help
int cmd_help(int argc, char **argv) {
    printf("Available commands:\n");
    printf("  help - Show this help message\n");
    printf("  frc - Trigger forced recalibration on the SCD41\n");
    printf("  read_scd41 - Read SCD41 sensor data\n");
    printf("  read_as7262 - Read AS7262 sensor data\n");
    return 0;
}

// Register console commands
void register_commands() {
    esp_console_cmd_t cmd = {
        .command = "help",
        .help = "Show this help message",
        .hint = NULL,
        .func = &cmd_help,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "frc",
        .help = "Trigger forced recalibration on the SCD41",
        .hint = NULL,
        .func = &cmd_forced_recalibration,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "read_scd41",
        .help = "Read SCD41 sensor data",
        .hint = NULL,
        .func = &cmd_read_scd41,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "read_as7262",
        .help = "Read AS7262 sensor data",
        .hint = NULL,
        .func = &cmd_read_as7262,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

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
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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
        if (line == NULL) { // Check for NULL in case of error or EOF
            continue;
        }
        linenoiseHistoryAdd(line); // Add to command history
        int ret = esp_console_run(line, &ret);
        if (ret != ESP_OK) {
            printf("Error or unknown command: %d\n", ret);
        }
        linenoiseFree(line); // Free the memory allocated by linenoise
    }
}
