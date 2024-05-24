#ifndef AS7262_DRIVER_H
#define AS7262_DRIVER_H

#include "esp_err.h"
#include "i2c_service.h"

#define AS7262_I2C_ADDRESS           0x49

// Register addresses
#define AS7262_WRITE_REG             0x01
#define AS7262_READ_REG              0x02
#define AS7262_STATUS_REG            0x00
#define AS7262_CONTROL_SETUP_REG     0x04
#define AS7262_INT_T_REG             0x05
#define AS7262_DEVICE_TEMP_REG       0x06
#define AS7262_LED_CONTROL_REG       0x07

// Status register bits
#define AS7262_TX_VALID              0x02
#define AS7262_RX_VALID              0x01

// Function prototypes
esp_err_t as7262_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t* dev_handle);
esp_err_t as7262_set_integration_time(i2c_master_dev_handle_t dev_handle, uint8_t integration_time);
esp_err_t as7262_set_gain(i2c_master_dev_handle_t dev_handle, uint8_t gain);
esp_err_t as7262_start_measurement(i2c_master_dev_handle_t dev_handle, uint8_t mode);
esp_err_t as7262_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t* channels);
esp_err_t as7262_read_calibrated_data(i2c_master_dev_handle_t dev_handle, float* channels);

#endif // AS7262_DRIVER_H
