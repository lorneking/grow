#ifndef UART_COMMANDS_H
#define UART_COMMANDS_H

#include "esp_err.h"
#include "esp_console.h"

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

// Function to register all UART commands
void register_commands(void);

#endif // UART_COMMANDS_H
