#ifndef INC_DHT22_H_
#define INC_DHT22_H_

// Include thư viện HAL chính
#include "stm32f4xx_hal.h"

// Định nghĩa Port và Pin cho DHT22 (phải khớp với User Label trong CubeMX)
#define DHT22_PORT        GPIOA
#define DHT22_PIN         GPIO_PIN_7 // Ví dụ, thay đổi nếu dùng chân khác

// Định nghĩa các mã trạng thái trả về
#define DHT22_OK            0
#define DHT22_ERR_RESPONSE  1 // Lỗi không nhận được phản hồi từ cảm biến
#define DHT22_ERR_CHECKSUM  2 // Lỗi checksum không khớp

// Khai báo biến toàn cục cho Timer Handle (nếu cần truy cập từ bên ngoài)
// extern TIM_HandleTypeDef htim6; // Ví dụ nếu dùng TIM6

// Khai báo các hàm public của driver
void DHT22_Init(TIM_HandleTypeDef *htim); // Hàm khởi tạo, nhận timer handle
uint8_t DHT22_Read_Data(float *Temperature, float *Humidity); // Hàm đọc dữ liệu chính

#endif /* INC_DHT22_H_ */
