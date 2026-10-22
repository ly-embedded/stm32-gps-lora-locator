#include "inf_lora.h"

/**
 * @brief 初始化
 */
extern void Ebyte_E220x_SetLoraSyncWord(uint16e_t syncWord);
void inf_lora_init(void)
{
    Ebyte_RF.Init();
    Ebyte_E220x_SetLoraSyncWord(0x12);
}

/**
 * @brief发送数据
 */
void inf_lora_send_data(uint8_t *data, uint8_t len)
{
    Ebyte_RF.Send(data, len, 0);
}

/**
 * @brief 接收数据
 */
void inf_lora_rece_data(void)
{
    Ebyte_RF.StartPollTask();
}