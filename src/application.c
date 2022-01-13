#include <application.h>

#define BATTERY_UPDATE_INTERVAL 60 * 1000
#define SERVICE_INTERVAL_INTERVAL 15 * 60 * 1000


twr_led_t led;

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (twr_module_battery_get_voltage(&voltage))
        {
            twr_radio_pub_battery(&voltage);
        }
    }
}

void switch_to_normal_mode_task(void *param)
{
    /*twr_module_climate_set_update_interval_thermometer(UPDATE_NORMAL_INTERVAL);
    twr_module_climate_set_update_interval_hygrometer(UPDATE_NORMAL_INTERVAL);
    twr_module_climate_set_update_interval_lux_meter(UPDATE_NORMAL_INTERVAL);
    twr_module_climate_set_update_interval_barometer(BAROMETER_UPDATE_NORMAL_INTERVAL);

    twr_scheduler_unregister(twr_scheduler_get_current_task_id());*/
}

void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);
    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);

    twr_uart_init(TWR_UART_UART1, TWR_UART_BAUDRATE_9600, TWR_UART_SETTING_8N1);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_radio_pairing_request("ir-co2-monitor", VERSION);

    twr_scheduler_register(switch_to_normal_mode_task, NULL, SERVICE_INTERVAL_INTERVAL);

    twr_led_pulse(&led, 2000);
    twr_scheduler_plan_from_now(0, 10000);
}

void application_task(void)
{
    uint8_t data[6] = {0x02, 0x31, 0x31, 0x30, 0x30, 0x03};
    uint8_t readedData[40];
    int readedLen = 0;
    twr_uart_write(TWR_UART_UART1, data, sizeof(data));
    readedLen = twr_uart_read(TWR_UART_UART1, readedData, sizeof(readedData), 1000);

    twr_log_debug("READED: %d", readedLen);

    int dataIndex = 0;
    char sensorIdArray[10];
    char timestampArray[15];
    char concentrationArray[10];
    char temperatureArray[5];
    char pressureArray[8];

    int index = 0;
    for(int i = 1; i < readedLen - 1; i++)
    {
        if(readedData[i] == 32)
        {
            switch(dataIndex)
            {
            case 0: sensorIdArray[index++] = '\0'; break;
            case 1: timestampArray[index++] = '\0'; break;
            case 2: concentrationArray[index++] = '\0'; break;
            case 3: temperatureArray[index++] = '\0'; break;
            case 4: pressureArray[index++] = '\0'; break;
            }
            dataIndex++;
            index = 0;
        }
        switch(dataIndex)
        {
            case 0: sensorIdArray[index++] = readedData[i]; break;
            case 1: timestampArray[index++] = readedData[i]; break;
            case 2: concentrationArray[index++] = readedData[i]; break;
            case 3: temperatureArray[index++] = readedData[i]; break;
            case 4: pressureArray[index++] = readedData[i]; break;
        }
    }

    int sensorId = 0;
    sscanf(sensorIdArray, "%d", &sensorId);
    int timestamp = 0;
    sscanf(timestampArray, "%d", &timestamp);
    int concentration = 0;
    sscanf(concentrationArray, "%d", &concentration);
    int temperatureInt = 0;
    sscanf(temperatureArray, "%d", &temperatureInt);
    float temperature = (float)temperatureInt / 10;
    int pressure = 0;
    sscanf(pressureArray, "%d", &pressure);

    twr_log_debug("Temperature ORIGINAL %s", temperatureArray);
    twr_log_debug("Temperature INT %d", temperatureInt);

    twr_log_debug("ID: %d", sensorId);
    twr_log_debug("Timestamp %d", timestamp);
    twr_log_debug("Concentration %d", concentration);
    twr_log_debug("Temperature %.1f", temperature);
    twr_log_debug("Pressure %d", pressure);

    twr_scheduler_plan_current_from_now(10000);
}
