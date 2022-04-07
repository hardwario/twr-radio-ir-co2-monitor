#include <application.h>

#define CO2_PUB_INTERVAL (15 * 60 * 1000)
#define CO2_UPDATE_INTERVAL 6000

twr_led_t led;
twr_ir_co2_t ir_co2;
twr_tick_t co2_next_pub;
twr_button_t button;

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_BUTTON_EVENT_HOLD)
    {
        char response = twr_ir_co2_zero_point_adjustment(&ir_co2);
        if(response == '0')
        {
            twr_led_blink(&led, 10);
        }
        else
        {
            twr_led_pulse(&led, 5000);
        }
    }
}

void ir_co2_event_handler(twr_ir_co2_t *self, twr_ir_co2_event_t event)
{
    if (event == TWR_IR_CO2_EVENT_UPDATE)
    {
        if (co2_next_pub < twr_scheduler_get_spin_tick())
        {
            float concentration;
            twr_ir_co2_get_concentration(self, &concentration);

            twr_radio_pub_float("concentration", &concentration);
            twr_log_debug("concentration radio %.2f", concentration);

            // To get the co2 value in the ppm value, like CO2 Module
            /*int concentration_ppm;
            twr_ir_co2_get_concentration_ppm(self, &concentration_ppm);

            twr_radio_pub_int("concentration/ppm", &concentration_ppm);
            twr_log_debug("Concentration ppm %d", concentration_ppm);*/

            // To get the temperature from the sensor
            float temperature;
            twr_ir_co2_get_temperature(self, &temperature);
            twr_radio_pub_float("temperature", &temperature);

            co2_next_pub = twr_scheduler_get_spin_tick() + CO2_PUB_INTERVAL;
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

    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN,0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);
    twr_button_set_hold_time(&button, 1000);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);

    // Initialize IR CO2
    twr_ir_co2_init(&ir_co2, TWR_UART_UART1);
    twr_ir_co2_set_event_handler(&ir_co2, ir_co2_event_handler);
    twr_ir_co2_set_update_interval(&ir_co2, CO2_UPDATE_INTERVAL);

    twr_scheduler_plan_from_now(0, 4000);

    twr_radio_pairing_request("ir-co2-monitor", VERSION);

    twr_led_pulse(&led, 2000);
}

void application_task()
{
    twr_log_debug("Application task");

    // FACTORY RESET
    /*uint8_t data[6] = {0x02, 0x35, 0x30, 0x30, 0x35, 0x03};

    int bytes_written = 0;
    bytes_written = twr_uart_write(TWR_UART_UART1, data, sizeof(data));

    uint8_t read_data[3];

    int bytes_read = twr_uart_read(TWR_UART_UART1, read_data, sizeof(read_data), 200);

    twr_log_debug("DONE %d", read_data[1]);*/

}
