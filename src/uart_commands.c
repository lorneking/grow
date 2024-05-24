#include "uart_commands.h"
#include "nvs_service.h"
#include "scd41_driver.h"
#include "as7262_driver.h"
#include "esp_log.h"
#include "esp_console.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


#define TAG "UART_COMMANDS"
#define NVS_NAMESPACE "as7262_cal"

// Global device handles (extern to access from main.c)
extern i2c_master_dev_handle_t scd41_dev;
extern i2c_master_dev_handle_t as7262_dev;

static as7262_calibration_t calibration;

// Function to apply correction factors
void apply_correction_factors(float* data) {
    for (int i = 0; i < 6; i++) {
        data[i] *= calibration.correction_factors[i];
    }
}

// Save calibration parameters to NVS
esp_err_t save_as7262_calibration(const as7262_calibration_t* calibration) {
    esp_err_t ret = nvs_service_set_blob("calibration", calibration, sizeof(as7262_calibration_t));
    if (ret == ESP_OK) {
        nvs_service_commit();
    }
    return ret;
}

// Load calibration parameters from NVS
esp_err_t load_as7262_calibration(as7262_calibration_t* calibration) {
    size_t required_size = sizeof(as7262_calibration_t);
    return nvs_service_get_blob("calibration", calibration, &required_size);
}

// Command handler for setting AS7262 calibration parameters
int cmd_set_as7262_calibration(int argc, char **argv) {
    if (argc != 7) {
        printf("Usage: set_as7262_cal <V> <B> <G> <Y> <O> <R>\n");
        return 1;
    }
    for (int i = 0; i < 6; i++) {
        calibration.correction_factors[i] = atof(argv[i + 1]);
    }
    esp_err_t ret = save_as7262_calibration(&calibration);
    if (ret == ESP_OK) {
        printf("Calibration parameters saved successfully.\n");
    } else {
        printf("Failed to save calibration parameters: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for getting AS7262 calibration parameters
int cmd_get_as7262_calibration(int argc, char **argv) {
    esp_err_t ret = load_as7262_calibration(&calibration);
    if (ret == ESP_OK) {
        printf("Calibration parameters: V=%.2f, B=%.2f, G=%.2f, Y=%.2f, O=%.2f, R=%.2f\n",
               calibration.correction_factors[0], calibration.correction_factors[1],
               calibration.correction_factors[2], calibration.correction_factors[3],
               calibration.correction_factors[4], calibration.correction_factors[5]);
    } else {
        printf("Failed to load calibration parameters: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
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
        printf("SCD41 - CO2: %u ppm, Temperature: %.2f °C, Humidity: %.2f %%\n", co2, temperature, humidity);
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
    printf("  nvs_set_i32 - Set an integer value in NVS\n");
    printf("  nvs_get_i32 - Get an integer value from NVS\n");
    printf("  nvs_set_str - Set a string value in NVS\n");
    printf("  nvs_get_str - Get a string value from NVS\n");
    printf("  nvs_stats - Print NVS statistics\n");
    printf("  set_as7262_cal - Set AS7262 calibration parameters\n");
    printf("  get_as7262_cal - Get AS7262 calibration parameters\n");
    return 0;
}

// Command handler for setting an integer value in NVS
int cmd_nvs_set_i32(int argc, char **argv) {
    int32_t value;
    if (argc != 3) {
        printf("Usage: nvs_set_i32 <key> <value>\n");
        return 1;
    }
    const char* key = argv[1];
    value = atoi(argv[2]);
    esp_err_t ret = nvs_service_set_i32(key, value);
    if (ret == ESP_OK) {
        nvs_service_commit();
        printf("Set integer value: %" PRId32 " for key: %s\n", value, key);
    } else {
        printf("Failed to set integer value: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for getting an integer value from NVS
int cmd_nvs_get_i32(int argc, char **argv) {
    int32_t value;
    if (argc != 2) {
        printf("Usage: nvs_get_i32 <key>\n");
        return 1;
    }
    const char* key = argv[1];
    esp_err_t ret = nvs_service_get_i32(key, &value);
    if (ret == ESP_OK) {
        printf("Got integer value: %" PRId32 " for key: %s\n", value, key);
    } else {
        printf("Failed to get integer value: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for setting a string value in NVS
int cmd_nvs_set_str(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: nvs_set_str <key> <value>\n");
        return 1;
    }
    const char* key = argv[1];
    const char* value = argv[2];
    esp_err_t ret = nvs_service_set_str(key, value);
    if (ret == ESP_OK) {
        nvs_service_commit();
        printf("Set string value: %s for key: %s\n", value, key);
    } else {
        printf("Failed to set string value: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for getting a string value from NVS
int cmd_nvs_get_str(int argc, char **argv) {
    char value[128]; // Adjust size as needed
    if (argc != 2) {
        printf("Usage: nvs_get_str <key>\n");
        return 1;
    }
    const char* key = argv[1];
    esp_err_t ret = nvs_service_get_str(key, value, sizeof(value));
    if (ret == ESP_OK) {
        printf("Got string value: %s for key: %s\n", value, key);
    } else {
        printf("Failed to get string value: %s\n", esp_err_to_name(ret));
    }
    return ret == ESP_OK ? 0 : 1;
}

// Command handler for printing NVS statistics
int cmd_nvs_stats(int argc, char **argv) {
    print_nvs_stats();
    return 0;
}

// Register commands
void register_commands() {
    esp_console_cmd_t cmd;

    cmd = (esp_console_cmd_t) {
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

    // Register NVS commands
    cmd = (esp_console_cmd_t) {
        .command = "nvs_set_i32",
        .help = "Set an integer value in NVS",
        .hint = NULL,
        .func = &cmd_nvs_set_i32,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "nvs_get_i32",
        .help = "Get an integer value from NVS",
        .hint = NULL,
        .func = &cmd_nvs_get_i32,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "nvs_set_str",
        .help = "Set a string value in NVS",
        .hint = NULL,
        .func = &cmd_nvs_set_str,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "nvs_get_str",
        .help = "Get a string value from NVS",
        .hint = NULL,
        .func = &cmd_nvs_get_str,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "nvs_stats",
        .help = "Print NVS statistics",
        .hint = NULL,
        .func = &cmd_nvs_stats,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    // Register AS7262 calibration commands
    cmd = (esp_console_cmd_t) {
        .command = "set_as7262_cal",
        .help = "Set AS7262 calibration parameters",
        .hint = "<V> <B> <G> <Y> <O> <R>",
        .func = &cmd_set_as7262_calibration,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t) {
        .command = "get_as7262_cal",
        .help = "Get AS7262 calibration parameters",
        .hint = NULL,
        .func = &cmd_get_as7262_calibration,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
