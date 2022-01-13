
#ifndef _TWR_IR_CO2
#define _TWR_IR_CO2

#include <twr_scheduler.h>

struct twr_ir_co2_sensor_t
{
    uint32_t _sensor_id;
    uint32_t _timestamp;
    int32_t _co2_concentration_raw;
    float _co2_concentration;
    int16_t _temperature_raw;
    float _temperature;
    int16_t _pressure;
};

typedef enum
{
    //! @brief Error event
    TWR_IR_CO2_EVENT_ERROR = -1,

    //! @brief Update event
    TWR_IR_CO2_EVENT_UPDATE = 0,

} twr_ir_co2_event_t;

typedef enum
{
    TWR_DS18B20_STATE_ERROR = -1,
    TWR_DS18B20_STATE_PREINITIALIZE = 0,
    TWR_DS18B20_STATE_INITIALIZE = 1,
    TWR_DS18B20_STATE_READY = 2,
    TWR_DS18B20_STATE_MEASURE = 3,
    TWR_DS18B20_STATE_READ = 4,
    TWR_DS18B20_STATE_UPDATE = 5

} twr_ir_co2_state_t;

struct twr_ir_co2_t
{
    twr_scheduler_task_id_t _task_id_interval;
    twr_scheduler_task_id_t _task_id_measure;
    void (*_event_handler)(twr_ir_co2_t *, twr_ir_co2_event_t, void *);
    void *_event_param;
    bool _measurement_active;
    twr_tick_t _update_interval;
    twr_ir_co2_state_t _state;

    twr_ir_co2_state_t *_sensor;
};

#endif // _TWR_IR_CO2
