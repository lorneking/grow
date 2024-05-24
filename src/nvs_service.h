#ifndef NVS_SERVICE_H
#define NVS_SERVICE_H

#include "esp_err.h"

// Initialize NVS
esp_err_t nvs_service_init(void);

// Commit NVS changes
esp_err_t nvs_service_commit(void);

// Set an integer value in NVS
esp_err_t nvs_service_set_i32(const char* key, int32_t value);

// Get an integer value from NVS
esp_err_t nvs_service_get_i32(const char* key, int32_t* value);

// Set a string value in NVS
esp_err_t nvs_service_set_str(const char* key, const char* value);

// Get a string value from NVS
esp_err_t nvs_service_get_str(const char* key, char* value, size_t length);

// Set a blob in NVS
esp_err_t nvs_service_set_blob(const char* key, const void* value, size_t length);

// Get a blob from NVS
esp_err_t nvs_service_get_blob(const char* key, void* value, size_t* length);

// Print NVS statistics
void print_nvs_stats(void);

#endif // NVS_SERVICE_H
