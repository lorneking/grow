#include "nvs_service.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <inttypes.h> // Include for PRId32

static const char* TAG = "NVS_SERVICE";
static nvs_handle_t my_nvs_handle; // Rename to avoid conflicts

esp_err_t nvs_service_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t nvs_service_set_i32(const char* key, int32_t value) {
    esp_err_t ret = nvs_set_i32(my_nvs_handle, key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to NVS (%s)!", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_service_get_i32(const char* key, int32_t* value) {
    esp_err_t ret = nvs_get_i32(my_nvs_handle, key, value);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Value for key %s = %" PRId32, key, *value); // Use PRId32
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "The value for key %s is not initialized yet!", key);
    } else {
        ESP_LOGE(TAG, "Error (%s) reading key %s!", esp_err_to_name(ret), key);
    }
    return ret;
}

esp_err_t nvs_service_set_str(const char* key, const char* value) {
    esp_err_t ret = nvs_set_str(my_nvs_handle, key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write string to NVS (%s)!", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_service_get_str(const char* key, char* value, size_t max_len) {
    esp_err_t ret = nvs_get_str(my_nvs_handle, key, value, &max_len);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Value for key %s = %s", key, value);
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "The value for key %s is not initialized yet!", key);
    } else {
        ESP_LOGE(TAG, "Error (%s) reading key %s!", esp_err_to_name(ret), key);
    }
    return ret;
}

esp_err_t nvs_service_erase_key(const char* key) {
    esp_err_t ret = nvs_erase_key(my_nvs_handle, key);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase key from NVS (%s)!", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_service_commit(void) {
    esp_err_t ret = nvs_commit(my_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit changes to NVS (%s)!", esp_err_to_name(ret));
    }
    return ret;
}
