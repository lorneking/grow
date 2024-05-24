#include "i2c_service.h"
#include "esp_log.h"

#define I2C_MASTER_SDA_IO 21      // GPIO number for I2C Master data
#define I2C_MASTER_SCL_IO 22      // GPIO number for I2C Master clock
#define I2C_MASTER_FREQ_HZ 100000 // I2C Master clock frequency
#define I2C_PORT_NUM I2C_NUM_0    // I2C port number for master dev

// Function for initializing the I2C master bus using provided configuration
esp_err_t initialize_i2c_master(i2c_master_bus_handle_t *bus_handle) {
    i2c_master_bus_config_t i2c_master_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&i2c_master_config, bus_handle);
}

// Function for adding an I2C device to the master bus
esp_err_t add_i2c_device(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle, uint16_t device_address) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    return i2c_master_bus_add_device(bus_handle, &dev_cfg, dev_handle);
}

// Function to write data to the I2C device
esp_err_t i2c_write_to_device(i2c_master_dev_handle_t dev_handle, const uint8_t *data_wr, size_t size) {
    return i2c_master_transmit(dev_handle, data_wr, size, -1);
}

// Function to read data from the I2C device
esp_err_t i2c_read_from_device(i2c_master_dev_handle_t dev_handle, uint8_t *data_rd, size_t size) {
    return i2c_master_receive(dev_handle, data_rd, size, -1);
}

// Function to deinitialize the I2C master bus
esp_err_t deinitialize_i2c_master(i2c_master_bus_handle_t bus_handle) {
    return i2c_del_master_bus(bus_handle);
}
