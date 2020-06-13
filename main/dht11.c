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
#include "string.h"
static const char *TAG_DHT = "DHT sensor";

#define TIMEOUT_ERROR 127
#define RESPONSE_OK 1

static int64_t last_read_time = -2000000;
volatile uint8_t bytes[5] = {0};
volatile float RH = 0;
volatile float T = 0;

uint8_t DHT_GPIO = 4;

//func prototypes
static uint8_t wait_response();
static uint8_t wait_change_level(int level, int time);
static _Bool check_crc(uint8_t *data);

static void send_dht_start();
static uint8_t *read_dht_data();
static void decode_data();

void set_dht_gpio(uint8_t pin)
{
   DHT_GPIO = pin;
}

static void send_dht_start()
{

   gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);

   gpio_set_level(DHT_GPIO, 0);
   ets_delay_us(20000);

   gpio_set_level(DHT_GPIO, 1);
   ets_delay_us(40);

   gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
   gpio_pad_select_gpio(DHT_GPIO);
}

static uint8_t wait_response()
{
   if (wait_change_level(0, 80) == TIMEOUT_ERROR)
      return TIMEOUT_ERROR;

   if (wait_change_level(1, 80) == TIMEOUT_ERROR)
      return TIMEOUT_ERROR;

   return RESPONSE_OK;
}

static uint8_t wait_change_level(int level, int time)
{
   uint8_t cpt = 0;

   //Count how many time gpio is equal to level
   while (gpio_get_level(DHT_GPIO) == level)
   {
      if (cpt > time)
      {
         return TIMEOUT_ERROR;
      }
      ++cpt;
      ets_delay_us(1);
   }
   return cpt;
}

static uint8_t *read_dht_data()
{
   //If the last reading was 2 seconds ago pass
   //otherwise, return last reading.
   if (esp_timer_get_time() - last_read_time < 2000000)
   {
      ESP_LOGI(TAG_DHT, "EARLY READ, LAST WAS < 2 seconds ago!");
      return bytes;
   }
   last_read_time = esp_timer_get_time();

   uint8_t time_width = 0;
   memset(bytes, 0, sizeof(bytes));
   
   portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
   portENTER_CRITICAL(&my_spinlock); // timing critical start
   {
      //Send start
      send_dht_start();
      ets_delay_us(10);
      //wait reponse
      if (wait_response() != RESPONSE_OK)
         return NULL;

      //start reading
      for (uint8_t i = 0; i < 40; i++)
      {
         //0 is start-bit
         if (wait_change_level(0, 50) == TIMEOUT_ERROR)
         {
            return TIMEOUT_ERROR;
         }
         else
         {
            //1 with variable length is the data bit
            time_width = wait_change_level(1, 80);

            //20 and 30 was time widths found printing time_width
            if (time_width > 30)
               set_bit(bytes[i / 8], (7 - (i % 8)));
         }
      }
   }
   portEXIT_CRITICAL(&my_spinlock); // timing critical end
   ESP_LOGI(TAG_DHT, "END OF READ!");
   return bytes;
}

static _Bool check_crc(uint8_t *data)
{
   if (data[4] == (data[0] + data[1] + data[2] + data[3]))
   {
      ESP_LOGI(TAG_DHT, "VALID CRC");
      return true;
   }

   ESP_LOGI(TAG_DHT, "INVALID CRC");
   return false;
}

static void decode_data()
{
   uint8_t *arr = read_dht_data();

   //if (check_crc(arr))
   //{
   RH = arr[0] + (arr[1] / 10.0);
   T = arr[2] + (arr[3] / 10.0);
   //}
}

float get_temperature()
{
   decode_data();
   return T;
}

float get_humidity()
{
   decode_data();
   return RH;
}