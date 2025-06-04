#ifndef BH1750_H
#define BH1750_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <string.h>

#define BH1750_ADDR       (0x23 << 1) // I2C address (ADDR low), shifted for HAL
#define BH1750_POWER_ON   0x01
#define BH1750_RESET      0x07
#define BH1750_CONT_H_RES 0x10 // Continuous H-Resolution Mode (1 lx resolution)

// Device structure
typedef struct {
    I2C_HandleTypeDef *hi2c; // I2C handle
    char name[32];           // Device name
    bool initialized;        // Initialization status
    uint16_t lux;            // Latest lux reading
    void (*poll)(struct BH1750_device_t *dev); // Poll function pointer
} BH1750_device_t;

// Function prototypes
void BH1750_init_i2c(I2C_HandleTypeDef *hi2c);
BH1750_device_t* BH1750_init_dev_struct(I2C_HandleTypeDef *hi2c, const char *name, bool auto_init);
void BH1750_init_dev(BH1750_device_t *dev);
void BH1750_poll(BH1750_device_t *dev);

#endif
