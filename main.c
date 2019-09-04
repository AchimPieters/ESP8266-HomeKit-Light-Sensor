#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <string.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int led_gpio = 2;



void identify_task(void *_args) {
  // We identify the `Light Sensor` by Flashing it's LED.
  for (int i=0; i<3; i++) {
          for (int j=0; j<2; j++) {
                  led_write(true);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  led_write(false);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
          }

          vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(false);

  vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    printf("identify\n\n");
    xTaskCreate(identify_task, "identify", 128, NULL, 2, NULL);
}

homekit_characteristic_t currentAmbientLightLevel = HOMEKIT_CHARACTERISTIC_(CURRENT_AMBIENT_LIGHT_LEVEL, 0,.min_value = (float[]) {0},);

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

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Light Sensor");

homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                        &name,
                        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "StudioPieters®"),
                        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "037A2BABF19D"),
                        HOMEKIT_CHARACTERISTIC(MODEL, "Light Sensor"),
                        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1.6"),
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
        .password = "558-98-144",
        .setupId="9SW7",
};

void on_wifi_ready() {
}

void create_accessory_name() {
        uint8_t macaddr[6];
        sdk_wifi_get_macaddr(STATION_IF, macaddr);

        int name_len = snprintf(NULL, 0, "Light Sensor-%02X%02X%02X",
                                macaddr[3], macaddr[4], macaddr[5]);
        char *name_value = malloc(name_len+1);
        snprintf(name_value, name_len+1, "Light Sensor-%02X%02X%02X",
                 macaddr[3], macaddr[4], macaddr[5]);

        name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
        uart_set_baud(0, 115200);
         light_sensor_init();

        create_accessory_name();

        wifi_config_init("Light Sensor", NULL, on_wifi_ready);



        homekit_server_init(&config);


}
