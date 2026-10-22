#include "inf_ds3553.h"

/**
 * @brief 静态 修改配置寄存器
 */
static void inf_ds3553_write_set(uint8_t set_mask, FlagStatus value)
{
    uint8_t set_value = 0;

    // 1. 拉低片选
    DS3553_CS_LOW;
    HAL_Delay(3);

    // 2. 读取配置寄存器的值
    HAL_I2C_Mem_Read(&hi2c1, DS3553_I2C_ADDR << 1, USER_SET_REG_ADDR, I2C_MEMADD_SIZE_8BIT, &set_value, 1, 1000);

    // 3. 判断并得到写入设置的值
    if (value == SET)
    {
        set_value |= set_mask;
    }
    else
    {
        set_value &= ~set_mask;
    }

    // 4.写入
    HAL_I2C_Mem_Write(&hi2c1, DS3553_I2C_ADDR << 1, USER_SET_REG_ADDR, I2C_MEMADD_SIZE_8BIT, &set_value, 1, 1000);

    // 5. 拉高片选
    DS3553_CS_HIGH;
    HAL_Delay(10);
}
/**
 * @brief 静态 读寄存的数据
 */
static uint8_t inf_ds3553_read_reg(uint8_t reg_addr)
{
    // 1. 定义一个返回值
    uint8_t value = 0;

    // 2. 拉低片选线
    DS3553_CS_LOW;

    // 3. CS为低之后 要延迟3ms以上
    HAL_Delay(3);

    // 4. 读取数据
    HAL_I2C_Mem_Read(&hi2c1, DS3553_I2C_ADDR << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);

    // 5. 拉高CS
    DS3553_CS_HIGH;

    // 6. CS为高以后 需要延迟10ms以后才能开始下一次通信
    HAL_Delay(10);

    return value;
}

/**
 * @brief 初始化
 */
void inf_ds3553_init(void)
{
    // 禁止计步脉冲输出
    inf_ds3553_write_set(DS3553_SET_PULSE_EN_MASK, RESET);

    // 禁止去噪功能
    inf_ds3553_write_set(DS3553_SET_NOISE_DIS_MASK, SET);
}

/**
 * @brief 读取设备id
 */
uint8_t inf_ds3553_read_id(void)
{
    return inf_ds3553_read_reg(CHIP_ID_REG_ADDR);
}

/**
 * @brief 读取步数
 */
uint32_t inf_ds3553_read_step(void)
{
    uint32_t step = 0;

    step |= inf_ds3553_read_reg(STEP_CNT_L_REG_ADDR);
    step |= (inf_ds3553_read_reg(STEP_CNT_M_REG_ADDR) << 8);
    step |= (inf_ds3553_read_reg(STEP_CNT_H_REG_ADDR) << 16);

    return step;
}
