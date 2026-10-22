#ifndef __INF_GPS_H__
#define __INF_GPS_H__

#include "usart.h"
#include "com_debug.h"

/**
 * @brief 初始化
 */
void inf_gps_init(void);

/**
 * @brief 处理接受中断的回调函数
 */
void inf_gps_rx_callback(uint16_t size);

/**
 * @brief 用来读取gps模块接收到的数据
 */
void inf_gps_read_data(uint8_t *data, uint16_t *size);

/**
 * @brief 给GPS模组发送命令
 */
void inf_gps_send_cmd(uint8_t *cmd);

/**
 * @brief gps进入低功耗
 */
void inf_gps_enter_lowpower(void);

/**
 * @brief gps退出低功耗
 */
void inf_gps_leave_lowpower(void);

#endif /* __INF_GPS_H__ */
