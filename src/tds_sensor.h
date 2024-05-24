#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#include "esp_err.h"

esp_err_t initialize_tds_sensor(void);
float read_tds_sensor(void);

#endif // TDS_SENSOR_H
