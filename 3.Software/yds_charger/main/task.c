/*
 * @Author: [LiaoZhelin]
 * @Date: 2022-05-10 14:35:47
 * @LastEditors: [LiaoZhelin]
 * @LastEditTime: 2022-06-14 13:44:33
 * @Description:
 */
#include "task.h"

#include <stdio.h>
#include <string.h>
 // #include "freertos/FreeRTOS.h"
 // #include "freertos/task.h"
 // #include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "adc_read.h"
#include "lis3dh.h"
//#include "led_strip.h"
#include "sw3526.h"

static const char* TAG = "task";

void adcTask(void* pvParameters)
{
  for (;;)
  {
    ADC_getVoltage(ADC);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  vTaskDelete(NULL);
}

void sw35xxTask(void* pvParameters)
{
  for (;;)
  {
    SW35XXUpdate();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
//TODO: 根据充电状态，切换LED灯
//有充电时才点亮屏幕
//TODO:增加配色
//充电状态，快充状态，涓流充电
extern uint8_t screenShutdown;
extern uint16_t screenTimeOut;
const uint32_t colorMap[12] = { 0xb6c206,0xf9e805,0xf2991b,0xfa8a17,0xf50101,0x980d4e,0x350d60,0x2423d5,0x1897ac,0x1564bc,0x5ba723,0xd0e21a };

void ws28xxTask(void* pvParameters)
{
  uint32_t red = 0;
  uint32_t green = 0;
  uint32_t blue = 0;
  uint32_t brightness = 0;
  uint32_t color;
  uint8_t rgb_flag = 0;
  for (;;)
  {
    //LED index C1-1 C2-2 U1-3 U2-4
    if (((sw35xx_c1.OutCur * 25 / 10) > 100) || ((sw35xx_c2.OutCur * 25 / 10) > 100)) {//输出电流大于100mA
      screenShutdown = 0;
      screenTimeOut = 0;
    }
    if (screenShutdown == 0) {
      screenTimeOut++;
      if (screenTimeOut > (1000 * 30 / 10)) {
        screenShutdown = 1;
      }
    }
    if (((sw35xx_c1.OutCur * 25 / 10) > 100)) {
      color = colorMap[sw35xx_c1.protocol];
      red = (color >> 16) & 0xFF;
      green = (color >> 8) & 0xFF;
      blue = color & 0xFF;
      brightness = 50;
      //brightness = (sw35xx_c1.OutCur * 25 / 10) * 100 / 3250;//最大电流3.25A，
      ESP_ERROR_CHECK(strip->set_pixel(strip, 0, red * brightness / 100, green * brightness / 100, blue * brightness / 100));
    }
    else ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 0, 0, 0));

    if (((sw35xx_c2.OutCur * 25 / 10) > 100)) {
      color = colorMap[sw35xx_c2.protocol];
      red = (color >> 16) & 0xFF;
      green = (color >> 8) & 0xFF;
      blue = color & 0xFF;
      brightness = 50;
      //brightness = (sw35xx_c2.OutCur * 25 / 10) * 100 / 3250;//最大电流3.25A，
      ESP_ERROR_CHECK(strip->set_pixel(strip, 1, red * brightness / 100, green * brightness / 100, blue * brightness / 100));
    }
    else ESP_ERROR_CHECK(strip->set_pixel(strip, 1, 0, 0, 0));
    //0xfa8a17
    if (ADC[0] > 6000) ESP_ERROR_CHECK(strip->set_pixel(strip, 2, 0xFA, 0x8A, 0x17));
    else ESP_ERROR_CHECK(strip->set_pixel(strip, 2, 0, 0, 0));

    if (ADC[1] > 6000) ESP_ERROR_CHECK(strip->set_pixel(strip, 3, 0xFA, 0x8A, 0x17));
    else ESP_ERROR_CHECK(strip->set_pixel(strip, 3, 0, 0, 0));
    // for (int j = 1; j < 4; j += 1)
    // {
    //   ESP_ERROR_CHECK(strip->set_pixel(strip, j, 0, green, 0));
    // }
    ESP_ERROR_CHECK(strip->refresh(strip, 100));

    // if (!rgb_flag)
    // {
    //   green = (green > 150 ? 150 : green + 1);
    //   if (green == 150)
    //   {
    //     rgb_flag = 1;
    //   }
    // }
    // else
    // {
    //   green = (green < 1 ? 1 : green - 1);
    //   if (green == 1)
    //   {
    //     rgb_flag = 0;
    //   }
    // }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void lis3dhTask(void* pvParameters)
{
  uint8_t buffer1, buffer2;
  uint16_t X_V, Y_V, Z_V;
  for (;;)
  {
    LIS3DH_ReadReg(LIS3DH_REG_OUT_X_L, &buffer1);
    LIS3DH_ReadReg(LIS3DH_REG_OUT_X_H, &buffer2);
    X_V = ((buffer2 << 8) | buffer1);
    LIS3DH_ReadReg(LIS3DH_REG_OUT_Y_L, &buffer1);
    LIS3DH_ReadReg(LIS3DH_REG_OUT_Y_H, &buffer2);
    Y_V = ((buffer2 << 8) | buffer1);
    LIS3DH_ReadReg(LIS3DH_REG_OUT_Z_L, &buffer1);
    LIS3DH_ReadReg(LIS3DH_REG_OUT_Z_H, &buffer2);
    Z_V = ((buffer2 << 8) | buffer1);
    //ESP_LOGI(TAG, "LIS3DH_X=%d  LIS3DH_Y=%d  LIS3DH_Z=%d",X_V,Y_V,Z_V);
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

void ntpClockTask(void* pvParameters) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void taskMonitor(void* pvParameters) {
  UBaseType_t uxHighWaterMark;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("--------------------------------------------\r\n");
    uxHighWaterMark = uxTaskGetStackHighWaterMark(adcTask_handle);
    printf("Task: adcTask_handle stacksize=%d\r\n", uxHighWaterMark);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(sw35xxTask_handle);
    printf("Task: sw35xxTask_handle stacksize=%d\r\n", uxHighWaterMark);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(ws28xxTask_handle);
    printf("Task: ws28xxTask_handle stacksize=%d\r\n", uxHighWaterMark);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(lis3dhtask_handle);
    printf("Task: lis3dhtask_handle stacksize=%d\r\n", uxHighWaterMark);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(oledTask_handle);
    printf("Task: oledTask_handle stacksize=%d\r\n", uxHighWaterMark);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(ntpTask_handle);
    printf("Task: ntpTask_handle stacksize=%d\r\n", uxHighWaterMark);
  }
}
