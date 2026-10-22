#include "app_lowpower.h"

/**
 * @brief 进入低功耗
 */
void app_lowpower_enter(void)
{
    // 1. gps进入低功耗
    inf_gps_enter_lowpower();

    // 2. nbiot进入低功耗
    inf_nbiot_enter_lowpower();

    // 3. 设置RTC闹钟唤醒
    RTC_TimeTypeDef time;
    RTC_AlarmTypeDef alarm;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);

    alarm.AlarmTime.Hours = time.Hours;
    alarm.AlarmTime.Minutes = time.Minutes;
    alarm.AlarmTime.Seconds = time.Seconds + 10;
    HAL_RTC_SetAlarm(&hrtc, &alarm, RTC_FORMAT_BIN);

    // 4. 待机模式下有 待机标志位和唤醒标志位,我们至少要在下次进入待机前清除掉
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    // 4. 主控芯片进入低功耗
    HAL_PWR_EnterSTANDBYMode();
}

/**
 * @brief 退出低功耗
 */
void app_lowpower_leave(void)
{
    // 1. gps退出低功耗
    inf_gps_leave_lowpower();

    // 2. nbiot退出低功耗
    inf_nbiot_leave_lowpower();
}