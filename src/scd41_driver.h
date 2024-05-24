#ifndef SCD41_DRIVER_H
#define SCD41_DRIVER_H

#include "esp_err.h"
#include "i2c_service.h"
#include <stdbool.h>

// SCD41 I2C address
#define SCD41_I2C_ADDRESS 0x62

// SCD41 command codes
#define START_PERIODIC_MEASUREMENT 0x21B1
#define STOP_PERIODIC_MEASUREMENT 0x3F86
#define MEASURE_SINGLE_SHOT 0x219D
#define PERFORM_SELF_TEST 0x3639
#define PERFORM_FACTORY_RESET 0x3632
#define REINIT 0x3646

// Function declarations

// Initialize the SCD41
esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t* dev_handle);

// Start periodic measurement
esp_err_t scd41_start_periodic_measurement(i2c_master_dev_handle_t dev_handle);

// Stop periodic measurement
esp_err_t scd41_stop_periodic_measurement(i2c_master_dev_handle_t dev_handle);

// Read measurement values
esp_err_t scd41_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t* co2, float* temperature, float* humidity);

// Perform single-shot measurement
esp_err_t scd41_measure_single_shot(i2c_master_dev_handle_t dev_handle);

// Perform self-test
esp_err_t scd41_perform_self_test(i2c_master_dev_handle_t dev_handle, bool* malfunction);

// Perform factory reset
esp_err_t scd41_perform_factory_reset(i2c_master_dev_handle_t dev_handle);

// Reinitialize the sensor
esp_err_t scd41_reinit(i2c_master_dev_handle_t dev_handle);

// Function to start automatic self-calibration
esp_err_t scd41_start_automatic_self_calibration(i2c_master_dev_handle_t dev_handle);

// Function to set forced recalibration
esp_err_t scd41_set_forced_recalibration(i2c_master_dev_handle_t dev_handle, uint16_t target_co2_concentration);

#endif // SCD41_DRIVER_H
