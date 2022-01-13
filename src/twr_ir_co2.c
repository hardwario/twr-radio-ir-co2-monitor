#include <twr_ir_co2.h>

static void _twr_ir_co2_task_interval(void *param);

static void _twr_ir_co2_task_measure(void *param);

#define _TWR_IR_CO2_DELAY_RUN 8000

void twr_ir_co2_init(twr_ir_co2_t *self, twr_uart_channel_t channel)
{
    memset(self, 0, sizeof(*self));

    self->_channel = channel;

    twr_uart_init(self->_channel, TWR_UART_BAUDRATE_9600, TWR_UART_SETTING_8N1);

    self->_task_id_interval = twr_scheduler_register(_twr_ir_co2_task_interval, self, TWR_TICK_INFINITY);
    self->_task_id_measure = twr_scheduler_register(_twr_ir_co2_task_measure, self, _TWR_IR_CO2_DELAY_RUN);
}

void twr_ir_co2_set_event_handler(twr_ir_co2_t *self,
        void (*event_handler)(twr_ir_co2_t *, twr_ir_co2_event_t))
{
    self->_event_handler = event_handler;
}

void twr_ir_co2_set_update_interval(twr_ir_co2_t *self, twr_tick_t interval)
{
    self->_update_interval = interval;

    if (self->_update_interval == TWR_TICK_INFINITY)
    {
        twr_scheduler_plan_absolute(self->_task_id_interval, TWR_TICK_INFINITY);
    }
    else
    {
        twr_scheduler_plan_relative(self->_task_id_interval, self->_update_interval);
        twr_ir_co2_measure(self);
    }
}

void twr_ir_co2_get_temperature(twr_ir_co2_t *self, float *temperature)
{
    *temperature = self->_temperature;
}

void twr_ir_co2_get_temperature_raw(twr_ir_co2_t *self, int *temperature_raw)
{
    *temperature_raw = self->_temperature_raw;
}

void twr_ir_co2_get_concentration(twr_ir_co2_t *self, float *concentration)
{
    *concentration = self->_co2_concentration;
}

void twr_ir_co2_get_concentration_raw(twr_ir_co2_t *self, int *concentration_raw)
{
    *concentration_raw = self->_co2_concentration_raw;
}

void twr_ir_co2_get_pressure(twr_ir_co2_t *self, int *pressure)
{
    *pressure = self->_pressure;
}

bool twr_ir_co2_measure(twr_ir_co2_t *self)
{
    if (self->_measurement_active)
    {
        return false;
    }

    self->_measurement_active = true;

    twr_scheduler_plan_absolute(self->_task_id_measure, _TWR_IR_CO2_DELAY_RUN);

    return true;
}

static void _twr_ir_co2_task_interval(void *param)
{
    twr_ir_co2_t *self = param;

    twr_ir_co2_measure(self);

    twr_scheduler_plan_current_relative(self->_update_interval);
}

static void _twr_ir_co2_task_measure(void *param)
{
    twr_ir_co2_t *self = param;

    start:

    switch (self->_state)
    {
        case TWR_IR_CO2_STATE_ERROR:
        {
            self->_measurement_active = false;

            if (self->_event_handler != NULL)
            {
                self->_event_handler(self, TWR_IR_CO2_STATE_ERROR);
            }

            self->_state = TWR_IR_CO2_STATE_MEASURE;

            return;
        }
        case TWR_IR_CO2_STATE_MEASURE:
        {
            self->_state = TWR_IR_CO2_STATE_ERROR;

            uint8_t data[6] = {0x02, 0x31, 0x31, 0x30, 0x30, 0x03};
            int writed = 0;
            writed = twr_uart_write(self->_channel, data, sizeof(data));

            if(writed != 6)
            {
                self->_state = TWR_IR_CO2_STATE_ERROR;
                break;
            }

            self->_state = TWR_IR_CO2_STATE_READ;

            goto start;
        }
        case TWR_IR_CO2_STATE_READ:
        {
            self->_state = TWR_IR_CO2_STATE_ERROR;
            uint8_t _readedData[40];
            int _dataIndex = 0;

            int _readed_len = twr_uart_read(self->_channel, _readedData, sizeof(_readedData), 1000);

            char _sensorIdArray[10];
            char _timestampArray[15];
            char _concentrationArray[10];
            char _temperatureArray[5];
            char _pressureArray[15];

            int _index = 0;
            for(int i = 1; i < _readed_len - 1; i++)
            {
                if(_readedData[i] == 32)
                {
                    switch(_dataIndex)
                    {
                    case 0: _sensorIdArray[_index++] = '\0'; break;
                    case 1: _timestampArray[_index++] = '\0'; break;
                    case 2: _concentrationArray[_index++] = '\0'; break;
                    case 3: _temperatureArray[_index++] = '\0'; break;
                    case 4: _pressureArray[_index++] = '\0'; break;
                    }
                    _dataIndex++;
                    _index = 0;
                }
                switch(_dataIndex)
                {
                    case 0: _sensorIdArray[_index++] = _readedData[i]; break;
                    case 1: _timestampArray[_index++] = _readedData[i]; break;
                    case 2: _concentrationArray[_index++] = _readedData[i]; break;
                    case 3: _temperatureArray[_index++] = _readedData[i]; break;
                    case 4: _pressureArray[_index++] = _readedData[i]; break;
                }
            }

            int _sensor_id = 0;
            sscanf(_sensorIdArray, "%d", &_sensor_id);
            self->_sensor_id = _sensor_id;

            int _timestamp = 0;
            sscanf(_timestampArray, "%d", &_timestamp);
            self->_timestamp = _timestamp;

            int _concentration_raw = 0;
            sscanf(_concentrationArray, "%d", &_concentration_raw);
            self->_co2_concentration_raw = _concentration_raw;
            self->_co2_concentration = (float)_concentration_raw / 1000;

            int _temperature_raw = 0;
            sscanf(_temperatureArray, "%d", &_temperature_raw);
            self->_temperature_raw = _temperature_raw;
            self->_temperature = (float)_temperature_raw / 10;

            int _pressure = 0;
            sscanf(_pressureArray, "%d", &_pressure);
            self->_pressure = _pressure;

            self->_state = TWR_IR_CO2_STATE_UPDATE;
            goto start;
        }

        case TWR_IR_CO2_STATE_UPDATE:
        {
            self->_measurement_active = false;

            self->_state = TWR_IR_CO2_STATE_MEASURE;

            self->_event_handler(self, TWR_IR_CO2_EVENT_UPDATE);

            return;
        }
        default:
        {
            self->_state = TWR_IR_CO2_STATE_ERROR;

            goto start;
        }
    }
}
