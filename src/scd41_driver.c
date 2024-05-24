#include "scd41_driver.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"
#include "crc.h"

static const char* TAG = "SCD41_DRIVER";

// Helper function to send command
esp_err_t scd41_send_command(i2c_master_dev_handle_t dev_handle, uint16_t command) {
    uint8_t cmd[2];
    cmd[0] = (command >> 8) & 0xFF;
    cmd[1] = command & 0xFF;
    return i2c_write_to_device(dev_handle, cmd, sizeof(cmd));
}

// Helper function to read data
esp_err_t scd41_read_data(i2c_master_dev_handle_t dev_handle, uint8_t* data, size_t len) {
    return i2c_read_from_device(dev_handle, data, len);
}

// Initialize the SCD41
esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t* dev_handle) {
    return add_i2c_device(bus_handle, dev_handle, SCD41_I2C_ADDRESS);
}

// Start periodic measurement
esp_err_t scd41_start_periodic_measurement(i2c_master_dev_handle_t dev_handle) {
    return scd41_send_command(dev_handle, START_PERIODIC_MEASUREMENT);
}

// Stop periodic measurement
esp_err_t scd41_stop_periodic_measurement(i2c_master_dev_handle_t dev_handle) {
    return scd41_send_command(dev_handle, STOP_PERIODIC_MEASUREMENT);
}

// Read measurement values
esp_err_t scd41_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t* co2, float* temperature, float* humidity) {
    uint8_t data[9];
    esp_err_t ret = scd41_read_data(dev_handle, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }

    // Log raw data for debugging
    ESP_LOGI(TAG, "Raw data: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);

    // Verify CRC
    for (int i = 0; i < 3; i++) {
        uint8_t calculated_crc = calculate_crc(data + (i * 3), 2);
        if (calculated_crc != data[2 + 3 * i]) {
            ESP_LOGE(TAG, "CRC mismatch at block %d: Calculated CRC: 0x%02X, Received CRC: 0x%02X", i, calculated_crc, data[2 + 3 * i]);
            return ESP_ERR_INVALID_CRC;
        }
    }

    *co2 = (data[0] << 8) | data[1];
    uint16_t raw_temp = (data[3] << 8) | data[4];
    uint16_t raw_hum = (data[6] << 8) | data[7];

    *temperature = -45 + 175 * ((float)raw_temp / 65535);
    *humidity = 100 * ((float)raw_hum / 65535);

    return ESP_OK;
}

// Perform single-shot measurement
esp_err_t scd41_measure_single_shot(i2c_master_dev_handle_t dev_handle) {
    return scd41_send_command(dev_handle, MEASURE_SINGLE_SHOT);
}

// Perform self-test
esp_err_t scd41_perform_self_test(i2c_master_dev_handle_t dev_handle, bool* malfunction) {
    uint8_t data[3];
    esp_err_t ret = scd41_send_command(dev_handle, PERFORM_SELF_TEST);
    if (ret != ESP_OK) {
        return ret;
    }

    // Wait for 10 seconds for the self-test to complete
    vTaskDelay(pdMS_TO_TICKS(10000));

    ret = scd41_read_data(dev_handle, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }

    if (data[0] == 0 && data[1] == 0) {
        *malfunction = false;
    } else {
        *malfunction = true;
    }

    return ESP_OK;
}

// Perform factory reset
esp_err_t scd41_perform_factory_reset(i2c_master_dev_handle_t dev_handle) {
    return scd41_send_command(dev_handle, PERFORM_FACTORY_RESET);
}

// Reinitialize the sensor
esp_err_t scd41_reinit(i2c_master_dev_handle_t dev_handle) {
    return scd41_send_command(dev_handle, REINIT);
}