#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int led_gpio = 2;
bool led_on = false;

void led_write(bool on) {
        gpio_write(led_gpio, on ? 0 : 1);
}
void identify_task(void *_args) {
        for (int i=0; i<3; i++) {
                for (int j=0; j<2; j++) {
                        led_write(true);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        led_write(false);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        led_write(led_on);
        vTaskDelete(NULL);
}
void identify(homekit_value_t _value) {
        printf("Identify\n");
        xTaskCreate(identify_task, "Identify", 128, NULL, 2, NULL);
}
homekit_characteristic_t currentAmbientLightLevel = HOMEKIT_CHARACTERISTIC_(CURRENT_AMBIENT_LIGHT_LEVEL, 0,.min_value = (float[]) {0},);


void light_sensor_task(void *_args) {

        uint16_t analog_light_value;
        while (1) {
          // The code below reads the LDR (Light Dependent Resistor) on the Analog PGIO from the ESP unit.
          // As HomeKit uses Lux this isn't a correct reading. This code gives a demonstration a this sensor type.
          // If you know a different approach, feel free to share with me and the communnity.
                analog_light_value = sdk_system_adc_read();
                currentAmbientLightLevel.value.float_value = (1*100/(1000-analog_light_value));
                homekit_characteristic_notify(&currentAmbientLightLevel, HOMEKIT_FLOAT((analog_light_value)));
                vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
}
void light_sensor_init() {
        xTaskCreate(light_sensor_task, "Light Sensor", 256, NULL, 2, NULL);
}


homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]){
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Light Sensor"),
                        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "StudioPieters®"),
                        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "Q39QDPS8GRX8"),
                        HOMEKIT_CHARACTERISTIC(MODEL, "HKVF2T/L"),
                        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
                        NULL
                }),
                HOMEKIT_SERVICE(LIGHT_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Light Sensor"),
                        &currentAmbientLightLevel,
                        NULL
                }),
                NULL
        }),
        NULL
};
homekit_server_config_t config = {
        .accessories = accessories,
        .password = "595-33-595",
        .setupId="RW5X",
};
//accessory category code = 10
void user_init(void) {
        uart_set_baud(0, 115200);

        light_sensor_init();
        void on_wifi_ready();

        homekit_server_init(&config);
}
