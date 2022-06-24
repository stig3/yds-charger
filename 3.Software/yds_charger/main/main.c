/*
 * @Author: [LiaoZhelin]
 * @Date: 2022-04-03 10:05:08
 * @LastEditors: [LiaoZhelin]
 * @LastEditTime: 2022-06-14 13:26:23
 * @Description:
 */
 // System:
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_event.h"

// User:
#include "wifi.h"
#include "adc_read.h"
#include "lis3dh.h"
#include "led_strip.h"
#include "sw3526.h"
#include "led_strip.h"
#include "driver/rmt.h"
#include <driver/gpio.h>
#include "menu.h"
#include "u8g2_esp32_hal.h"
#include "ota.h"
#include "sntptime.h"

#include "task.h"

static const char* TAG = "main";

#define RMT_TX_CHANNEL RMT_CHANNEL_0

//TODO: 增加自动息屏时间，以下几种状态点亮屏幕，超时后息屏
// 1. 动作按钮 2. 快充状态变化 3. 进入/退出充电状态
void GPIO_Init(void) {
  //Input GPIO
  gpio_pad_select_gpio(8);
  gpio_set_direction(8, GPIO_MODE_INPUT);
  gpio_set_pull_mode(8, GPIO_PULLUP_ONLY);
  gpio_pad_select_gpio(9);
  gpio_set_direction(9, GPIO_MODE_INPUT);
  gpio_set_pull_mode(9, GPIO_PULLUP_ONLY);
  gpio_pad_select_gpio(10);
  gpio_set_direction(10, GPIO_MODE_INPUT);
  gpio_set_pull_mode(10, GPIO_PULLUP_ONLY);

  //Output GPIO
  gpio_pad_select_gpio(20);
  gpio_set_direction(20, GPIO_MODE_OUTPUT);
  gpio_set_level(20, 1);

  //Output GPIO
  gpio_pad_select_gpio(21);
  gpio_set_direction(21, GPIO_MODE_OUTPUT);
  gpio_set_level(21, 1);
}

void NVS_Init(void) {
  esp_err_t ret;
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void ALL_Init(void) {
  //Initization
  nvs_handle_t wificonfig_handle;
  uint8_t s_WifiConfigVal;

  u8g2_Init();// Init I2C
  oledInitMessageTask(1, "");
  NVS_Init();
  ADC_Init();
  strip = led_strip_init(RMT_CHANNEL_0, GPIO_NUM_4, 4);
  GPIO_Init();
  oledInitMessageTask(1, "OK");
  vTaskDelay(pdMS_TO_TICKS(200));
  oledInitMessageTask(2, "");
  SW35xxInit();
  oledInitMessageTask(2, "OK");
  vTaskDelay(pdMS_TO_TICKS(200));
  oledInitMessageTask(3, "");
  LIS3DH_Init();// No init I2C
  oledInitMessageTask(3, "OK");
  vTaskDelay(pdMS_TO_TICKS(200));

  oledInitMessageTask(4, "");
  ESP_ERROR_CHECK(nvs_open("WifiConfigFlag", NVS_READWRITE, &wificonfig_handle));
  nvs_get_u8(wificonfig_handle, "WifiConfigFlag", &s_WifiConfigVal);

  if (s_WifiConfigVal == wifi_configed) {
    if (wifi_init_sta(0)) {
      oledInitMessageTask(4, "OK");
      vTaskDelay(pdMS_TO_TICKS(200));
      SNTP_Init();
    }
    else {
      oledInitMessageTask(4, "ER");
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
  else {
    oledInitMessageTask(4, "NO");
    ESP_LOGI(TAG, "No Config Wifi");
    vTaskDelay(pdMS_TO_TICKS(200));

    nvs_close(wificonfig_handle);
  }
  //OTA_Init(); // Init Wifi

  //TaskCreate
  xTaskCreate(adcTask, "adcTask", 1024, NULL, 1, &adcTask_handle);
  xTaskCreate(sw35xxTask, "sw35xxTask", 1024, NULL, 1, &sw35xxTask_handle);
  xTaskCreate(lis3dhTask, "lis3dhtask", 1024, NULL, 1, &lis3dhtask_handle);
  xTaskCreate(oledTask, "oledtask", 1024 * 3, NULL, 1, &oledTask_handle);
  xTaskCreate(ws28xxTask, "ws28xxTask", 1024, NULL, 0, &ws28xxTask_handle);
  xTaskCreate(ntpClockTask, "ntpClockTask", 1024, NULL, 0, &ntpTask_handle);
  //DEBUG STACK SIZE
  xTaskCreate(taskMonitor, "taskMonitor", 1024 * 2, NULL, 1, NULL);
}

void app_main(void)
{
  ALL_Init();
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ota_process
  }
}
