#ifndef I2C_SERVICE_H
#define I2C_SERVICE_H

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t initialize_i2c_master(i2c_master_bus_handle_t *bus_handle);
esp_err_t add_i2c_device(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle, uint16_t device_address);
esp_err_t i2c_write_to_device(i2c_master_dev_handle_t dev_handle, const uint8_t *data_wr, size_t size);
esp_err_t i2c_read_from_device(i2c_master_dev_handle_t dev_handle, uint8_t *data_rd, size_t size);
esp_err_t deinitialize_i2c_master(i2c_master_bus_handle_t bus_handle);

#endif // I2C_SERVICE_H