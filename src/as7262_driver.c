#include "as7262_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "i2c_service.h"

//static const char* TAG = "AS7262_DRIVER";

// Internal helper functions
static esp_err_t as7262_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t value);
static esp_err_t as7262_read_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* value);
static esp_err_t as7262_read_calibrated_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, float* value);

// Buffer to store the bus handle
static i2c_master_bus_handle_t internal_bus_handle;

// Initialize the AS7262
esp_err_t as7262_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t* dev_handle) {
    internal_bus_handle = bus_handle;  // Load bus handle into internal buffer
    return add_i2c_device(internal_bus_handle, dev_handle, AS7262_I2C_ADDRESS);
}

// Configure integration time
esp_err_t as7262_set_integration_time(i2c_master_dev_handle_t dev_handle, uint8_t integration_time) {
    return as7262_write_register(dev_handle, AS7262_INT_T_REG, integration_time);
}

// Set gain
esp_err_t as7262_set_gain(i2c_master_dev_handle_t dev_handle, uint8_t gain) {
    uint8_t control;
    esp_err_t ret = as7262_read_register(dev_handle, AS7262_CONTROL_SETUP_REG, &control);
    if (ret != ESP_OK) return ret;
    control = (control & 0xCF) | (gain << 4); // Set GAIN bits
    return as7262_write_register(dev_handle, AS7262_CONTROL_SETUP_REG, control);
}

// Start measurement
esp_err_t as7262_start_measurement(i2c_master_dev_handle_t dev_handle, uint8_t mode) {
    uint8_t control;
    esp_err_t ret = as7262_read_register(dev_handle, AS7262_CONTROL_SETUP_REG, &control);
    if (ret != ESP_OK) return ret;
    control = (control & 0xF3) | (mode << 2); // Set BANK bits
    return as7262_write_register(dev_handle, AS7262_CONTROL_SETUP_REG, control);
}

// Read measurement
esp_err_t as7262_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t* channels) {
    uint8_t high, low;
    for (uint8_t i = 0; i < 6; i++) {
        esp_err_t ret = as7262_read_register(dev_handle, 0x08 + 2 * i, &high);
        if (ret != ESP_OK) return ret;
        ret = as7262_read_register(dev_handle, 0x09 + 2 * i, &low);
        if (ret != ESP_OK) return ret;
        channels[i] = (high << 8) | low;
    }
    return ESP_OK;
}

// Read calibrated data
esp_err_t as7262_read_calibrated_data(i2c_master_dev_handle_t dev_handle, float* channels) {
    for (uint8_t i = 0; i < 6; i++) {
        esp_err_t ret = as7262_read_calibrated_register(dev_handle, 0x14 + 4 * i, &channels[i]);
        if (ret != ESP_OK) return ret;
    }
    return ESP_OK;
}

// Internal helper functions
static esp_err_t as7262_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t value) {
    uint8_t status;
    do {
        esp_err_t ret = i2c_read_from_device(dev_handle, &status, sizeof(status));
        if (ret != ESP_OK) return ret;
    } while (status & AS7262_TX_VALID);

    // Write the register address
    uint8_t reg_data[2] = {AS7262_WRITE_REG, reg | 0x80};
    esp_err_t ret = i2c_write_to_device(dev_handle, reg_data, sizeof(reg_data));
    if (ret != ESP_OK) return ret;

    // Write the value
    reg_data[1] = value;
    return i2c_write_to_device(dev_handle, reg_data, sizeof(reg_data));
}

static esp_err_t as7262_read_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* value) {
    uint8_t status;
    do {
        esp_err_t ret = i2c_read_from_device(dev_handle, &status, sizeof(status));
        if (ret != ESP_OK) return ret;
    } while (status & AS7262_TX_VALID);

    // Write the register address
    uint8_t reg_data[2] = {AS7262_WRITE_REG, reg};
    esp_err_t ret = i2c_write_to_device(dev_handle, reg_data, sizeof(reg_data));
    if (ret != ESP_OK) return ret;

    do {
        ret = i2c_read_from_device(dev_handle, &status, sizeof(status));
        if (ret != ESP_OK) return ret;
    } while (!(status & AS7262_RX_VALID));

    // Read the value
    return i2c_read_from_device(dev_handle, value, sizeof(uint8_t));
}

static esp_err_t as7262_read_calibrated_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, float* value) {
    uint8_t data[4];
    for (uint8_t i = 0; i < 4; i++) {
        esp_err_t ret = as7262_read_register(dev_handle, reg + i, &data[i]);
        if (ret != ESP_OK) return ret;
    }
    *value = *(float*)data;
    return ESP_OK;
}
