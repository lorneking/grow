#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "scd41_driver.h"
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

    // Start periodic measurement
    ret = scd41_start_periodic_measurement(scd41_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start periodic measurement: %s", esp_err_to_name(ret));
        return;
    }

    // Regularly read measurements
    while (true) {
        uint16_t co2;
        float temperature, humidity;
        ret = scd41_read_measurement(scd41_dev, &co2, &temperature, &humidity);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "CO2: %d ppm, Temperature: %.2f °C, Humidity: %.2f %%", co2, temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Error reading SCD41 measurement, code: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delays for 5 seconds
    }
}