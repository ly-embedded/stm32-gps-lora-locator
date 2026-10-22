#ifndef __APP_LOWPOWER_H__
#define __APP_LOWPOWER_H__
#include "inf_gps.h"
#include "inf_nbiot.h"
#include "rtc.h"

/**
 * @brief 进入低功耗
 */
void app_lowpower_enter(void);

/**
 * @brief 退出低功耗
 */
void app_lowpower_leave(void);

#endif /* __APP_LOWPOWER_H__ */
