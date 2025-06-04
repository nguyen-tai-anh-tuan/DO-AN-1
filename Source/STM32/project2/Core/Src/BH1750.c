#include "BH1750.h"

void BH1750_init_i2c(I2C_HandleTypeDef *hi2c) {
    // I2C initialization is handled by CubeMX-generated code (see main.c)
    // This function can be used for additional setup if needed
}

BH1750_device_t* BH1750_init_dev_struct(I2C_HandleTypeDef *hi2c, const char *name, bool auto_init) {
    BH1750_device_t *dev = (BH1750_device_t *)malloc(sizeof(BH1750_device_t));
    if (dev == NULL) return NULL;

    dev->hi2c = hi2c;
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0';
    dev->initialized = false;
    dev->lux = 0;
    dev->poll = BH1750_poll;

    if (auto_init) {
        BH1750_init_dev(dev);
    }

    return dev;
}

void BH1750_init_dev(BH1750_device_t *dev) {
    uint8_t data = BH1750_POWER_ON;
    HAL_I2C_Master_Transmit(dev->hi2c, BH1750_ADDR, &data, 1, HAL_MAX_DELAY);

    data = BH1750_RESET;
    HAL_I2C_Master_Transmit(dev->hi2c, BH1750_ADDR, &data, 1, HAL_MAX_DELAY);

    data = BH1750_CONT_H_RES;
    HAL_I2C_Master_Transmit(dev->hi2c, BH1750_ADDR, &data, 1, HAL_MAX_DELAY);

    dev->initialized = true;
}

void BH1750_poll(BH1750_device_t *dev) {
    if (!dev->initialized) return;

    uint8_t buffer[2];
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(dev->hi2c, BH1750_ADDR, buffer, 2, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        uint16_t raw_lux = (buffer[0] << 8) | buffer[1];
        dev->lux = (uint16_t)(raw_lux / 1.2); // Convert to lux
    }
}
