#ifndef __INF_LORA_H__
#define __INF_LORA_H__
#include "ebyte_core.h"
/**
 * @brief 初始化
 */
void inf_lora_init(void);

/**
 * @brief发送数据
 */
void inf_lora_send_data(uint8_t *data, uint8_t len);

/**
 * @brief 接收数据
 */
void inf_lora_rece_data(void);

#endif /* __INF_LORA_H__ */
