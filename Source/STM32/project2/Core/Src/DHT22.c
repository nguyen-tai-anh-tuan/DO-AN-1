#include "dht22.h"
#include "main.h" // Bao gồm main.h để sử dụng các định nghĩa từ CubeMX (như DHT22_GPIO_Port, DHT22_Pin nếu dùng define trong main.h)

// Biến cục bộ để lưu Timer Handle
static TIM_HandleTypeDef *dht_timer;

// --- Các hàm nội bộ (static) ---

// Hàm tạo độ trễ micro giây
// Yêu cầu Timer đã được khởi tạo và Start trong main.c hoặc DHT22_Init
static void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(dht_timer, 0); // Reset bộ đếm Timer
    while (__HAL_TIM_GET_COUNTER(dht_timer) < us); // Chờ đến khi bộ đếm đạt giá trị us
}

// Hàm thay đổi chế độ chân GPIO thành Output
static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Output Push-Pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Tốc độ thấp là đủ
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// Hàm thay đổi chế độ chân GPIO thành Input
static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Không cần pull-up/down nội bộ
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// Hàm gửi tín hiệu Start
static void DHT22_Start(void) {
    Set_Pin_Output(DHT22_PORT, DHT22_PIN); // Chuyển sang Output
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET); // Kéo xuống LOW
    HAL_Delay(2); // Giữ LOW ít nhất 1ms (dùng HAL_Delay cho ms) [3, 4]
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET); // Kéo lên HIGH
    delay_us(30); // Giữ HIGH 20-40us [3, 4]
    Set_Pin_Input(DHT22_PORT, DHT22_PIN); // Chuyển sang Input để nhận phản hồi
}

// Hàm kiểm tra tín hiệu phản hồi từ DHT22
static uint8_t DHT22_Check_Response(void) {
    uint8_t Response = 0;
    delay_us(40); // Đợi khoảng 40us sau khi chuyển sang Input [3, 17]
    if (!(HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN))) // Kiểm tra xem cảm biến có kéo xuống LOW không
    {
        delay_us(80); // Đợi hết pha LOW 80us của cảm biến
        if ((HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN))) // Kiểm tra xem cảm biến có kéo lên HIGH không
        {
            Response = 1; // Phản hồi OK
        } else {
            Response = 0; // Lỗi, không kéo lên HIGH sau 80us LOW
        }
    }
    // Đợi cho đến khi kết thúc pha HIGH 80us của cảm biến (chân về LOW)
    while ((HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)));

    return Response;
}

// Hàm đọc một byte (8 bit) từ DHT22
static uint8_t DHT22_Read_Byte(void) {
    uint8_t i, result = 0;
    for (i = 0; i < 8; i++) {
        // Đợi cho đến khi chân lên HIGH (kết thúc pha LOW 50us đầu bit)
        while (!(HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)));

        // Đợi 40us (dài hơn bit 0, ngắn hơn bit 1)
        delay_us(40); // [3, 17]

        if (!(HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN))) // Nếu chân đã xuống LOW -> bit 0
        {
            result &= ~(1 << (7 - i)); // Ghi bit 0 (thực ra chỉ cần dịch trái là đủ nếu khởi tạo = 0)
        } else // Nếu chân vẫn còn HIGH -> bit 1
        {
            result |= (1 << (7 - i)); // Ghi bit 1
            // Đợi cho đến khi chân xuống LOW (kết thúc pha HIGH của bit 1)
            while ((HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)));
        }
         // Nếu là bit 0, chân đã xuống LOW trước đó, không cần đợi thêm
    }
    return result;
}

// --- Các hàm Public ---

// Hàm khởi tạo driver DHT22
void DHT22_Init(TIM_HandleTypeDef *htim) {
    dht_timer = htim; // Lưu con trỏ timer handle
    // Đảm bảo Timer đã được start ở đâu đó (ví dụ trong main)
    // HAL_TIM_Base_Start(dht_timer); // Có thể start ở đây hoặc trong main
}

// Hàm đọc nhiệt độ và độ ẩm
uint8_t DHT22_Read_Data(float *Temperature, float *Humidity) {
	uint8_t data[5];
	uint16_t temp_raw, hum_raw;
	uint8_t checksum_calc;

	DHT22_Start();

	if (DHT22_Check_Response()) {
	    data[0] = DHT22_Read_Byte(); // Humidity integral
	    data[1] = DHT22_Read_Byte(); // Humidity decimal
	    data[2] = DHT22_Read_Byte(); // Temperature integral
	    data[3] = DHT22_Read_Byte(); // Temperature decimal
	    data[4] = DHT22_Read_Byte(); // Checksum

	    checksum_calc = data[0] + data[1] + data[2] + data[3];

	    if (checksum_calc == data[4]) {
	        hum_raw = (data[0] << 8) | data[1];
	        temp_raw = (data[2] << 8) | data[3];

	        *Humidity = hum_raw / 10.0f;

	        if (temp_raw & 0x8000) { // check sign bit
	            temp_raw = temp_raw & 0x7FFF;
	            *Temperature = -(temp_raw / 10.0f);
	        } else {
	            *Temperature = temp_raw / 10.0f;
	        }

	        return DHT22_OK;
	    } else {
	        return DHT22_ERR_CHECKSUM;
	    }
	} else {
	    return DHT22_ERR_RESPONSE;
	}

}

