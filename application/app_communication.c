#include "app_communication.h"
static UpdateData_Type updateData;
static uint8_t gps_data[512];
static uint16_t gps_data_len = 0;
/**
 * @brief 初始化
 */
void app_communication_init(void)
{
    inf_ds3553_init();
    inf_gps_init();
    inf_nbiot_init();
    inf_lora_init();
}

/**
 * @brief 处理数据并发送给云端
 */
void app_communication_send(void)
{
    char *gnrmc = NULL;
    // 1. 先拿到gps数据的($GNRMC 最小推荐导航数据) 中的 经纬度 时间
    // 1.1 gps数据唤醒后可能需要时间才能拿到,所以要循环读取,直到拿到为止
    while (1)
    {
        // 1.2 读取数据
        inf_gps_read_data(gps_data, &gps_data_len);
        // 1.3 判断是否拿到了数据
        if (gps_data_len > 0)
        {
            // 1.4 从gps回复的数据中拿到我们需要的$GNRMC
            // gnrmc = strstr((char *)gps_data, "$GNRMC");
            gnrmc = "$GNRMC,154919.000,A,3028.09110,N,11423.29505,E,3.44,156.11,060525,,,A,V*00";

            // 1.5 判断数据是否有效,如果有效就直接退出循环,如果无效,则不处理,继续循环
            char flag = '\0';
            sscanf(gnrmc, "%*[^AV]%c", &flag);

            if (flag == 'A')
            {
                debug_printfln("data valid....");
                break;
            }
            else
            {
                debug_printfln("data invalid....");
            }
        }

        HAL_Delay(1000);
    }

    // 1.6 数据解析
    uint8_t time[7];
    uint8_t date[7];
    sscanf(
        gnrmc,
        "$GNRMC,%6c%*7c%f,%c,%f,%c,%*f,%*f,%6c",
        time,
        &updateData.lat,
        updateData.latDir,
        &updateData.lon,
        updateData.lonDir,
        date);

    // 3028.09110

    // 处理经纬度
    updateData.lat = (int)updateData.lat / 100 + (updateData.lat - (int)(updateData.lat / 100) * 100) / 60;
    updateData.lon = (int)updateData.lon / 100 + (updateData.lon - (int)(updateData.lon / 100) * 100) / 60;

    // 处理时间
    sprintf(
        updateData.datetime,
        "20%c%c-%c%c-%c%c %c%c:%c%c:%c%c",
        date[4],
        date[5],
        date[2],
        date[3],
        date[0],
        date[1],
        time[0],
        time[1],
        time[2],
        time[3],
        time[4],
        time[5]);

    // 打印刚才的数据

    // 2. 拿到步数
    updateData.step = inf_ds3553_read_step();
    debug_printfln("time:%s", time);
    debug_printfln("lat:%f", updateData.lat);
    debug_printfln("latDir:%s", updateData.latDir);
    debug_printfln("lon:%f", updateData.lon);
    debug_printfln("lonDir:%s", updateData.lonDir);
    debug_printfln("date:%s", updateData.datetime);
    debug_printfln("step:%d", updateData.step);

    // 3. 把收集到的数据转为json字符串格式
    // 3.1 创建一个json对象
    cJSON *root = cJSON_CreateObject();
    // 3.2 给json对象添加数据
    cJSON_AddNumberToObject(root, "lat", updateData.lat);
    cJSON_AddStringToObject(root, "latDir", updateData.latDir);
    cJSON_AddNumberToObject(root, "lon", updateData.lon);
    cJSON_AddStringToObject(root, "lonDir", updateData.lonDir);
    cJSON_AddStringToObject(root, "datetime", updateData.datetime);
    cJSON_AddNumberToObject(root, "step", updateData.step);

    // 3.3 把json对象转为字符串
    char *jsonStr = cJSON_PrintUnformatted(root);

    printf("jsonStr:%s\n", jsonStr);

    // 4. 使用NB-IoT发送给云端
    /* HAL_StatusTypeDef result = inf_nbiot_send_server("8.135.10.183", 37410, (uint8_t *)jsonStr, strlen(jsonStr), 1);

    // 5. 判断是否发送成功
    if (result == HAL_OK)
    {
        debug_printfln("send success....");
    }
    else
    {
        // 发送不成功, 把数据发送给lora网关
        inf_lora_send_data((uint8_t *)jsonStr, strlen(jsonStr));
    } */

    inf_lora_send_data((uint8_t *)jsonStr, strlen(jsonStr));

    // 6. 释放cjson的空间
    cJSON_Delete(root);
    free(jsonStr);
}
