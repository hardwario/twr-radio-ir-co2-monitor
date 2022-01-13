#include <application.h>

#define BATTERY_UPDATE_INTERVAL 60 * 1000
#define SERVICE_INTERVAL_INTERVAL 15 * 60 * 1000


twr_led_t led;
twr_ir_co2_t ir_co2;

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

void ir_co2_event_handler(twr_ir_co2_t *self, twr_ir_co2_event_t event)
{
    if(event == TWR_IR_CO2_EVENT_UPDATE)
    {
        float temperature;
        int concentration_raw;
        float concentration;
        twr_ir_co2_get_temperature(self, &temperature);
        twr_ir_co2_get_concentration_raw(self, &concentration_raw);
        twr_ir_co2_get_concentration(self, &concentration);

        twr_radio_pub_int("concentration/raw", &concentration_raw);
        twr_radio_pub_float("concentration", &concentration);
    }
    else if(event == TWR_IR_CO2_STATE_ERROR)
    {
        twr_log_debug("ERROR");
    }
}

void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);
    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);

    twr_ir_co2_init(&ir_co2, TWR_UART_UART1);
    twr_ir_co2_set_event_handler(&ir_co2, ir_co2_event_handler);
    twr_ir_co2_set_update_interval(&ir_co2, 5000);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_radio_pairing_request("ir-co2-monitor", VERSION);

    twr_led_pulse(&led, 2000);
}
