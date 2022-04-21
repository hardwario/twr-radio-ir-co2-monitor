
#ifndef _TWR_IR_CO2
#define _TWR_IR_CO2

#include <twr_scheduler.h>
#include <twr_uart.h>
#include <twr_log.h>


typedef struct twr_ir_co2_t twr_ir_co2_t;

typedef enum
{
    //! @brief Error event
    TWR_IR_CO2_EVENT_ERROR = -1,

    //! @brief Update event
    TWR_IR_CO2_EVENT_UPDATE = 0,

} twr_ir_co2_event_t;

typedef enum
{
    TWR_IR_CO2_STATE_ERROR = -1,
    TWR_IR_CO2_STATE_MEASURE = 0,
    TWR_IR_CO2_STATE_READ = 1,
    TWR_IR_CO2_STATE_UPDATE = 2

} twr_ir_co2_state_t;

struct twr_ir_co2_t
{
    twr_scheduler_task_id_t _task_id_interval;
    twr_scheduler_task_id_t _task_id_measure;
    twr_scheduler_task_id_t _calibration_task_id;
    twr_scheduler_task_id_t _factory_reset_task_id;

    twr_uart_channel_t _channel;

    void (*_event_handler)(twr_ir_co2_t *, twr_ir_co2_event_t);
    bool _measurement_active;
    bool _calibration_active;
    twr_tick_t _update_interval;
    twr_ir_co2_state_t _state;

    uint32_t _sensor_id;
    uint32_t _timestamp;
    int32_t _co2_concentration_raw;
    float _co2_concentration;
    int16_t _temperature_raw;
    float _temperature;
    int16_t _pressure;
};

void twr_ir_co2_init(twr_ir_co2_t *self, twr_uart_channel_t uart_channel);

void twr_ir_co2_set_event_handler(twr_ir_co2_t *self, void (*event_handler)(twr_ir_co2_t *, twr_ir_co2_event_t));

void twr_ir_co2_set_update_interval(twr_ir_co2_t *self, twr_tick_t interval);

bool twr_ir_co2_measure(twr_ir_co2_t *self);

void twr_ir_co2_get_temperature(twr_ir_co2_t *self, float *temperature);

void twr_ir_co2_get_temperature_raw(twr_ir_co2_t *self, int *temperature_raw);

void twr_ir_co2_get_concentration(twr_ir_co2_t *self, float *concentration);

void twr_ir_co2_get_concentration_raw(twr_ir_co2_t *self, int *concentration);

void twr_ir_co2_get_concentration_ppm(twr_ir_co2_t *self, int *concentration_ppm);

void twr_ir_co2_get_pressure(twr_ir_co2_t *self, int *pressure);

void twr_ir_co2_zero_point_adjustment(void *param);

void twr_ir_co2_factory_reset(void *param);

#endif // _TWR_IR_CO2
