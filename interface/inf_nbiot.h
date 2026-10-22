#ifndef __INF_NBIOT_H__
#define __INF_NBIOT_H__

#include "usart.h"
#include "com_debug.h"

/**
 * @brief 初始化
 */
void inf_nbiot_init(void);

/**
 * @brief 中断接收的回调函数
 */
void inf_nbiot_rx_callback(uint16_t size);

/**
 * @brief 发送AT指令
 */
void inf_nbiot_send_cmd(uint8_t *cmd);

/**
 * @brief 发送数据给服务器
 */
HAL_StatusTypeDef inf_nbiot_send_server(uint8_t *serverIP, uint16_t serverPort, uint8_t *data, uint16_t len, uint8_t sequence);

/**
 * @brief 进入低功耗
 */
void inf_nbiot_enter_lowpower(void);

/**
 * @brief 退出低功耗
 */
void inf_nbiot_leave_lowpower(void);
#endif /* __INF_NBIOT_H__ */
