#ifndef UART_COMMANDS_H
#define UART_COMMANDS_H

#include "esp_err.h"
#include "esp_console.h"

// Structure to hold calibration parameters
typedef struct {
    float correction_factors[6];
} as7262_calibration_t;

// Function declarations for command handlers
int cmd_forced_recalibration(int argc, char **argv);
int cmd_read_scd41(int argc, char **argv);
int cmd_read_as7262(int argc, char **argv);
int cmd_help(int argc, char **argv);
int cmd_nvs_set_i32(int argc, char **argv);
int cmd_nvs_get_i32(int argc, char **argv);
int cmd_nvs_set_str(int argc, char **argv);
int cmd_nvs_get_str(int argc, char **argv);
int cmd_nvs_stats(int argc, char **argv);
int cmd_set_as7262_calibration(int argc, char **argv);
int cmd_get_as7262_calibration(int argc, char **argv);

// Function to register all UART commands
void register_commands(void);

// Function to save and load calibration parameters
esp_err_t save_as7262_calibration(const as7262_calibration_t* calibration);
esp_err_t load_as7262_calibration(as7262_calibration_t* calibration);

#endif // UART_COMMANDS_H
