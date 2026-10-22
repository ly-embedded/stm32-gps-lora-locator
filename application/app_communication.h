#ifndef __APP_COMMUNICATION_H__
#define __APP_COMMUNICATION_H__
#include "inf_ds3553.h"
#include "inf_gps.h"
#include "inf_nbiot.h"
#include "cJSON.h"
#include "stdlib.h"
#include "inf_lora.h"
/**
 * 定义结构体类型 用来存放数据
 */
typedef struct
{
    // 纬度
    float lat;
    // 纬度方向
    char latDir[2];
    // 经度
    float lon;
    // 经度方向
    char lonDir[2];

    // 规范时间格式
    char datetime[20];
    // 步数
    uint32_t step;

} UpdateData_Type;

/**
 * @brief 初始化
 */
void app_communication_init(void);

/**
 * @brief 处理数据并发送给云端
 */
void app_communication_send(void);

#endif /* __APP_COMMUNICATION_H__ */
