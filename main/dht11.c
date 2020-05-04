/*
    dht11.c
    Author:
       Maike Rodrigo A. Reis
       Electrical Engineering Stundent at University Federal of Pará
       maikerdralcantara@gmail.com
    Created on:
       29 Apr 2020
*/
/*
 *         Send start signal
 *
 *       |     18~20 ms  40 us |
 *      -|--+            +-----|  
 *       |  |            |     |  
 *       |  |            |     |  
 *       |  |            |     |  
 *       |  +------------+     |--
 *
 *               "0"        "1"
 * 
 *     DHT init sequecen response
 *
 *                             |   80 us   80 us  |
 *                            -|-+        +-------|  
 *                             | |        |       |  
 *                             | |        |       |  
 *                             | |        |       |  
 *                             | +--------+       |--
 *
 *                                  "0"     "1"
 *
 *     DHT sending data
 *
 *                                                |  50 us  <Variable len> us  |
 *                                               -|-+     +------------------+ |      +-----
 *                                                | |     |                  | |      |
 *                                                | |     |                  | |      |
 *                                                | |     |                  | |      |
 *                                                | +-----+                  +-|------+
 *
 *                                                    "0"       18us = "0"        repeat       
 *                                                              40us = "1"        
 *                                                     |            |                |
 *                                                 Start bit     Data bit 1      Start bit
 *
 */

#include "dht11.h"

static const char *TAG = "DHT sensor";

uint8_t DHT_GPIO = 2;
uint8_t timeout = 0;
float RH = 0;
float T = 0;


uint8_t wait_change_level(int level, int time);
_Bool check_crc(uint8_t *data);



void set_dht_gpio(uint8_t pin)
{
   DHT_GPIO = pin;
}

void send_dht_start()
{

   gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);

   gpio_set_level(DHT_GPIO, 0);
   ets_delay_us(20000);

   gpio_set_level(DHT_GPIO, 1);
   ets_delay_us(40);

   gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
   gpio_pad_select_gpio(DHT_GPIO);
}

uint8_t wait_change_level(int level, int time)
{
   uint8_t cpt = 0;

   //Count how many time gpio is equal to level
   while (gpio_get_level(DHT_GPIO) == level)
   {
      if (cpt > time)
      {
         timeout = 1;
         //DO SOMETHING
      }
      ++cpt;
      ets_delay_us(1);
   }
   return cpt;
}

uint8_t *read_dht_data()
{

   uint8_t time_width = 0;
   static uint8_t bytes[5] = {0};



   portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
   portENTER_CRITICAL(&my_spinlock); // timing critical start
   {
      //Send start
      send_dht_start();
      ets_delay_us(10);

      //wait reponse
      wait_change_level(0, 80);
      wait_change_level(1, 80);

      //start reading
      for (uint8_t i = 0; i < 40; i++)
      {
         //0 is start-bit
         wait_change_level(0, 50);

         //1 with variable length is the data bit
         time_width = wait_change_level(1, 80);

         //20 and 30 was time widths found printing time_width

         if (time_width < 20)
            clr_bit(bytes[i / 8], (7 - (i % 8)));

         if (time_width > 30)
            set_bit(bytes[i / 8], (7 - (i % 8)));
      }
   }
   portEXIT_CRITICAL(&my_spinlock); // timing critical end

   return bytes;
}

_Bool check_crc(uint8_t *data)
{
   if (data[4] == (data[0] + data[1] + data[2] + data[3]))
      return true;

   return false;
}

void decode_data()
{
   uint8_t *arr = read_dht_data();

   if (check_crc(arr))
   {
      RH = arr[0] + arr[1] / 10.0;
      T = arr[2] + arr[3] / 10.0;
   }
}

float get_temperature()
{  
   decode_data();
   ESP_LOGI(TAG, "Humidity: %f", T);
   return T;
}

float get_humidity()
{
   //Because timeout is not implemented yet this piece of code must not run
   //decode_data();
   ESP_LOGI(TAG, "Temperature: %f", RH);
   return RH;
}