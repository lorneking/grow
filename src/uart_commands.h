#ifndef UART_COMMANDS_H
#define UART_COMMANDS_H

#include "tds_sensor.h"
#include "esp_console.h"
#include "esp_log.h"
#include "nvs_service.h"
#include "scd41_driver.h"
#include "as7262_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"

#define NVS_NAMESPACE "as7262_cal"

// Global device handles (extern to access from main.c)
extern i2c_master_dev_handle_t scd41_dev;
extern i2c_master_dev_handle_t as7262_dev;

typedef struct {
    float correction_factors[6];
} as7262_calibration_t;

void apply_correction_factors(float* data);
esp_err_t save_as7262_calibration(const as7262_calibration_t* calibration);
esp_err_t load_as7262_calibration(as7262_calibration_t* calibration);
int cmd_set_as7262_calibration(int argc, char **argv);
int cmd_get_as7262_calibration(int argc, char **argv);
int cmd_forced_recalibration(int argc, char **argv);
int cmd_read_scd41(int argc, char **argv);
int cmd_read_as7262(int argc, char **argv);
int cmd_help(int argc, char **argv);
int cmd_nvs_set_i32(int argc, char **argv);
int cmd_nvs_get_i32(int argc, char **argv);
int cmd_nvs_set_str(int argc, char **argv);
int cmd_nvs_get_str(int argc, char **argv);
int cmd_nvs_stats(int argc, char **argv);
int cmd_read_tds(int argc, char **argv);
int cmd_reset_system(int argc, char **argv);
void register_commands();

#endif // UART_COMMANDS_H
