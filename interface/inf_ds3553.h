#ifndef __INF_DS3553_H__
#define __INF_DS3553_H__

#include "i2c.h"
#include "gpio.h"

#define DS3553_I2C_ADDR 0x27

#define DS3553_CS_LOW (HAL_GPIO_WritePin(GPIOB, DS3553_CS_Pin, GPIO_PIN_RESET))
#define DS3553_CS_HIGH (HAL_GPIO_WritePin(GPIOB, DS3553_CS_Pin, GPIO_PIN_SET))

#define CHIP_ID_REG_ADDR 0x01
#define USER_SET_REG_ADDR 0xC3
#define STEP_CNT_L_REG_ADDR 0xC4
#define STEP_CNT_M_REG_ADDR 0xC5
#define STEP_CNT_H_REG_ADDR 0xC6

#define DS3553_SET_PWR_MOD_MASK 1 << 7
#define DS3553_SET_SEN_DIS_MASK 1 << 6
#define DS3553_SET_RAISE_EN_MASK 1 << 5
#define DS3553_SET_PULSE_EN_MASK 1 << 4
#define DS3553_SET_NOISE_DIS_MASK 1 << 3
#define DS3553_SET_CLEAR_MASK 1 << 2
#define DS3553_SET_PEDO_1_MASK 1 << 1
#define DS3553_SET_PEDO_0_MASK 1 << 0

/**
 * @brief 初始化
 */
void inf_ds3553_init(void);

/**
 * @brief 读取设备id
 */
uint8_t inf_ds3553_read_id(void);

/**
 * @brief 读取步数
 */
uint32_t inf_ds3553_read_step(void);

#endif /* __INF_DS3553_H__ */
