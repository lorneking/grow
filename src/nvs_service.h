#ifndef NVS_SERVICE_H
#define NVS_SERVICE_H

#include "esp_err.h"
#include <stdint.h>

// Initialize the NVS service
esp_err_t nvs_service_init(void);

// Write an integer to NVS
esp_err_t nvs_service_set_i32(const char* key, int32_t value);

// Read an integer from NVS
esp_err_t nvs_service_get_i32(const char* key, int32_t* value);

// Write a string to NVS
esp_err_t nvs_service_set_str(const char* key, const char* value);

// Read a string from NVS
esp_err_t nvs_service_get_str(const char* key, char* value, size_t max_len);

// Erase a key from NVS
esp_err_t nvs_service_erase_key(const char* key);

// Commit changes to NVS
esp_err_t nvs_service_commit(void);

// Print NVS statistics
void print_nvs_stats(void);

#endif // NVS_SERVICE_H
