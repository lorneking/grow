#include "tds_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char* TAG = "TDS_SENSOR";

#define ADC_CHANNEL ADC_CHANNEL_0 // GPIO26 is ADC_CHANNEL_0
#define ADC_UNIT ADC_UNIT_1       // ADC Unit 1

static adc_oneshot_unit_handle_t adc_handle;

esp_err_t tds_sensor_init(void) {
    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "TDS sensor initialized");
    return ESP_OK;
}

float read_tds_sensor(void) {
    int raw_value;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
    // Convert the raw ADC value to TDS value (this conversion factor may vary based on your specific sensor and calibration)
    float tds_value = (float)raw_value * (5.0 / 4096.0) * 500.0; // Example conversion formula

    ESP_LOGI(TAG, "Raw ADC Value: %d, TDS Value: %.2f ppm", raw_value, tds_value);
    return tds_value;
}
