#include "inf_nbiot.h"

// 定义缓冲区 用来接收 AT指令的回复
#define QS100_BUFF_SIZE 128
static uint8_t QS100RxBuff[QS100_BUFF_SIZE];
static uint16_t QS100RxLen;
/**
 * @brief 初始化
 */
void inf_nbiot_init(void)
{
    // 根据文档得知,设备上电或者唤醒需要至少2-3s再发送AT指令
    HAL_Delay(3000);

    // 开启uart中断接收AT指令的回复
    HAL_StatusTypeDef status;
    while (status != HAL_OK)
    {
        status = HAL_UARTEx_ReceiveToIdle_IT(&huart3, QS100RxBuff, QS100_BUFF_SIZE);
    }

    // 发送AT指令 测试是否OK
    inf_nbiot_send_cmd("AT\r\n");
}

/**
 * @brief 中断接收的回调函数
 */
void inf_nbiot_rx_callback(uint16_t size)
{
    // 拿到真正接收的数据长度
    QS100RxLen = size;

    // 开启uart中断接收AT指令的回复
    HAL_StatusTypeDef status;
    while (status != HAL_OK)
    {
        status = HAL_UARTEx_ReceiveToIdle_IT(&huart3, QS100RxBuff, QS100_BUFF_SIZE);
    }
}

/**
 * @brief 发送AT指令
 */
void inf_nbiot_send_cmd(uint8_t *cmd)
{
    // 每次发送AT指令都会回复消息放在缓冲区,所以我们先把缓冲区给清空一下
    memset(QS100RxBuff, 0, QS100RxLen);
    QS100RxLen = 0;

    // 发送指令
    HAL_UART_Transmit(&huart3, cmd, strlen((char *)cmd), 1000);

    // AT指令有时是耗时的 所以我们可以做一个判断等待
    // 等什么?1. 要么拿到数据  2. 要么超时
    // 获取当前心跳值
    uint32_t start_tick = HAL_GetTick();
    while (QS100RxLen == 0 && (HAL_GetTick() - start_tick) < 3000)
    {
        // 在死循环等待中 加入延迟的意义是:
        // 在未来多任务执行的时候,等待可以让出线程 暂时给其他任务执行
        HAL_Delay(10);
    }

    // 判断并打印
    if (QS100RxLen > 0)
    {
        // 拿到数据了
        debug_printfln("receive [%s] message: \r\n %.*s", cmd, QS100RxLen, QS100RxBuff);
    }
    else
    {
        // 没有拿到数据
        debug_printfln("receive [%s] message timeout!!!!!!", cmd);
    }
}

/**
 * @brief 静态 发送判断是否附着的指令
 */
static HAL_StatusTypeDef inf_nbiot_is_get_ip(void)
{
    // 1. 发送指令
    inf_nbiot_send_cmd("AT+CGATT?\r\n");

    // 2. 判断
    // 如果已经附着,则响应数据中一定有 "+CGATT:1"
    if (strstr((char *)QS100RxBuff, "+CGATT:1"))
    {
        return HAL_OK;
    }

    return HAL_ERROR;
}

/**
 * @brief 静态 开启socket通道(开启服务)
 */
static HAL_StatusTypeDef inf_nbiot_open_socket(uint8_t *socketID)
{
    // 1. 发送AT指令
    inf_nbiot_send_cmd("AT+NSOCR=STREAM,6,0,1\r\n");

    // 2. 检测是否收到应答
    if (strstr((char *)QS100RxBuff, "OK"))
    {
        sscanf((char *)QS100RxBuff, "%*[^:]:%hhd", socketID);
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
 * @brief 连接云端服务器
 */
static HAL_StatusTypeDef inf_nbiot_connect_server(uint8_t socketID, uint8_t *serverIP, uint16_t serverPort)
{
    // 1. 发送指令
    uint8_t cmd[50];
    sprintf((char *)cmd, "AT+NSOCO=%d,%s,%d\r\n", socketID, serverIP, serverPort);
    inf_nbiot_send_cmd(cmd);

    // 2. 判断
    if (strstr((char *)QS100RxBuff, "OK"))
    {
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
 * @brief 发送数据
 */
static HAL_StatusTypeDef inf_nbiot_send_data(uint8_t socketID, uint8_t *data, uint16_t len, uint8_t sequence)
{

    // 1. 发送命令之前-先把数据转为16进制字符串
    uint8_t data_hex[len * 2 + 1];
    for (uint16_t i = 0; i < len; i++)
    {
        sprintf((char *)&data_hex[i * 2], "%02X", data[i]);
    }

    // 2. 拼接cmd
    uint8_t cmd[500];
    sprintf((char *)cmd, "AT+NSOSD=%d,%d,%s,0x200,%d\r\n", socketID, len, data_hex, sequence);

    // 3. 发送数据
    inf_nbiot_send_cmd(cmd);

    // 4. 判断
    if (strstr((char *)QS100RxBuff, "OK"))
    {
        // 如果我们发送成功 以后  我们需要主动去服务器查询当前消息发送是否成功
        uint8_t cmd[50];
        sprintf((char *)cmd, "AT+SEQUENCE=%d,%d\r\n", socketID, sequence);

        // 循环判断是否是发送中,如果是 则继续发送命令查询
        do
        {
            HAL_Delay(10);
            inf_nbiot_send_cmd(cmd);
            // debug_printfln("~~~~~~~~~~~~~%c", QS100RxBuff[2]);
        } while (QS100RxBuff[2] == '2');

        // 直到发送结果不是发送中,发送成功是 1
        if (QS100RxBuff[2] == '1')
        {
            return HAL_OK;
        }
        else
        {
            return HAL_ERROR;
        }
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
 * @brief 发送数据给服务器
 */
HAL_StatusTypeDef inf_nbiot_send_server(uint8_t *serverIP, uint16_t serverPort, uint8_t *data, uint16_t len, uint8_t sequence)
{

    // 先关闭清理所有的socket
    inf_nbiot_send_cmd("AT+NSOCL=0\r\n");
    inf_nbiot_send_cmd("AT+NSOCL=1\r\n");
    HAL_Delay(500);

    uint8_t cnt = 0;
    // 1. 判断是否附着
    // 给他机会,多判断几次(我们可以时间间隔拉长 去循环检测 x次)
    // 要么附着成功 要么超过次数
    cnt = 20;
    while (inf_nbiot_is_get_ip() != HAL_OK && --cnt)
    {
        HAL_Delay(1000);
    }
    // 走到这一步说明:1. 联网成功  2. 超时
    if (cnt == 0)
    {
        debug_printfln("get ip timeout....");
        return HAL_ERROR;
    }
    else
    {
        debug_printfln("get ip success....");
    }

    // 2. 开启socket通道
    uint8_t socketID = 2; // socket默认值是0或1 我们给一个不存在的初始值
    if (inf_nbiot_open_socket(&socketID) == HAL_OK)
    {
        debug_printfln("open socket success, socketID : %d....", socketID);
    }
    else
    {
        debug_printfln("open socket fail....");
    }

    // 3. 链接服务器
    cnt = 20;
    while (inf_nbiot_connect_server(socketID, serverIP, serverPort) != HAL_OK && --cnt)
    {
        HAL_Delay(1000);
    }

    if (cnt == 0)
    {
        debug_printfln("connect server fail....");
        return HAL_ERROR;
    }
    else
    {
        debug_printfln("connect server success....");
    }

    // 4. 发送数据
    cnt = 20;
    while (inf_nbiot_send_data(socketID, data, len, sequence) != HAL_OK && --cnt)
    {
        HAL_Delay(1000);
    }

    if (cnt == 0)
    {
        debug_printfln("send data fail....");
        return HAL_ERROR;
    }
    else
    {
        debug_printfln("send data success....");
    }

    return HAL_OK;
}

/**
 * @brief 进入低功耗
 */
void inf_nbiot_enter_lowpower(void)
{
    debug_printfln("nbiot-enter low power...");
    inf_nbiot_send_cmd("AT+FASTOFF=0\r\n");
}
/**
 * @brief 退出低功耗
 */
void inf_nbiot_leave_lowpower(void)
{
    debug_printfln("nbiot_leave low power...");
    // 软重启(非常粗爆的做法,不建议这么做的....)
    // inf_nbiot_send_cmd("AT+RB\r\n");

    HAL_GPIO_WritePin(NB_WKUP_GPIO_Port, NB_WKUP_Pin, GPIO_PIN_SET);

    HAL_Delay(1);

    HAL_GPIO_WritePin(NB_WKUP_GPIO_Port, NB_WKUP_Pin, GPIO_PIN_RESET);
}