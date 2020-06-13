#ifndef __DHT11_H__
#define __DHT11_H__

#include "sdkconfig.h"

#include "inttypes.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <time.h>
#include "utility.h"


void set_dht_gpio(uint8_t pin);
float get_temperature();
float get_humidity();

















// void func_test1();
// int func_test2();
// void func_test3();
// int * func_test4();





#endif