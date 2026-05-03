#include "stm32l0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h> // NTC sıcaklık hesaplaması için gerekli matematik kütüphanesi

// --- ÇEVRE BİRİMLERİ ---
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim2;

// --- GLOBAL DEĞİŞKENLER ---
float temperature = 0.0;
uint8_t mpu6050_data[2];
uint8_t motion_detected = 0;
uint8_t system_mode = 0; 
char uart_buf[120];

// Zamanlama ve Buton Takibi İçin
uint32_t last_timer_tick = 0;
uint8_t last_button_state = 0;

// --- FONKSİYON PROTOTİPLERİ ---
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);

int main(void) {
  HAL_Init();
  SystemClock_Config();

  // Tüm çevre birimlerini başlat
  MX_GPIO_Init();
  MX_USART2_UART_Init(); 
  MX_ADC1_Init();        
  MX_I2C1_Init();        
  MX_SPI1_Init();        
  MX_TIM2_Init();        

  // Fan simülasyonu için PWM sinyalini başlat
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

  char *start_msg = "\r\n--- AKILLI ORTAM IZLEME SISTEMI BASLATILDI ---\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)start_msg, strlen(start_msg), 100);

  while (1) {
    // 1. BUTON OKUMA (Sistem Modu Değişimi)
    uint8_t current_button = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    if (current_button == GPIO_PIN_SET && last_button_state == GPIO_PIN_RESET) {
      system_mode = !system_mode; // 0: Oto, 1: Manuel
      HAL_Delay(50); // Buton arkını önle (Debounce)
    }
    last_button_state = current_button;

    // 2. PERİYODİK OKUMA (Saniyede 1 kez çalışır)
    if (HAL_GetTick() - last_timer_tick >= 1000) {
      last_timer_tick = HAL_GetTick();

      // --- SENSÖR OKUMALARI ---
      // ADC ile NTC Sıcaklık Oku
      HAL_ADC_Start(&hadc1);
      if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        
        // NTC Logaritmik Hesaplaması (173 C gibi hataları önler)
        if (adc_val > 0 && adc_val < 4095) {
          temperature = 1.0 / (log(1.0 / (4095.0 / (float)adc_val - 1.0)) / 3950.0 + 1.0 / 298.15) - 273.15;
        }
      }
      HAL_ADC_Stop(&hadc1);

      // I2C ile MPU6050'den Hareket Oku
      HAL_I2C_Mem_Read(&hi2c1, (0x68<<1), 0x3B, 1, mpu6050_data, 2, 100);
      motion_detected = (mpu6050_data[0] > 100) ? 1 : 0;

      // --- İKLİMLENDİRME KONTROL ALGORİTMASI ---
      if (temperature > 30.0) {
        // ÇOK SICAK: Soğutma Aktif
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);   // Yeşil LED YANSIN (Fan)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // Kırmızı LED SÖNSÜN (Isıtıcı Kapalı)
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 800);    // PWM %80 Hız
      } else {
        // NORMAL/SOĞUK: Isıtma Aktif
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); // Yeşil LED SÖNSÜN (Fan Kapalı)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   // Kırmızı LED YANSIN (Isıtıcı)
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 200);    // PWM %20 Hız
      }

      // --- FLOAT YAZDIRMA ÇÖZÜMÜ ---
      int temp_tam = (int)temperature;
      int temp_ondalik = (int)((temperature - temp_tam) * 10);
      if (temp_ondalik < 0) temp_ondalik = -temp_ondalik;

      // --- UART İLE TERMİNALE GÖNDERİM ---
      sprintf(uart_buf, "Mod: %s | Sicaklik: %d.%d C | Hareket: %s\r\n", 
              (system_mode ? "Manuel" : "Oto"), temp_tam, temp_ondalik, (motion_detected ? "Var" : "Yok"));
      HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);

      // --- SPI İLE OLED EKRANA GÖNDERİM ---
      uint8_t spi_data[] = "DATA";
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // CS LOW
      HAL_SPI_Transmit(&hspi1, spi_data, 4, 100);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);   // CS HIGH
    }
  }
}

// --- DONANIM VE PIN AYARLARI ---

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // PB3 (Yeşil LED) ve PB0 (Kırmızı LED) Çıkışları
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // SPI CS Pini (PA4) Çıkışı
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Buton (PA0) Girişi
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART2_UART_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  // PA2 (TX) ve PA3 (RX) Terminal Pinleri
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; 
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Alternate = GPIO_AF4_USART2;   
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.Mode = UART_MODE_TX_RX;
  HAL_UART_Init(&huart2);
}

static void MX_ADC1_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  // PA1 Sıcaklık Sensörü Pini
  GPIO_InitStruct.Pin = GPIO_PIN_1; 
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; 
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  HAL_ADC_Init(&hadc1);
  sConfig.Channel = ADC_CHANNEL_1; 
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00303D5B;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  HAL_I2C_Init(&hi2c1);
}

static void MX_SPI1_Init(void) {
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;   
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;       
  hspi1.Init.NSS = SPI_NSS_SOFT; 
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  HAL_SPI_Init(&hspi1);
}

static void MX_TIM2_Init(void) {
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.Period = 1000;
  HAL_TIM_PWM_Init(&htim2);
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
}

void SystemClock_Config(void) {}
