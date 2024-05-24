#include "tds_sensor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#define TDS_ADC_CHANNEL ADC_CHANNEL_6 // Adjust based on your actual ADC channel

static const char *TAG = "TDS_SENSOR";
static adc_oneshot_unit_handle_t adc_handle = NULL;

esp_err_t initialize_tds_sensor(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_0,
    };
    ret = adc_oneshot_config_channel(adc_handle, TDS_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

float read_tds_sensor(void) {
    if (adc_handle == NULL) {
        ESP_LOGE(TAG, "ADC handle not initialized");
        return -1.0;
    }

    int raw_value;
    esp_err_t ret = adc_oneshot_read(adc_handle, TDS_ADC_CHANNEL, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC value: %s", esp_err_to_name(ret));
        return -1.0;
    }

    // Convert the raw ADC value to TDS value here if needed
    float tds_value = (float)raw_value; // Placeholder conversion
    return tds_value;
}
