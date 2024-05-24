#ifndef AS7262_DRIVER_H
#define AS7262_DRIVER_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// AS7262 I2C Address
#define AS7262_I2C_ADDRESS 0x49

// Define registers
#define AS7262_STATUS_REG 0x00
#define AS7262_WRITE_REG 0x01
#define AS7262_READ_REG 0x02
#define AS7262_CONTROL_SETUP_REG 0x04
#define AS7262_INT_T_REG 0x05

// STATUS register bits
#define AS7262_TX_VALID 0x02
#define AS7262_RX_VALID 0x01

// Control setup register bits
#define AS7262_RST 0x80
#define AS7262_INT 0x40
#define AS7262_GAIN 0x30
#define AS7262_BANK 0x0C
#define AS7262_DATA_RDY 0x02

esp_err_t as7262_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t* dev_handle);
esp_err_t as7262_set_integration_time(i2c_master_dev_handle_t dev_handle, uint8_t integration_time);
esp_err_t as7262_start_measurement(i2c_master_dev_handle_t dev_handle, uint8_t mode);
esp_err_t as7262_read_measurement(i2c_master_dev_handle_t dev_handle, uint16_t* channels);

#endif // AS7262_DRIVER_H