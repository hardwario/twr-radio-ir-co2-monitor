#include <application.h>

#define CO2_PUB_INTERVAL (1 * 60 * 1000)
#define CO2_UPDATE_INTERVAL 5000

twr_led_t led;
twr_ir_co2_t ir_co2;
twr_tick_t co2_next_pub;

void ir_co2_event_handler(twr_ir_co2_t *self, twr_ir_co2_event_t event)
{
    if (event == TWR_IR_CO2_EVENT_UPDATE)
    {
        if (co2_next_pub < twr_scheduler_get_spin_tick())
        {
            float concentration;
            twr_ir_co2_get_concentration(self, &concentration);
            twr_radio_pub_float("concentration", &concentration);

            co2_next_pub = twr_scheduler_get_spin_tick() + CO2_PUB_INTERVAL;

            // To get the co2 value in the ppm value, like CO2 Module
            /*int concentration_ppm;
            twr_ir_co2_get_concentration_ppm(self, &concentration_ppm);

            twr_radio_pub_int("concentration/ppm", &concentration_ppm);*/

            // To get the temperature from the sensor
            /*float temperature;
            twr_ir_co2_get_temperature(self, &temperature);
            twr_radio_pub_float("temperature", &temperature);*/
        }
    }
    else if (event == TWR_IR_CO2_EVENT_ERROR)
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

    // Initialize IR CO2
    twr_ir_co2_init(&ir_co2, TWR_UART_UART1);
    twr_ir_co2_set_event_handler(&ir_co2, ir_co2_event_handler);
    twr_ir_co2_set_update_interval(&ir_co2, CO2_UPDATE_INTERVAL);

    twr_radio_pairing_request("ir-co2-monitor", VERSION);

    twr_led_pulse(&led, 2000);
}
