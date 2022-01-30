#include <twr_ir_co2.h>

static void _twr_ir_co2_task_interval(void *param);

static void _twr_ir_co2_task_measure(void *param);

#define _TWR_IR_CO2_DELAY_RUN (9500)

void twr_ir_co2_init(twr_ir_co2_t *self, twr_uart_channel_t channel)
{
    memset(self, 0, sizeof(*self));

    self->_channel = channel;

    twr_uart_init(self->_channel, TWR_UART_BAUDRATE_9600, TWR_UART_SETTING_8N1);

    self->_task_id_interval = twr_scheduler_register(_twr_ir_co2_task_interval, self, TWR_TICK_INFINITY);
    self->_task_id_measure = twr_scheduler_register(_twr_ir_co2_task_measure, self, _TWR_IR_CO2_DELAY_RUN);

}

char twr_ir_co2_zero_point_adjustment(twr_ir_co2_t *self)
{
    uint8_t data[8] = {0x02, 0x31, 0x32, 0x30, 0x33, 0x34, 0x30, 0x03};

    int bytes_written = 0;
    bytes_written = twr_uart_write(self->_channel, data, sizeof(data));

    uint8_t read_data[3];

    int bytes_read = twr_uart_read(self->_channel, read_data, sizeof(read_data), 200);

    return read_data[1];
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
    if (self->_temperature_raw == -1000)
    {
        *temperature = NAN;
    }
    else
    {
        *temperature = self->_temperature;
    }
}

void twr_ir_co2_get_temperature_raw(twr_ir_co2_t *self, int *temperature_raw)
{
    if (self->_temperature_raw == -1000)
    {
        *temperature_raw = INT32_MIN;;
    }
    else
    {
        *temperature_raw = self->_temperature_raw;
    }
}

void twr_ir_co2_get_concentration(twr_ir_co2_t *self, float *concentration)
{
    if (self->_co2_concentration_raw == -1000)
    {
        *concentration = NAN;
    }
    else
    {
        *concentration = self->_co2_concentration;
    }
}

void twr_ir_co2_get_concentration_raw(twr_ir_co2_t *self, int *concentration_raw)
{
    if (self->_co2_concentration_raw == -1000)
    {
        *concentration_raw = INT32_MIN;
    }
    else
    {
        *concentration_raw = self->_co2_concentration_raw;
    }
}

void twr_ir_co2_get_concentration_ppm(twr_ir_co2_t *self, int *concentration_ppm)
{
    if (self->_co2_concentration_raw == -1000)
    {
        *concentration_ppm = INT32_MIN;
    }
    else
    {
        *concentration_ppm = self->_co2_concentration_raw * 10;
    }
}

void twr_ir_co2_get_pressure(twr_ir_co2_t *self, int *pressure)
{
    if (self->_pressure == -1000)
    {
        *pressure = INT32_MIN;;
    }
    else
    {
        *pressure = self->_pressure;
    }
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
            int bytes_written = 0;
            bytes_written = twr_uart_write(self->_channel, data, sizeof(data));

            if (bytes_written != 6)
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
            uint8_t read_data[50];
            int data_index = 0;

            int bytes_read = twr_uart_read(self->_channel, read_data, sizeof(read_data), 200);

            char sensor_id_array[15];
            char timestamp_array[15];
            char concentration_array[10];
            char temperature_array[5];
            char pressure_array[15];

            for(int i = 0; i < bytes_read; i++)
            {
                twr_log_debug("%d: %c", i, read_data[i]);
            }

            int index = 0;
            for (int i = 1; i < bytes_read - 1; i++)
            {
                switch (data_index)
                {
                    case 0: sensor_id_array[index++] = (read_data[i] == 0x20 ? '\0' : read_data[i]); break;
                    case 1: timestamp_array[index++] = (read_data[i] == 0x20 ? '\0' : read_data[i]); break;
                    case 2: concentration_array[index++] = (read_data[i] == 0x20 ? '\0' : read_data[i]); break;
                    case 3: temperature_array[index++] = (read_data[i] == 0x20 ? '\0' : read_data[i]); break;
                    case 4: pressure_array[index++] = (read_data[i] == 0x20 ? '\0' : read_data[i]); break;
                }

                if (read_data[i] == 0x20)
                {
                    data_index++;
                    index = 0;
                }
            }

            twr_log_debug("Concentration %s", concentration_array);

            int sensor_id = 0;
            sensor_id = strtol(sensor_id_array, NULL, 10);
            self->_sensor_id = sensor_id;

            int timestamp = 0;
            timestamp = strtol(timestamp_array, NULL, 10);
            self->_timestamp = timestamp;

            int concentration_raw = 0;
            concentration_raw = strtol(concentration_array, NULL, 10);
            self->_co2_concentration_raw = concentration_raw;
            self->_co2_concentration = (float)concentration_raw / 1000.0f;

            twr_log_debug("Concentration RAW %ld", self->_co2_concentration_raw);

            int temperature_raw = 0;
            temperature_raw = strtol(temperature_array, NULL, 10);
            self->_temperature_raw = temperature_raw;
            self->_temperature = (float)temperature_raw / 10;

            int pressure = 0;
            pressure = strtol(pressure_array, NULL, 10);
            self->_pressure = pressure;

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
