/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_lcd.h"
#include "BH1750.h"
#include <stdio.h>
#include <string.h>

#include "dht22.h"
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart6;

I2C_LCD_HandleTypeDef lcd1;

volatile uint32_t msTicks;
float RH, TEMP;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef enum {
  MODE_INTRO = 0,
  MODE_AUTH = 1,
  MODE_SENSOR = 2,
  MODE_SETUP = 3,
  MODE_VERIFY_SETUP = 4
} DisplayMode;

DisplayMode currentMode = MODE_INTRO;
uint8_t menuPosition = 0;
uint8_t setupMenuPosition = 0;

uint8_t global_status;
BH1750_device_t *global_test_dev;
uint8_t rx_char;

char password_display[7] = "";
uint8_t password_length = 0;
bool is_entering_password = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Display_Intro(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 1, 0);
  lcd_puts(&lcd1, "HE THONG GIAM SAT");
  lcd_gotoxy(&lcd1, 3, 1);
  lcd_puts(&lcd1, "NHA THONG MINH");
  lcd_gotoxy(&lcd1, 1, 2);
  lcd_puts(&lcd1, "NGUYEN TAI ANH TUAN");
  lcd_gotoxy(&lcd1, 3, 3);
  lcd_puts(&lcd1, "DH SPKT TP.HCM");
}

void Display_Auth(uint8_t position) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "CHE DO XAC THUC");
  
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, position == 0 ? "> KEYPAD4x4" : "  KEYPAD4x4");
  
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, position == 1 ? "> THE RFID" : "  THE RFID");

  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, position == 2 ? "> VAN TAY" : "  VAN TAY");
}

void Display_Sensors(float temp, float humidity, uint32_t gas, uint32_t light) {
  lcd_clear(&lcd1);
  
  char buffer[20];
  
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "THONG TIN CAM BIEN");
  
  lcd_gotoxy(&lcd1, 0, 1);
  snprintf(buffer, sizeof(buffer), "NHIET DO: %d.%01d", (int)temp, (int)(temp * 10) % 10);
  lcd_puts(&lcd1, buffer);
  lcd_send_data(&lcd1, 0xDF);
  lcd_send_data(&lcd1, 'C');
  
  lcd_gotoxy(&lcd1, 0, 2);
  snprintf(buffer, sizeof(buffer), "DO AM: %d.%01d%%", (int)humidity, (int)(humidity * 10) % 10);
  lcd_puts(&lcd1, buffer);
  
  lcd_gotoxy(&lcd1, 0, 3);
  snprintf(buffer, sizeof(buffer), "GAS:%u  LUX:%u", gas, light);
  lcd_puts(&lcd1, buffer);
}

void Display_Password_Entry(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "NHAP MAT KHAU:");
  
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "MAT KHAU: ");
  
  lcd_gotoxy(&lcd1, 10, 1);
  for (int i = 0; i < password_length; i++) {
    lcd_send_data(&lcd1, '*');
  }
  
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, "# XAC NHAN");
  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, "* XOA/HUY");
}

void Display_Setup_Verify(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "XAC THUC CAI DAT");
  
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "MAT KHAU: ");
  
  lcd_gotoxy(&lcd1, 10, 1);
  for (int i = 0; i < password_length; i++) {
    lcd_send_data(&lcd1, '*');
  }
  
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, "# XAC NHAN");
  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, "* XOA/HUY");
}

void Servo_SetAngle(uint8_t angle);
bool status_open = false;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    switch(rx_char) {
      case 'j':
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
        break;
      case 'k':
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
      case 'g':
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
        break;
      case 'h':
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
        break;
      case 'd':
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET);
        break;
      case 'f':
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET);
        break;
      case 'a':
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);
        break;
      case 's':
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET);
        break;
      case 'A':
        currentMode = MODE_AUTH;
        menuPosition = 0;
        is_entering_password = false;
        Display_Auth(menuPosition);
        break;
        
      case 'B':
        if (currentMode == MODE_AUTH && !is_entering_password) {
          menuPosition = (menuPosition + 1) % 3;
          Display_Auth(menuPosition);
        }
        else if (currentMode == MODE_SETUP) {
          setupMenuPosition = (setupMenuPosition + 1) % 3;
          Display_Setup(setupMenuPosition);
        }
        break;
        
      case '#':
        if (currentMode == MODE_AUTH && !is_entering_password) {
          lcd_gotoxy(&lcd1, 0, 3);
          switch(menuPosition) {
            case 0:
              lcd_puts(&lcd1, "XAC THUC KEYPAD...");
              break;
            case 1:
              lcd_puts(&lcd1, "XAC THUC RFID...");
              break;
            case 2:
              lcd_puts(&lcd1, "XAC THUC VAN TAY...");
              break;
          }
        }
        else if (currentMode == MODE_SETUP) {
          lcd_gotoxy(&lcd1, 0, 3);
          switch(setupMenuPosition) {
            case 0:
              lcd_puts(&lcd1, "SETUP MAT KHAU...");
              break;
            case 1:
              lcd_puts(&lcd1, "SETUP VAN TAY...");
              break;
            case 2:
              lcd_puts(&lcd1, "SETUP RFID...");
              break;
          }
        }
        break;
        
      case 'C':
        currentMode = MODE_SENSOR;
        is_entering_password = false;
        break;
        
      case 'S':
        currentMode = MODE_SETUP;
        setupMenuPosition = 0;
        is_entering_password = false;
        Display_Setup(setupMenuPosition);
        break;
        
      case 'I':
        currentMode = MODE_INTRO;
        is_entering_password = false;
        Display_Intro();
        break;
        
      case 'W':
        currentMode = MODE_VERIFY_SETUP;
        is_entering_password = true;
        password_length = 0;
        memset(password_display, 0, sizeof(password_display));
        Display_Setup_Verify();
        break;
        
      case 'P':
        is_entering_password = true;
        password_length = 0;
        memset(password_display, 0, sizeof(password_display));
        Display_Password_Entry();
        break;
        
      case 'N':
        is_entering_password = true;
        password_length = 0;
        memset(password_display, 0, sizeof(password_display));
        Display_Setup_Password();
        break;
        
      case 'R':
        is_entering_password = false;
        lcd_clear(&lcd1);
        lcd_gotoxy(&lcd1, 0, 0);
        lcd_puts(&lcd1, "XAC THUC RFID");
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, "VUI LONG QUET THE...");
        lcd_gotoxy(&lcd1, 0, 3);
        lcd_puts(&lcd1, "* DE HUY");
        break;
        
      case 'T':
        is_entering_password = false;
        Display_Setup_RFID();
        break;
        
      case 'V':
        is_entering_password = false;
        lcd_clear(&lcd1);
        lcd_gotoxy(&lcd1, 0, 0);
        lcd_puts(&lcd1, "XAC THUC VAN TAY");
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, "DAT NGON TAY...");
        lcd_gotoxy(&lcd1, 0, 3);
        lcd_puts(&lcd1, "* DE HUY");
        break;
        
      case 'G':
        is_entering_password = false;
        Display_Setup_Fingerprint();
        break;
        
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (is_entering_password && password_length < 6) {
          password_display[password_length] = rx_char;
          password_length++;
          if (currentMode == MODE_SETUP) {
            Display_Setup_Password();
          } else if (currentMode == MODE_VERIFY_SETUP) {
            Display_Setup_Verify();
          } else {
            Display_Password_Entry();
          }
        }
        break;
        
      case 'X':
        if (is_entering_password && password_length > 0) {
          password_length--;
          password_display[password_length] = '\0';
          if (currentMode == MODE_SETUP) {
            Display_Setup_Password();
          } else if (currentMode == MODE_VERIFY_SETUP) {
            Display_Setup_Verify();
          } else {
            Display_Password_Entry();
          }
        }
        break;
        
      case 'O':
        status_open = true;
        is_entering_password = false;
        lcd_clear(&lcd1);
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, "XAC THUC THANH CONG");
        lcd_gotoxy(&lcd1, 0, 2);
        lcd_puts(&lcd1, "DANG MO CUA...");
        break;
        
      case 'Y':
        is_entering_password = false;
        Display_Success("CAI DAT THANH CONG");
        break;
        
      case 'F':
        if (is_entering_password) {
          if (currentMode == MODE_VERIFY_SETUP) {
            is_entering_password = false;
            Display_Error("SAI MAT KHAU!");
          } else {
            lcd_gotoxy(&lcd1, 0, 2);
            lcd_puts(&lcd1, "SAI MAT KHAU!     ");
            lcd_gotoxy(&lcd1, 0, 3);
            lcd_puts(&lcd1, "THU LAI...        ");
            password_length = 0;
            memset(password_display, 0, sizeof(password_display));
          }
        }
        break;
        
      case 'E':
        is_entering_password = false;
        Display_Error("CAI DAT THAT BAI");
        break;
        
      case 'U':
        is_entering_password = false;
        lcd_clear(&lcd1);
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, "XAC THUC THAT BAI!");
        lcd_gotoxy(&lcd1, 0, 2);
        lcd_puts(&lcd1, "THE KHONG HOP LE");
        break;
        
      case 'Z':
        is_entering_password = false;
        lcd_clear(&lcd1);
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, "SAI VAN TAY!");
        lcd_gotoxy(&lcd1, 0, 2);
        lcd_puts(&lcd1, "THU LAI...");
        break;
    }
    
    HAL_UART_Receive_IT(&huart6, &rx_char, 1);
  }
}

void Servo_SetAngle(uint8_t angle) {
    if (angle > 180) angle = 180;
    uint16_t pulse = 500 + (angle * 2000) / 180;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse);
}

void Display_Setup(uint8_t position) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "CHE DO CAI DAT");
  
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, position == 0 ? "> MAT KHAU MOI" : "  MAT KHAU MOI");
  
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, position == 1 ? "> VAN TAY MOI" : "  VAN TAY MOI");

  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, position == 2 ? "> THE RFID MOI" : "  THE RFID MOI");
}

void Display_Setup_Password(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "DAT MAT KHAU MOI:");
  
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "MAT KHAU: ");
  
  lcd_gotoxy(&lcd1, 10, 1);
  for (int i = 0; i < password_length; i++) {
    lcd_send_data(&lcd1, '*');
  }
  
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, "# XAC NHAN");
  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, "* XOA/HUY");
}

void Display_Setup_Fingerprint(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "DANG KY VAN TAY");
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "DAT NGON TAY...");
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, "# DE BAT DAU");
  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, "* DE HUY");
}

void Display_Setup_RFID(void) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "DANG KY THE RFID");
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "VUI LONG QUET THE...");
  lcd_gotoxy(&lcd1, 0, 3);
  lcd_puts(&lcd1, "* DE HUY");
}

void Display_Success(char* message) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "THANH CONG!");
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, message);
}

void Display_Error(char* message) {
  lcd_clear(&lcd1);
  lcd_gotoxy(&lcd1, 0, 1);
  lcd_puts(&lcd1, "LOI!");
  lcd_gotoxy(&lcd1, 0, 2);
  lcd_puts(&lcd1, message);
}

void Send_Sensor_Data_To_ESP32(float temp, float humidity, uint32_t gas, uint32_t light) {
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "SENSOR:%d,%d,%lu,%lu\n", (int)temp, (int)humidity, gas, light);
  HAL_UART_Transmit(&huart6, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  BH1750_init_i2c(&hi2c1);
  BH1750_device_t *test_dev = BH1750_init_dev_struct(&hi2c1, "test device", true);
  global_test_dev = test_dev;
  HAL_TIM_Base_Start(&htim6);
  DHT22_Init(&htim6);
  lcd1.hi2c = &hi2c1;
  lcd1.address = 0x4e;
  lcd_init(&lcd1);

  Display_Intro();
  
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  
  HAL_UART_Receive_IT(&huart6, &rx_char, 1);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
    if (status_open) {
      Servo_SetAngle(180);
      HAL_Delay(3000); 
      Servo_SetAngle(0);
      status_open = false;
      
      
      currentMode = MODE_INTRO;
      is_entering_password = false;
      Display_Intro();
    }
    
 
    if (currentMode == MODE_SENSOR) {
      float temp = 0, hum = 0;
      uint8_t retry = 0;
      uint8_t dht_status = DHT22_ERR_RESPONSE;
      
      while (retry < 3 && dht_status != DHT22_OK) {
        dht_status = DHT22_Read_Data(&temp, &hum);
        retry++;
        if (dht_status != DHT22_OK) {
          HAL_Delay(500);
        }
      }
      
      char debug_buffer[50];
      snprintf(debug_buffer, sizeof(debug_buffer), "DHT22: Status=%d, Temp=%.2f, Hum=%.2f\n", dht_status, temp, hum);
      HAL_UART_Transmit(&huart6, (uint8_t*)debug_buffer, strlen(debug_buffer), HAL_MAX_DELAY);
      
      uint32_t mq2_value = 0;
      HAL_ADC_Start(&hadc1);
      HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
      mq2_value = HAL_ADC_GetValue(&hadc1);
      HAL_ADC_Stop(&hadc1);
      
      global_test_dev->poll(global_test_dev);
      
      Display_Sensors(temp, hum, mq2_value, global_test_dev->lux);
      
      if (dht_status == DHT22_OK && temp != 0.0 && hum != 0.0) {
        Send_Sensor_Data_To_ESP32(temp, hum, mq2_value, global_test_dev->lux);
      } else {
        char error_msg[30];
        snprintf(error_msg, sizeof(error_msg), "DHT22_ERROR: Status=%d\n", dht_status);
        HAL_UART_Transmit(&huart6, (uint8_t*)error_msg, strlen(error_msg), HAL_MAX_DELAY);
      }
    }
    
    HAL_Delay(500);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
