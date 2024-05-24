#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "scd41_driver.h"
#include "as7262_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_service.h"  // Ensure this header file includes the necessary I2C function declarations

#define TAG "Main"

void app_main(void) {
    // Initialize I2C master bus
    i2c_master_bus_handle_t i2c_bus;
    esp_err_t ret = initialize_i2c_master(&i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        return;
    }

    // Add the SCD41 device on the I2C bus
    i2c_master_dev_handle_t scd41_dev;
    ret = scd41_init(i2c_bus, &scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SCD41 device: %s", esp_err_to_name(ret));
        return;
    }

    // Add the AS7262 device on the I2C bus
    i2c_master_dev_handle_t as7262_dev;
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

    // Regularly read measurements
    while (true) {
        // Read SCD41 measurements
        uint16_t co2;
        float temperature, humidity;
        ret = scd41_read_measurement(scd41_dev, &co2, &temperature, &humidity);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "SCD41 - CO2: %d ppm, Temperature: %.2f °C, Humidity: %.2f %%", co2, temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Error reading SCD41 measurement, code: %s", esp_err_to_name(ret));
        }

        // Read AS7262 measurements
        uint16_t channels[6];
        ret = as7262_read_measurement(as7262_dev, channels);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "AS7262 - V: %d, B: %d, G: %d, Y: %d, O: %d, R: %d", channels[0], channels[1], channels[2], channels[3], channels[4], channels[5]);
        } else {
            ESP_LOGE(TAG, "Error reading AS7262 measurement, code: %s", esp_err_to_name(ret));
        }

        // Delay for 5 seconds between readings
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}