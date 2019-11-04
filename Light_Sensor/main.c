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
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

void light_sensor_task(void *_args) {

// thaks to https://github.com/peros550/esp-homekit-multiple-sensors/blob/master/examples/multiple_sensors/multiple_sensors.c

    uint16_t analog_light_value;
    while (1) {
         analog_light_value = sdk_system_adc_read();
		 //The below code does not produce accurate LUX readings which is what homekit expects. It only provides an indication of brightness on a scale between 0 to 1024
		//More work needs to be done so that accurate conversation to LUX scale can take place. However this is strongly dependent on the type of sensor used.
		//In my case I used a Photodiode Light Sensor
            currentAmbientLightLevel.value.float_value = (1024 - analog_light_value);
            homekit_characteristic_notify(&currentAmbientLightLevel, HOMEKIT_FLOAT((1024 - analog_light_value)));
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
            HOMEKIT_CHARACTERISTIC(MODEL, "MKVF2LL/L"),
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
    .password = "438-33-595",
    .setupId="RW7X",
};

//accessory category code = 10

void user_init(void) {
    uart_set_baud(0, 115200);

    light_sensor_init();
    void on_wifi_ready();

    homekit_server_init(&config);
}
