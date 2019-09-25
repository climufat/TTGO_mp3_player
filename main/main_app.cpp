/* GPIO Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <strstream>
#include <fstream> 
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "defines.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_event_loop.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "eth.h"
#include "event.h"
#include "wifi.h"
#include "hal_i2c.h"
#include "hal_i2s.h"
#include "wm8978.h"
#include "webserver.h"
#include "http.h"
#include "cJSON.h"
#include "mdns_task.h"
#include "audio.h"
#include "aplay.h"
#include <dirent.h>
#include "esp_heap_caps.h"
#include "euler.h"
#include "websocket.h"
#include "esp_heap_caps.h"
#include "ftpd.h"
#include "ws2812.h"

void app_main(void);
}

esp_err_t setup_fs(); 
esp_err_t setup_io();
esp_err_t setup_WM8978();
int read_num_files();
uint8_t gPlay_random_file = false; 

void doSomething(int numFiles)
{
  char buffer [32];

  gpio_set_level(GPIO_OUTPUT_IO_0, 1);

  if( numFiles != 0 )
  {
    uint16_t sel = esp_random() % numFiles;
    ESP_LOGD(MAIN_TAG,"Playing %03d.mp3",sel);
    sprintf (buffer, "/sdcard/%03d.mp3", sel);
    aplay_mp3(buffer);
  }
  gpio_set_level(GPIO_OUTPUT_IO_0, 0);
}

void app_main(void)
{
  esp_err_t err;
  uint8_t has_err = false; 
  int numFiles = 0;
  event_engine_init();
  err = nvs_flash_init();
  if( err != ESP_OK )
  {
    ESP_LOGE(MAIN_TAG,"init nvs flash failed (%d)",err); 
    has_err = true; 
  }

  err = setup_fs();
  if( err != ESP_OK )
  {
    ESP_LOGE(MAIN_TAG,"init file system failed (%d)",err); 
    has_err = true; 
  }

  err = setup_io();
  if( err != ESP_OK )
  {
    ESP_LOGE(MAIN_TAG,"init IO failed (%d)",err); 
    has_err = true; 
  }

  err = setup_WM8978();
  if( err != ESP_OK )
  {
    ESP_LOGE(MAIN_TAG,"init WM8978 failed (%d)",err); 
    has_err = true; 
  }

  ws2812_init(WS2812_PIN);


  /*print the last ram*/
  size_t free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
  ESP_LOGI(MAIN_TAG,"free mem8bit: %d mem32bit: %d\n",free8start,free32start);

  /* switch off the rgb led ring... no need for it now */ 
  const uint8_t pixel_count = 19; // Number of your "pixels"
  rgbVal *pixels = (rgbVal*)malloc(sizeof(rgbVal) * pixel_count);


  if( has_err == false )
  {

    for (uint8_t i = 0; i < pixel_count; i++) 
      pixels[i] = makeRGBVal(128, 0, 0);
    ws2812_setColors(pixel_count, pixels);
    vTaskDelay(500);

    for (uint8_t i = 0; i < pixel_count; i++) 
      pixels[i] = makeRGBVal(0, 0, 0);
    ws2812_setColors(pixel_count, pixels);

    numFiles = read_num_files();

    while (1)
    {
      if (gPlay_random_file == true)
      {
        doSomething(numFiles);
        gPlay_random_file = false;
      }
      esp_sleep_enable_touchpad_wakeup();
      vTaskDelay(10); 
      // add deep sleep to save battery ... or light sleep with BT. 
    }
  }
  else
  {
    while (1)
    {
      /* in case there has been an error with initialization, blink the RGB LED strip red */ 
      for (uint8_t i = 0; i < pixel_count; i++) 
        pixels[i] = makeRGBVal(0, 0, 0);
      ws2812_setColors(pixel_count, pixels);
      vTaskDelay(750);

      for (uint8_t i = 0; i < pixel_count; i++) 
        pixels[i] = makeRGBVal(0, 128, 0);
      ws2812_setColors(pixel_count, pixels);
      vTaskDelay(750);
    }
  }
  
}

esp_err_t setup_fs()
{
  /*init sd card*/
  esp_err_t err;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = false,
    .max_files = 10
  }; 

  sdmmc_card_t* card;
  err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if (err != ESP_OK) 
  {
    if (err == ESP_FAIL)
    {
      ESP_LOGE(MAIN_TAG,"Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
    } else 
    {
      ESP_LOGE(MAIN_TAG,"Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", err);
    }
    return err;
  }
  sdmmc_card_print_info(stdout, card);
  return err;
}

esp_err_t setup_io()
{
  esp_err_t err = ESP_OK;
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  err |= gpio_config(&io_conf);
  err |= gpio_set_level(GPIO_OUTPUT_IO_0, 0);
  err |= gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);    // CMD, needed in 4- and 1- line modes
  err |= gpio_set_pull_mode(GPIO_NUM_2 , GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  err |= gpio_set_pull_mode(GPIO_NUM_4 , GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  err |= gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);    // D2, needed in 4-line mode only
  err |= gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);    // D3, needed in 4- and 1-line mode
  return err;
}

esp_err_t setup_WM8978()
{
      /*init codec */
    hal_i2c_init(0,19,18);
    hal_i2s_init(0,48000,16,2);
    WM8978_Init();
    WM8978_ADDA_Cfg(1,1); 
    WM8978_Input_Cfg(1,0,0);     
    WM8978_Output_Cfg(1,0); 
    WM8978_MIC_Gain(25);
    WM8978_AUX_Gain(0);
    WM8978_LINEIN_Gain(0);
    WM8978_SPKvol_Set(0);
    WM8978_HPvol_Set(15,15);
    WM8978_EQ_3D_Dir(0);
    WM8978_EQ1_Set(0,24);
    WM8978_EQ2_Set(0,24);
    WM8978_EQ3_Set(0,24);
    WM8978_EQ4_Set(0,24);
    WM8978_EQ5_Set(0,24);
    return ESP_OK;
}

int read_num_files()
{
  ESP_LOGD(MAIN_TAG,"Reading %s",MOUNT_POINT INDEX_FILE);
  std::ifstream input(MOUNT_POINT INDEX_FILE);
  std::string line;
  std::string word;  
  std::vector<std::string> header; 

  uint16_t numFiles; 

  if(input.is_open() && getline(input, line)) 
  {
    // parse first line 
    std::istringstream tmp;
    tmp.str(line);
    while(getline(tmp, word,';'))
    {
      header.push_back(word);
    }
  }
  else
  {
    ESP_LOGE(MAIN_TAG,"Error opening header file");
  }

  if( header.size() != 2 ) 
  {
    ESP_LOGE(MAIN_TAG,"Error reading header: len=%d",header.size());
    numFiles = 0;
  }
  else
  {
    // must be between 0 and 999
    numFiles = std::min(999,std::max(0, atoi( header.at(1).c_str()) ));
  }
  return numFiles;
}