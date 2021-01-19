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
static const char *TAG = "DHT";

#define RESPONSE_ERR -3
#define CRC_ERROR -2
#define TIMEOUT_ERROR -1
#define DHT_OK 0

static int64_t last_read_time = -2000000;

gpio_num_t DHT_GPIO = 15;

//func prototypes
static int wait_response();
static int wait_change_level(int level, int time);
int check_crc(uint8_t *data);

static void send_dht_start();
static int read_dht_data(uint8_t raw_bytes[5]);

void set_dht_gpio(gpio_num_t pin)
{  
   //Wait to pass the unstable state
   vTaskDelay(1000 / portTICK_PERIOD_MS);
   DHT_GPIO = pin;
}

static void send_dht_start()
{  
   //Select pad as GPIO mode, to control pin and read
   gpio_pad_select_gpio(DHT_GPIO);

   gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);
   gpio_set_level(DHT_GPIO, 0);
   ets_delay_us(20 * 1000);
   gpio_set_level(DHT_GPIO, 1);
   ets_delay_us(40);
   gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
}

static int wait_response()
{
   if (wait_change_level(0, 80) == TIMEOUT_ERROR)
      return RESPONSE_ERR;

   if (wait_change_level(1, 80) == TIMEOUT_ERROR)
      return RESPONSE_ERR;

   return DHT_OK;
}

static int wait_change_level(int level, int usec_time)
{
   int usec_ticks = 0;
   while (gpio_get_level(DHT_GPIO) == level)
   {
      if (usec_ticks++ > usec_time)
         return TIMEOUT_ERROR;
      ets_delay_us(1);
   }
   return usec_ticks;
}

static int read_dht_data(uint8_t raw_bytes[5])
{

   if (esp_timer_get_time() - 2000000 < last_read_time)
      return TIMEOUT_ERROR;

   last_read_time = esp_timer_get_time();

   uint8_t time_width = 0;

   send_dht_start();

   if (wait_response() == RESPONSE_ERR)
      return RESPONSE_ERR;

   //start reading
   for (uint8_t i = 0; i < 40; i++)
   {
      //0 is start-bit
      if (wait_change_level(0, 50) == TIMEOUT_ERROR)
         return TIMEOUT_ERROR;

      //1 with variable length is the data bit
      time_width = wait_change_level(1, 70);
      //20 and 30 was time widths found printing time_width
      if (time_width > 28)
         set_bit(raw_bytes[i / 8], (7 - (i % 8)));
   }
   return DHT_OK;
}

int check_crc(uint8_t *data)
{
   if (data[4] == (data[0] + data[1] + data[2] + data[3]))
      return DHT_OK;
   else
      return CRC_ERROR;
}

void read_dht(double *temp, double *hum)
{
   //Clear
   *temp = 0, *hum = 0;

   uint8_t bytes[5] = {0};
   int err = 0;
   err = read_dht_data(bytes);

   if (check_crc(bytes) != CRC_ERROR && (err == DHT_OK))
   {
      *hum = bytes[0] + (bytes[1] / 10.0);
      *temp = bytes[2] + (bytes[3] / 10.0);
      ESP_LOGI(TAG, "T: %f, H:%f", *temp, *hum);
   }
}
