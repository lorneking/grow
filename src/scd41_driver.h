#ifndef SCD41_DRIVER_H
#define SCD41_DRIVER_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// I2C configuration for SCD41
#define SCD41_I2C_ADDRESS  0x62  // 7-bit address (0xC4 >> 1)

// SCD41 Command Codes
#define START_PERIODIC_MEASUREMENT  0x21B1
#define STOP_PERIODIC_MEASUREMENT   0x3F86
#define READ_MEASUREMENT            0xEC05
#define MEASURE_SINGLE_SHOT         0x219D
#define PERFORM_SELF_TEST           0x3639
#define PERFORM_FACTORY_RESET       0x3632
#define REINIT                      0x3646

// Function prototypes
esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle);
esp_err_t scd41_start_periodic_measurement(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_stop_periodic_measurement(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t *co2, float *temperature, float *humidity);
esp_err_t scd41_measure_single_shot(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_perform_self_test(i2c_master_dev_handle_t dev_handle, bool *malfunction);
esp_err_t scd41_perform_factory_reset(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_reinit(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_start_automatic_self_calibration(i2c_master_dev_handle_t dev_handle);
esp_err_t scd41_set_forced_recalibration(i2c_master_dev_handle_t dev_handle, uint16_t target_co2_concentration);

#endif // SCD41_DRIVER_H