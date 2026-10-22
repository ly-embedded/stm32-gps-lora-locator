#include "inf_gps.h"
// GPS接收数据缓冲区
#define GPS_RXDATA_BUFF_SIZE 1024
static uint8_t GPSRxData[GPS_RXDATA_BUFF_SIZE];
// GPS接收数据缓冲区长度(实际接收的长度)
static uint16_t GPSRxDataLen;
/**
 * @brief 初始化
 */
void inf_gps_init(void)
{
    // 1. 上电
    HAL_GPIO_WritePin(GPIOB, GPS_EN_Pin, GPIO_PIN_SET);

    // 2. 根据手册的复位时序 需要延迟至少160ms
    HAL_Delay(200);

    // 3. 初始化配置
    inf_gps_send_cmd("PCAS02,1000");

    // 3. 开启中断接收gps数据(只要设备激活,就可以给我们发送数据)
    // 在hal库函数开启通信或者中断的时候,有可能会失败,我们可以判断返回值 是否持续开启
    HAL_StatusTypeDef status;
    while (status != HAL_OK)
    {
        status = HAL_UARTEx_ReceiveToIdle_IT(&huart2, GPSRxData, GPS_RXDATA_BUFF_SIZE);
    }
}

/**
 * @brief 处理接受中断的回调函数
 */
void inf_gps_rx_callback(uint16_t size)
{
    // 1. 把接收不定长数据的中断中 给的实际接收的长度 保存起来
    GPSRxDataLen = size;

    // 2. 再次开启中断接收
    HAL_StatusTypeDef status;
    while (status != HAL_OK)
    {
        status = HAL_UARTEx_ReceiveToIdle_IT(&huart2, GPSRxData, GPS_RXDATA_BUFF_SIZE);
    }
}

/**
 * @brief 用来读取gps模块接收到的数据
 * 我们未来在操作缓冲区的时候, 接收数据可能都会放在同一个缓冲区
 * 我们拿走数据的时候尽量把缓冲区清空(1.清缓冲区和长度  2. 清长度)
 * 保证下次接受放在新的缓冲区 尽量不覆盖数据
 */
void inf_gps_read_data(uint8_t *data, uint16_t *size)
{
    // 1.为了不想覆盖原来的形参data,我们可以做一次清除
    memset(data, 0, strlen((char *)data));
    *size = 0;

    // 2. 判断缓冲区中有没有数据要读取
    if (GPSRxDataLen == 0)
    {
        return;
    }

    // 3. 缓冲区中有gps数据
    // 拿数据和长度
    memcpy(data, GPSRxData, GPSRxDataLen);
    *size = GPSRxDataLen;

    // 4. 清空当前的静态gps数据
    memset(GPSRxData, 0, GPSRxDataLen);
    GPSRxDataLen = 0;
}

/**
 * @brief 给GPS模组发送命令
 */
void inf_gps_send_cmd(uint8_t *cmd)
{
    // 1. 定义一个变量 求cmd的异或值
    uint8_t temp = cmd[0];
    for (uint16_t i = 1; cmd[i] != '\0'; i++)
    {
        temp ^= cmd[i];
    }

    // 2. 拼接命令
    uint8_t temCmd[100];
    sprintf((char *)temCmd, "$%s*%02X\r\n", cmd, temp);

    // 3. 发送
    debug_printfln("cmd :%s", temCmd);
    HAL_UART_Transmit(&huart2, temCmd, strlen((char *)temCmd), 1000);
}

/**
 * @brief gps进入低功耗
 */
void inf_gps_enter_lowpower(void)
{
    HAL_GPIO_WritePin(GPIOB, GPS_EN_Pin, GPIO_PIN_RESET);
}

/**
 * @brief gps退出低功耗
 */
void inf_gps_leave_lowpower(void)
{
    // 1. 上电
    HAL_GPIO_WritePin(GPIOB, GPS_EN_Pin, GPIO_PIN_SET);

    // 2. 根据手册的复位时序 需要延迟至少160ms
    HAL_Delay(200);
}
