#include "ebyte_e220x.h"

/* ������Ϣ E220x���� !��Դ 32M���� */
#define XTAL_FREQ (double)32000000
#define FREQ_DIV (double)33554432 // 2��25�η�
#define FREQ_STEP (double)(XTAL_FREQ / FREQ_DIV)
#define XTA_TRIM_VALUE 0x1C // XTAL�������ڲ�������

/* LoRa ǰ����֡��ʽ��ز��� */
#define LORA_PREAMBLE_LENGTH 60

#define CRC_IBM_SEED 0xFFFF
#define CRC_CCITT_SEED 0x1D0F
#define CRC_POLYNOMIAL_IBM 0x8005
#define CRC_POLYNOMIAL_CCITT 0x1021

#define REG_LR_CRCSEEDBASEADDR 0x06BC
#define REG_LR_CRCPOLYBASEADDR 0x06BE
#define REG_LR_WHITSEEDBASEADDR_MSB 0x06B8
#define REG_LR_WHITSEEDBASEADDR_LSB 0x06B9
#define REG_LR_PACKETPARAMS 0x0704
#define REG_LR_PAYLOADLENGTH 0x0702
#define REG_LR_SYNCWORDBASEADDRESS 0x06C0
#define REG_LR_SYNCWORD 0x0740

#define RANDOM_NUMBER_GENERATORBASEADDR 0x0819

#define REG_RX_GAIN 0x08AC
#define REG_XTA_TRIM 0x0911
#define REG_XTB_TRIM 0x0912
#define REG_OCP 0x08E7

#define RADIO_GET_STATUS 0xC0
#define RADIO_WRITE_REGISTER 0x0D
#define RADIO_READ_REGISTER 0x1D
#define RADIO_WRITE_BUFFER 0x0E
#define RADIO_READ_BUFFER 0x1E
#define RADIO_SET_SLEEP 0x84
#define RADIO_SET_STANDBY 0x80
#define RADIO_SET_FS 0xC1
#define RADIO_SET_TX 0x83
#define RADIO_SET_RX 0x82
#define RADIO_SET_RXDUTYCYCLE 0x94
#define RADIO_SET_CAD 0xC5
#define RADIO_SET_TXCONTINUOUSWAVE 0xD1
#define RADIO_SET_TXCONTINUOUSPREAMBLE 0xD2
#define RADIO_SET_PACKETTYPE 0x8A
#define RADIO_GET_PACKETTYPE 0x11
#define RADIO_SET_RFFREQUENCY 0x86
#define RADIO_SET_TXPARAMS 0x8E
#define RADIO_SET_PACONFIG 0x95
#define RADIO_SET_CADPARAMS 0x88
#define RADIO_SET_BUFFERBASEADDRESS 0x8F
#define RADIO_SET_MODULATIONPARAMS 0x8B
#define RADIO_SET_PACKETPARAMS 0x8C
#define RADIO_GET_RXBUFFERSTATUS 0x13
#define RADIO_GET_PACKETSTATUS 0x14
#define RADIO_GET_RSSIINST 0x15
#define RADIO_GET_STATS 0x10
#define RADIO_RESET_STATS 0x00
#define RADIO_CFG_DIOIRQ 0x08
#define RADIO_GET_IRQSTATUS 0x12
#define RADIO_CLR_IRQSTATUS 0x02
#define RADIO_CALIBRATE 0x89
#define RADIO_CALIBRATEIMAGE 0x98
#define RADIO_SET_REGULATORMODE 0x96
#define RADIO_GET_ERROR 0x17
#define RADIO_CLR_ERROR 0x07
#define RADIO_SET_TCXOMODE 0x97
#define RADIO_SET_TXFALLBACKMODE 0x93
#define RADIO_SET_RFSWITCHMODE 0x9D
#define RADIO_SET_STOPRXTIMERONPREAMBLE 0x9F
#define RADIO_SET_LORASYMBTIMEOUT 0xA0

#define CHIP_MODE_STBY_RC 0x02
#define CHIP_MODE_STBY_XOSC 0x03
#define CHIP_MODE_FS 0x04
#define CHIP_MODE_RX 0x05
#define CHIP_MODE_X 0x06

/* ����ʶ��ģ����� */
#define EBYTE_E220_NAME_TYPE 0x220

/* ����ʶ��ģ��Ƶ�� */
#if defined(EBYTE_E220_400M22S)
#define EBYTE_E220_FREQUENCY_TYPE 0x400
#elif defined(EBYTE_E220_900M22S)
#define EBYTE_E220_FREQUENCY_TYPE 0x900

#endif

/* ����ʶ����������汾�� */
#define EBYTE_E220_PROGRAM_TYPE 0x10

typedef enum
{
    MODE_SLEEP = 0x00, //! The radio is in sleep mode
    MODE_STDBY_RC,     //! The radio is in standby mode with RC oscillator
    MODE_STDBY_XOSC,   //! The radio is in standby mode with XOSC oscillator
    MODE_FS,           //! The radio is in frequency synthesis mode
    MODE_TX,           //! The radio is in transmit mode
    MODE_RX,           //! The radio is in receive mode
    MODE_RX_DC,        //! The radio is in receive duty cycle mode
    MODE_CAD           //! The radio is in channel activity detection mode
} RadioOperatingModes_t;

typedef union RadioStatus_u
{
    unsigned char Value;
    struct
    {                                // bit order is lsb -> msb
        unsigned char Reserved : 1;  //!< Reserved
        unsigned char CmdStatus : 3; //!< Command status
        unsigned char ChipMode : 3;  //!< Chip mode
        unsigned char CpuBusy : 1;   //!< Flag for CPU radio busy
    } Fields;
} RadioStatus_t;

typedef enum
{
    IRQ_HEADER_ERROR_CODE = 0x01,
    IRQ_SYNCWORD_ERROR_CODE = 0x02,
    IRQ_CRC_ERROR_CODE = 0x04,
} IrqErrorCode_t;

enum IrqPblSyncHeaderCode_t
{
    IRQ_PBL_DETECT_CODE = 0x01,
    IRQ_SYNCWORD_VALID_CODE = 0x02,
    IRQ_HEADER_VALID_CODE = 0x04,
};

typedef enum
{
    STDBY_RC = 0x00,
    STDBY_XOSC = 0x01,
} RadioStandbyModes_t;

typedef enum
{
    PACKET_TYPE_GFSK = 0x00,
    PACKET_TYPE_LORA = 0x01,
    PACKET_TYPE_NONE = 0x0F,
} RadioPacketTypes_t;

typedef enum
{
    RADIO_RAMP_10_US = 0x00,
    RADIO_RAMP_20_US = 0x01,
    RADIO_RAMP_40_US = 0x02,
    RADIO_RAMP_80_US = 0x03,
    RADIO_RAMP_200_US = 0x04,
    RADIO_RAMP_800_US = 0x05,
    RADIO_RAMP_1700_US = 0x06,
    RADIO_RAMP_3400_US = 0x07,
} RadioRampTimes_t;

typedef enum
{
    LORA_CAD_01_SYMBOL = 0x00,
    LORA_CAD_02_SYMBOL = 0x01,
    LORA_CAD_04_SYMBOL = 0x02,
    LORA_CAD_08_SYMBOL = 0x03,
    LORA_CAD_16_SYMBOL = 0x04,
} RadioLoRaCadSymbols_t;

typedef enum
{
    LORA_CAD_ONLY = 0x00,
    LORA_CAD_RX = 0x01,
    LORA_CAD_LBT = 0x10,
} RadioCadExitModes_t;

typedef enum
{
    MOD_SHAPING_OFF = 0x00,
    MOD_SHAPING_G_BT_03 = 0x08,
    MOD_SHAPING_G_BT_05 = 0x09,
    MOD_SHAPING_G_BT_07 = 0x0A,
    MOD_SHAPING_G_BT_1 = 0x0B,
} RadioModShapings_t;

typedef enum
{
    RX_BW_4800 = 0x1F,
    RX_BW_5800 = 0x17,
    RX_BW_7300 = 0x0F,
    RX_BW_9700 = 0x1E,
    RX_BW_11700 = 0x16,
    RX_BW_14600 = 0x0E,
    RX_BW_19500 = 0x1D,
    RX_BW_23400 = 0x15,
    RX_BW_29300 = 0x0D,
    RX_BW_39000 = 0x1C,
    RX_BW_46900 = 0x14,
    RX_BW_58600 = 0x0C,
    RX_BW_78200 = 0x1B,
    RX_BW_93800 = 0x13,
    RX_BW_117300 = 0x0B,
    RX_BW_156200 = 0x1A,
    RX_BW_187200 = 0x12,
    RX_BW_234300 = 0x0A,
    RX_BW_312000 = 0x19,
    RX_BW_373600 = 0x11,
    RX_BW_467000 = 0x09,
} RadioRxBandwidth_t;

typedef enum
{
    LORA_SF5 = 0x05,
    LORA_SF6 = 0x06,
    LORA_SF7 = 0x07,
    LORA_SF8 = 0x08,
    LORA_SF9 = 0x09,
    LORA_SF10 = 0x0A,
    LORA_SF11 = 0x0B,
    LORA_SF12 = 0x0C,
} RadioLoRaSpreadingFactors_t;

typedef enum
{
    LORA_BW_500 = 6,
    LORA_BW_250 = 5,
    LORA_BW_125 = 4,
    LORA_BW_062 = 3,
    LORA_BW_041 = 10,
    LORA_BW_031 = 2,
    LORA_BW_020 = 9,
    LORA_BW_015 = 1,
    LORA_BW_010 = 8,
    LORA_BW_007 = 0,
} RadioLoRaBandwidths_t;

typedef enum
{
    LORA_CR_4_5 = 0x01,
    LORA_CR_4_6 = 0x02,
    LORA_CR_4_7 = 0x03,
    LORA_CR_4_8 = 0x04,
} RadioLoRaCodingRates_t;

typedef enum
{
    RADIO_PREAMBLE_DETECTOR_OFF = 0x00,     //!< Preamble detection length off
    RADIO_PREAMBLE_DETECTOR_08_BITS = 0x04, //!< Preamble detection length 8 bits
    RADIO_PREAMBLE_DETECTOR_16_BITS = 0x05, //!< Preamble detection length 16 bits
    RADIO_PREAMBLE_DETECTOR_24_BITS = 0x06, //!< Preamble detection length 24 bits
    RADIO_PREAMBLE_DETECTOR_32_BITS = 0x07, //!< Preamble detection length 32 bit
} RadioPreambleDetection_t;

typedef enum
{
    RADIO_ADDRESSCOMP_FILT_OFF = 0x00, //!< No correlator turned on, i.e. do not search for SyncWord
    RADIO_ADDRESSCOMP_FILT_NODE = 0x01,
    RADIO_ADDRESSCOMP_FILT_NODE_BROAD = 0x02,
} RadioAddressComp_t;

typedef enum
{
    RADIO_PACKET_FIXED_LENGTH = 0x00,    //!< The packet is known on both sides, no header included in the packet
    RADIO_PACKET_VARIABLE_LENGTH = 0x01, //!< The packet is on variable size, header included
} RadioPacketLengthModes_t;

typedef enum
{
    RADIO_CRC_OFF = 0x01, //!< No CRC in use
    RADIO_CRC_1_BYTES = 0x00,
    RADIO_CRC_2_BYTES = 0x02,
    RADIO_CRC_1_BYTES_INV = 0x04,
    RADIO_CRC_2_BYTES_INV = 0x06,
    RADIO_CRC_2_BYTES_IBM = 0xF1,
    RADIO_CRC_2_BYTES_CCIT = 0xF2,
} RadioCrcTypes_t;

typedef enum
{
    RADIO_DC_FREE_OFF = 0x00,
    RADIO_DC_FREEWHITENING = 0x01,
} RadioDcFree_t;

typedef enum
{
    LORA_PACKET_VARIABLE_LENGTH = 0x00, //!< The packet is on variable size, header included
    LORA_PACKET_FIXED_LENGTH = 0x01,    //!< The packet is known on both sides, no header included in the packet
    LORA_PACKET_EXPLICIT = LORA_PACKET_VARIABLE_LENGTH,
    LORA_PACKET_IMPLICIT = LORA_PACKET_FIXED_LENGTH,
} RadioLoRaPacketLengthsMode_t;

typedef enum
{
    LORA_CRC_ON = 0x01,  //!< CRC activated
    LORA_CRC_OFF = 0x00, //!< CRC not used
} RadioLoRaCrcModes_t;

typedef enum
{
    LORA_IQ_NORMAL = 0x00,
    LORA_IQ_INVERTED = 0x01,
} RadioLoRaIQModes_t;

typedef enum
{
    TCXO_CTRL_1_6V = 0x00,
    TCXO_CTRL_1_7V = 0x01,
    TCXO_CTRL_1_8V = 0x02,
    TCXO_CTRL_2_2V = 0x03,
    TCXO_CTRL_2_4V = 0x04,
    TCXO_CTRL_2_7V = 0x05,
    TCXO_CTRL_3_0V = 0x06,
    TCXO_CTRL_3_3V = 0x07,
} RadioTcxoCtrlVoltage_t;

typedef enum
{
    IRQ_RADIO_NONE = 0x0000,
    IRQ_TX_DONE = 0x0001,
    IRQ_RX_DONE = 0x0002,
    IRQ_PREAMBLE_DETECTED = 0x0004,
    IRQ_SYNCWORD_VALID = 0x0008,
    IRQ_HEADER_VALID = 0x0010,
    IRQ_HEADER_ERROR = 0x0020,
    IRQ_CRC_ERROR = 0x0040,
    IRQ_CAD_DONE = 0x0080,
    IRQ_CAD_ACTIVITY_DETECTED = 0x0100,
    IRQ_RX_TX_TIMEOUT = 0x0200,
    IRQ_RADIO_ALL = 0xFFFF,
} RadioIrqMasks_t;

typedef struct
{
    RadioPacketTypes_t packetType; //!< Packet to which the packet status are referring to.
    struct
    {
        struct
        {
            unsigned char RxStatus;
            char RssiAvg;  //!< The averaged RSSI
            char RssiSync; //!< The RSSI measured on last packet
            unsigned int FreqError;
        } Gfsk;
        struct
        {
            char RssiPkt; //!< The RSSI of the last packet
            char SnrPkt;  //!< The SNR of the last packet
            char SignalRssiPkt;
            unsigned int FreqError;
        } LoRa;
    } Params;
} PacketStatus_t;

typedef struct
{
    RadioPacketTypes_t packetType; //!< Packet to which the packet status are referring to.
    unsigned short PacketReceived;
    unsigned short CrcOk;
    unsigned short LengthError;
} RxCounter_t;

typedef union
{
    struct
    {
        unsigned char RC64KEnable : 1;    //!< Calibrate RC64K clock
        unsigned char RC13MEnable : 1;    //!< Calibrate RC13M clock
        unsigned char PLLEnable : 1;      //!< Calibrate PLL
        unsigned char ADCPulseEnable : 1; //!< Calibrate ADC Pulse
        unsigned char ADCBulkNEnable : 1; //!< Calibrate ADC bulkN
        unsigned char ADCBulkPEnable : 1; //!< Calibrate ADC bulkP
        unsigned char ImgEnable : 1;
        unsigned char : 1;
    } Fields;
    unsigned char Value;
} CalibrationParams_t;

typedef union
{
    struct
    {
        unsigned char WakeUpRTC : 1; //!< Get out of sleep mode if wakeup signal received from RTC
        unsigned char Reset : 1;
        unsigned char WarmStart : 1;
        unsigned char Reserved : 5;
    } Fields;
    unsigned char Value;
} SleepParams_t;

typedef union
{
    struct
    {
        unsigned char Rc64kCalib : 1; //!< RC 64kHz oscillator calibration failed
        unsigned char Rc13mCalib : 1; //!< RC 13MHz oscillator calibration failed
        unsigned char PllCalib : 1;   //!< PLL calibration failed
        unsigned char AdcCalib : 1;   //!< ADC calibration failed
        unsigned char ImgCalib : 1;   //!< Image calibration failed
        unsigned char XoscStart : 1;  //!< XOSC oscillator failed to start
        unsigned char PllLock : 1;    //!< PLL lock failed
        unsigned char BuckStart : 1;  //!< Buck converter failed to start
        unsigned char PaRamp : 1;     //!< PA ramp failed
        unsigned char : 7;            //!< Reserved
    } Fields;
    unsigned short Value;
} RadioError_t;

typedef enum
{
    USE_LDO = 0x00, // default
    USE_DCDC = 0x01,
} RadioRegulatorMode_t;

/* ״̬��: ģ���ڲ�״̬��ʶ */
static RadioOperatingModes_t OperatingMode;

/* �շ�: ���ݰ����Ʊ�ʶ */
static RadioPacketTypes_t PacketType;

/* ����: ���ݰ�������Ϣ */
static PacketStatus_t PacketStatus;

/* ����: ���ݰ�Ƶ��ƫ�� */
static volatile uint32e_t FrequencyError = 0;

/* ����: ���ݰ����ջ��� */
static uint8e_t RecvPacketBuffer[RF_RX_BUFFER_SIZE];

#if (RF_TX_RX_MODE == 0)
/* ����: LoRa���ʲ��ձ� */
static const int8e_t Ebyte_E220x_LoraBpsTable[8][3] =
    {
        {LORA_BW_500, LORA_SF11, LORA_CR_4_5}, /* Lora 0.3 kbps  */
        {LORA_BW_500, LORA_SF11, LORA_CR_4_5}, /* Lora 1.2 kbps  */
        {LORA_BW_500, LORA_SF11, LORA_CR_4_5}, /* Lora 2.4 kbps  */
        {LORA_BW_250, LORA_SF8, LORA_CR_4_5},  /* Lora 4.8 kbps  */
        {LORA_BW_500, LORA_SF8, LORA_CR_4_6},  /* Lora 9.6 kbps  */
        {LORA_BW_500, LORA_SF7, LORA_CR_4_6},  /* Lora 19.2 kbps */
        {LORA_BW_500, LORA_SF5, LORA_CR_4_8},  /* Lora 38.4 kbps */
        {LORA_BW_500, LORA_SF5, LORA_CR_4_5},  /* Lora 62.5 kbps */
};
#endif
/*!
 * FSK bandwidth definition
 */
typedef struct
{
    uint32e_t bandwidth;
    uint8e_t RegValue;
} FskBandwidth_t;

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] =
    {
        {4800, 0x1F},
        {5800, 0x17},
        {7300, 0x0F},
        {9700, 0x1E},
        {11700, 0x16},
        {14600, 0x0E},
        {19500, 0x1D},
        {23400, 0x15},
        {29300, 0x0D},
        {39000, 0x1C},
        {46900, 0x14},
        {58600, 0x0C},
        {78200, 0x1B},
        {93800, 0x13},
        {117300, 0x0B},
        {156200, 0x1A},
        {187200, 0x12},
        {234300, 0x0A},
        {312000, 0x19},
        {373600, 0x11},
        {467000, 0x09},
        {500000, 0x00}, // Invalid Bandwidth
};

/* !
 * @brief ģ�鸴λ
 *
 * @note  �͵�ƽ��λ(���ⲿIO����)
 */
void Ebyte_E220x_Reset(void)
{
    Ebyte_Port_DelayMs(10);
    Ebyte_Port_RstIoControl(0);
    Ebyte_Port_DelayMs(20);
    Ebyte_Port_RstIoControl(1);
    Ebyte_Port_DelayMs(20);
}

/* !
 * @brief ����ʽ ģ��æ�ȴ�
 *
 * @note  �ߵ�ƽ(�ⲿIO)��ʾ æ
 */
void Ebyte_E220x_WaitOnBusy(void)
{
    while (Ebyte_Port_BusyIoRead())
        ;
}

/* !
 * @brief ��ģ��������л���
 *
 * @note  ������ֲ� CS����ʱ sx126x���Զ�����
 */
void Ebyte_E220x_Wakeup(void)
{
    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(RADIO_GET_STATUS);
    Ebyte_Port_SpiTransmitAndReceivce(0x00);

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief ��ȡģ��״̬��Ϣ  �Ƕ�ȡ ״̬��
 *
 * @return �� RadioOperatingModes_t
 *     MODE_SLEEP                              = 0x00     ����
 *     MODE_STDBY_RC,                          = 0x01     �����ڲ�ʱ��RC-13M   ������ģʽ
 *     MODE_STDBY_XOSC,                        = 0x02     �����ⲿʱ��TXCO-32M ������ģʽ
 *     MODE_FS,                                = 0x03     FSģʽ ���ڲ���
 *     MODE_TX,                                = 0x04     TXģʽ ��ʾ���ڷ���
 *     MODE_RX,                                = 0x05     RXģʽ ��ʾ���ڼ���
 *     MODE_RX_DC,                             = 0x06     ��������-����ģʽ  �͹���
 *     MODE_CAD                                = 0x07,    �ŵ�����ģʽ
 */
RadioOperatingModes_t Ebyte_E220x_GetOperatingMode(void)
{
    return OperatingMode;
}

/* !
 * @brief ��ȡģ��״̬��Ϣ  ���ģ�����߾ͻ�����
 */
void Ebyte_E220x_CheckReady(void)
{
    if ((Ebyte_E220x_GetOperatingMode() == MODE_SLEEP) || (Ebyte_E220x_GetOperatingMode() == MODE_RX_DC))
    {
        Ebyte_E220x_Wakeup();
    }
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief ��E220xģ������д��ָ�� ��д������
 *
 * @param command ֧�ֵ�ָ�� ��μ� RadioCommands_t����
 * @param buffer  ָ���������ָ��
 * @param size    ָ������ݵĳ���
 */
void Ebyte_E220x_WriteCommand(uint8e_t command, uint8e_t *buffer, uint16e_t size)
{

    /* ��ǰ״̬��ʶ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif

    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(command);

    /* ����ָ��ӵ����ݰ� */
    for (uint16e_t i = 0; i < size; i++)
    {
        Ebyte_Port_SpiTransmitAndReceivce(buffer[i]);
    }

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* ������ָ������Ҫ�ȴ� */
    if (command != RADIO_SET_SLEEP)
    {
        Ebyte_E220x_WaitOnBusy();
    }
}

/* !
 * @brief ��E220xģ������д��ָ�� ����ȡ����
 *
 * @param command ֧�ֵ�ָ�� ��μ� RadioCommands_t����
 * @param buffer  ָ�� �������ݽ��洢��Ŀ���ַ
 * @param size    ��Ҫ��ȡ�����ݳ���
 */
void Ebyte_E220x_ReadCommand(uint8e_t command, uint8e_t *buffer, uint16e_t size)
{
    /* ��ǰ״̬��ʶ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(command);
    Ebyte_Port_SpiTransmitAndReceivce(0x00);

    /* ��ȡ���� */
    for (uint16e_t i = 0; i < size; i++)
    {
        buffer[i] = Ebyte_Port_SpiTransmitAndReceivce(0x00);
    }

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief ����д��E220x�ڲ��Ĵ���
 *
 * @param address �Ĵ�����ʼ��ַ
 * @param buffer  ָ���������ָ��
 * @param size    ָ������ݵĳ���
 */
void Ebyte_E220x_WriteRegisters(uint16e_t address, uint8e_t *buffer, uint16e_t size)
{
    /* ��ǰ״̬ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(RADIO_WRITE_REGISTER);
    Ebyte_Port_SpiTransmitAndReceivce((address & 0xFF00) >> 8);
    Ebyte_Port_SpiTransmitAndReceivce(address & 0x00FF);

    /* ����ָ��ӵ����ݰ� */
    for (uint16e_t i = 0; i < size; i++)
    {
        Ebyte_Port_SpiTransmitAndReceivce(buffer[i]);
    }

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief д��E220x�ڲ������Ĵ���
 *
 * @param address �Ĵ�����ַ
 * @param value   ��д��ֵ
 */
void Ebyte_E220x_WriteRegister(uint16e_t address, uint8e_t value)
{
    Ebyte_E220x_WriteRegisters(address, &value, 1);
}

/* !
 * @brief ������ȡE220x�ڲ��Ĵ�����ֵ
 *
 * @param address �Ĵ�����ʼ��ַ
 * @param buffer  ָ�� �������ݽ��洢��Ŀ���ַ
 * @param size    ��Ҫ��ȡ�����ݳ���
 */
void Ebyte_E220x_ReadRegisters(uint16e_t address, uint8e_t *buffer, uint16e_t size)
{
    /* ��ǰ״̬ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(RADIO_READ_REGISTER);
    Ebyte_Port_SpiTransmitAndReceivce((address & 0xFF00) >> 8);
    Ebyte_Port_SpiTransmitAndReceivce(address & 0x00FF);
    Ebyte_Port_SpiTransmitAndReceivce(0);

    for (uint16e_t i = 0; i < size; i++)
    {
        buffer[i] = Ebyte_Port_SpiTransmitAndReceivce(0x00);
    }

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief  ��ȡE220x�ڲ�ĳ���Ĵ�����ֵ
 *
 * @param  address Ŀ��Ĵ�����ַ
 * @return �Ĵ���ֵ
 */
uint8e_t Ebyte_E220x_ReadRegister(uint16e_t address)
{
    uint8e_t data;
    Ebyte_E220x_ReadRegisters(address, &data, 1);
    return data;
}

/* !
 * @brief ����д��E220x�ڲ� FIFO
 *
 * @param offset  ��ʼ��ַ
 * @param buffer  ָ���д������
 * @param size    д������ݳ���
 */
void Ebyte_E220x_WriteBuffer(uint8e_t offset, uint8e_t *buffer, uint8e_t size)
{
    /* ��ǰ״̬ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(RADIO_WRITE_BUFFER);
    Ebyte_Port_SpiTransmitAndReceivce(offset);

    /* �������� */
    for (uint16e_t i = 0; i < size; i++)
    {
        Ebyte_Port_SpiTransmitAndReceivce(buffer[i]);
    }
    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief ������ȡE220x�ڲ� FIFO
 *
 * @param offset  ��ʼ��ַ
 * @param buffer  ָ����洢��������
 * @param size    ��ȡ�����ݳ���
 */
void Ebyte_E220x_ReadBuffer(uint8e_t offset, uint8e_t *buffer, uint8e_t size)
{
    /* ��ǰ״̬ */
    Ebyte_E220x_CheckReady();

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(0);
#endif
    /* ����ָ�� */
    Ebyte_Port_SpiTransmitAndReceivce(RADIO_READ_BUFFER);
    Ebyte_Port_SpiTransmitAndReceivce(offset);
    Ebyte_Port_SpiTransmitAndReceivce(0);

    /* ��ȡ���� */
    for (uint16e_t i = 0; i < size; i++)
    {
        buffer[i] = Ebyte_Port_SpiTransmitAndReceivce(0x00);
    }

    /* SPI Ƭѡ CS */
#if EBYTE_PORT_SPI_CS_SOFTWARE
    Ebyte_Port_SpiCsIoControl(1);
#endif
    /* �ȴ�ģ�����׼�� */
    Ebyte_E220x_WaitOnBusy();
}

/* !
 * @brief ����ģ��IO ���뷢��ģʽ
 */
void Ebyte_E220x_RfIoEnterTxMode(void)
{
    /* IO RXEN  */
    Ebyte_Port_RxenIoControl(0);

    /* IO TXEN  */
    Ebyte_Port_TxenIoControl(1);
}

/* !
 * @brief ����ģ��IO �������ģʽ
 */
void Ebyte_E220x_RfIoEnterRxMode(void)
{
    /* IO TXEN  */
    Ebyte_Port_TxenIoControl(0);

    /* IO RXEN  */
    Ebyte_Port_RxenIoControl(1);
}

/***********************************************************************************************************************************/
/***********************************************************************************************************************************/
/***********************************************************************************************************************************/

/* !
 * @brief ����E220xģ��Ĵ���ʱ��ģʽ
 *
 * @param  standbyConfig ��������
 *         @arg STDBY_RC: ���ڲ���RC��Ƶʱ�ӣ����Խ��͹���
 *         @arg STDBY_XOSC��ֻ����Ӧ�ó����ʱ��Ҫ���ϸ����ǲ���Ҫ��XOSXģ��
 */
void Ebyte_E220x_SetStandby(RadioStandbyModes_t standbyConfig)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_STANDBY, (uint8e_t *)&standbyConfig, 1);
    if (standbyConfig == STDBY_RC)
    {
        OperatingMode = MODE_STDBY_RC;
    }
    else
    {
        OperatingMode = MODE_STDBY_XOSC;
    }
}

/* !
 * @brief ʱ������
 *
 * @param  calibParam �� CalibrationParams_t
 */
void Ebyte_E220x_Calibrate(CalibrationParams_t calibParam)
{
    Ebyte_E220x_WriteCommand(RADIO_CALIBRATE, (uint8e_t *)&calibParam, 1);
}

/* !
 * @brief ����ģ���ڲ���DIO3������Ϊ�ڲ�TCXO�Ŀ�������
 *
 * @param  tcxoVoltage ����DIO3�����ѹ
 * @param  timeout ��ʱʱ��
 * @note   E220x�ڲ�Ĭ��ʹ����Դ���� ��Ҫ3.3V
 */
void Ebyte_E220x_SetDio3AsTcxoCtrl(RadioTcxoCtrlVoltage_t tcxoVoltage, uint32e_t timeout)
{
    uint8e_t buf[4];

    buf[0] = tcxoVoltage & 0x07;
    buf[1] = (uint8e_t)((timeout >> 16) & 0xFF);
    buf[2] = (uint8e_t)((timeout >> 8) & 0xFF);
    buf[3] = (uint8e_t)(timeout & 0xFF);

    Ebyte_E220x_WriteCommand(RADIO_SET_TCXOMODE, buf, 4);
}

/* !
 * @brief ����ģ���DIO2������ΪTXEN�Ŀ�������
 *
 * @param  enable ʹ������
 *         0;�ر�
 *         1;����
 */
void Ebyte_E220x_SetDio2AsRfSwitchCtrl(uint8e_t enable)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_RFSWITCHMODE, &enable, 1);
}

/* !
 * @brief �ڲ�״̬�� ����ͳһʶ���շ����Է�ʽ LoRa / GFSK
 */
RadioPacketTypes_t Ebyte_E220x_GetPacketType(void)
{
    return PacketType;
}

/* !
 * @brief ��ȡ���յ������ݰ��ĳ��Ⱥ���ʼ��ַ
 *
 * @param  payloadLength �ѽ������ݰ��ĳ���
 * @param  rxStartBufferPointer �ѽ������ݰ����ڲ�FIFO��ʼ��ַ
 */
void Ebyte_E220x_GetRxBufferStatus(uint8e_t *payloadLength, uint8e_t *rxStartBufferPointer)
{
    uint8e_t status[2];

    Ebyte_E220x_ReadCommand(RADIO_GET_RXBUFFERSTATUS, status, 2);

    // In case of LORA fixed header, the payloadLength is obtained by reading
    // the register REG_LR_PAYLOADLENGTH
    //    if( ( Ebyte_E220x_GetPacketType( ) == PACKET_TYPE_LORA ) && ( Ebyte_E220x_ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
    if ((Ebyte_E220x_ReadRegister(REG_LR_PACKETPARAMS) >> 7 == 1))
    {
        *payloadLength = Ebyte_E220x_ReadRegister(REG_LR_PAYLOADLENGTH);
    }
    else
    {
        *payloadLength = status[0];
    }
    *rxStartBufferPointer = status[1];
}

/* !
 * @brief ��ģ���ڲ�FIFOд�����������
 *
 * @param payload ָ����������ݵ�ָ��
 * @param size ���������ݳ���
 */
void Ebyte_E220x_SetPayload(uint8e_t *payload, uint8e_t size)
{
    Ebyte_E220x_WriteBuffer(0x00, payload, size);
}

/* !
 * @brief ��ȡ���յ�������
 *
 * @param buffer ָ����ջ����ָ��
 * @param size   ��ȡ����
 * @param maxSize �����ճ��� �������ջ��� ��ͷ�ļ����� RF_RX_BUFFER_SIZE
 */
uint8e_t Ebyte_E220x_GetPayload(uint8e_t *buffer, uint8e_t *size, uint8e_t maxSize)
{
    uint8e_t offset = 0;

    Ebyte_E220x_GetRxBufferStatus(size, &offset);
    if (*size > maxSize)
    {
        return 1;
    }
    Ebyte_E220x_ReadBuffer(offset, buffer, *size);
    return 0;
}

/* !
 * @brief �����ڲ��жϱ�ʶ�Ĵ��� ��ӳ���жϵ�DIO1~3����  ����MCU״̬�ж�
 *
 * @param irqMask    ��RadioIrqMasks_t
 *                   @arg IRQ_RADIO_NONE
 *                   @arg IRQ_TX_DONE
 *                   @arg IRQ_RX_DONE
 *                   @arg IRQ_PREAMBLE_DETECTED
 *                   @arg IRQ_SYNCWORD_VALID
 *                   @arg IRQ_HEADER_VALID
 *                   @arg IRQ_HEADER_ERROR
 *                   @arg IRQ_CRC_ERROR
 *                   @arg IRQ_CAD_DONE
 *                   @arg IRQ_CAD_ACTIVITY_DETECTED
 *                   @arg IRQ_RX_TX_TIMEOUT
 *                   @arg IRQ_RADIO_ALL
 *
 * @param dio1Mask   ��RadioIrqMasks_t
 * @param dio2Mask   ��RadioIrqMasks_t
 * @param dio3Mask   ��RadioIrqMasks_t
 */
void Ebyte_E220x_SetDioIrqParams(uint16e_t irqMask, uint16e_t dio1Mask, uint16e_t dio2Mask, uint16e_t dio3Mask)
{
    uint8e_t buf[8];

    buf[0] = (uint8e_t)((irqMask >> 8) & 0x00FF);
    buf[1] = (uint8e_t)(irqMask & 0x00FF);
    buf[2] = (uint8e_t)((dio1Mask >> 8) & 0x00FF);
    buf[3] = (uint8e_t)(dio1Mask & 0x00FF);
    buf[4] = (uint8e_t)((dio2Mask >> 8) & 0x00FF);
    buf[5] = (uint8e_t)(dio2Mask & 0x00FF);
    buf[6] = (uint8e_t)((dio3Mask >> 8) & 0x00FF);
    buf[7] = (uint8e_t)(dio3Mask & 0x00FF);
    Ebyte_E220x_WriteCommand(RADIO_CFG_DIOIRQ, buf, 8);
}

/* !
 * @brief ����жϱ�־λ
 *
 * @param irq �жϱ�ʶ����
 *            @arg IRQ_RADIO_NONE                          = 0x0000,
 *            @arg IRQ_TX_DONE                             = 0x0001,
 *            @arg IRQ_RX_DONE                             = 0x0002,
 *            @arg IRQ_PREAMBLE_DETECTED                   = 0x0004,
 *            @arg IRQ_SYNCWORD_VALID                      = 0x0008,
 *            @arg IRQ_HEADER_VALID                        = 0x0010,
 *            @arg IRQ_HEADER_ERROR                        = 0x0020,
 *            @arg IRQ_CRC_ERROR                           = 0x0040,
 *            @arg IRQ_CAD_DONE                            = 0x0080,
 *            @arg IRQ_CAD_ACTIVITY_DETECTED               = 0x0100,
 *            @arg IRQ_RX_TX_TIMEOUT                       = 0x0200,
 *            @arg IRQ_RADIO_ALL                           = 0xFFFF,
 */
void Ebyte_E220x_ClearIrqStatus(uint16e_t irq)
{
    uint8e_t buf[2];

    buf[0] = (uint8e_t)(((uint16e_t)irq >> 8) & 0x00FF);
    buf[1] = (uint8e_t)((uint16e_t)irq & 0x00FF);
    Ebyte_E220x_WriteCommand(RADIO_CLR_IRQSTATUS, buf, 2);
}

/* !
 * @brief ʹ��ģ�� �������߷���
 *
 * @param timeout ��ʱʱ��
 *                @arg 0:�����г�ʱ�ж�
 */
void Ebyte_E220x_SetTx(uint32e_t timeout)
{
    uint8e_t buf[3];

    OperatingMode = MODE_TX;

    /* TXEN���� �л���·���뷢��ģʽ */
    Ebyte_E220x_RfIoEnterTxMode();

    Ebyte_E220x_SetStandby(STDBY_XOSC);
    Ebyte_E220x_WriteRegister(REG_XTA_TRIM, XTA_TRIM_VALUE);
    Ebyte_E220x_WriteRegister(REG_XTB_TRIM, XTA_TRIM_VALUE);

    Ebyte_E220x_SetDioIrqParams(IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                                IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                                IRQ_RADIO_NONE,
                                IRQ_RADIO_NONE);

    Ebyte_E220x_ClearIrqStatus(IRQ_RADIO_ALL);

    buf[0] = (uint8e_t)((timeout >> 16) & 0xFF);
    buf[1] = (uint8e_t)((timeout >> 8) & 0xFF);
    buf[2] = (uint8e_t)(timeout & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_TX, buf, 3);
}

/* !
 * @brief LoRa�������ݰ����(ǰ�����)��������
 * @param size ��Ϣ��ĳ���
 * @note  ע��ʱ�򣬱����� Ebyte_E220x_SetPacketType()�����趨LORA����ģʽ֮�����
 *        ͷ�ļ��п��Զ������
 */
void Ebyte_E220x_SetLoraPacketParams(uint8e_t size)
{
    uint8e_t buf[9] = {0};
    uint16e_t preambleLen = LORA_PREAMBLE_LENGTH;

    /* ��ϲ��� */
    buf[0] = (preambleLen >> 8) & 0xFF;   // ǰ���볤�ȸ�Byte
    buf[1] = preambleLen;                 // ǰ���볤�ȵ�Byte
    buf[2] = LORA_PACKET_VARIABLE_LENGTH; // 0:�̶����ȵ���Ϣ������  1:�ɱ䳤������ ������Header֡����������
    buf[3] = size;                        // ��Ч��Ϣ�ĳ���
    buf[4] = LORA_CRC_ON;                 // 1:����CRCУ��  0:�ر�
    buf[5] = LORA_IQ_NORMAL;              // 1:��λ����     0:�ر�

    /* ǰ����д��ģ�� */
    Ebyte_E220x_WriteCommand(RADIO_SET_PACKETPARAMS, buf, 6);
}

/* !
 * @brief CRC ����
 *
 * @param seed CRC����
 */
void Ebyte_E220x_SetCrcSeed(uint16e_t seed)
{
    uint8e_t buf[2];

    buf[0] = (uint8e_t)((seed >> 8) & 0xFF);
    buf[1] = (uint8e_t)(seed & 0xFF);

    switch (Ebyte_E220x_GetPacketType())
    {
    case PACKET_TYPE_GFSK:
        Ebyte_E220x_WriteRegisters(REG_LR_CRCSEEDBASEADDR, buf, 2);
        break;

    default:
        break;
    }
}

/* !
 * @brief CRC ����
 *
 * @param polynomial CRC����
 */
void Ebyte_E220x_SetCrcPolynomial(uint16e_t polynomial)
{
    uint8e_t buf[2];

    buf[0] = (uint8e_t)((polynomial >> 8) & 0xFF);
    buf[1] = (uint8e_t)(polynomial & 0xFF);

    switch (Ebyte_E220x_GetPacketType())
    {
    case PACKET_TYPE_GFSK:
        Ebyte_E220x_WriteRegisters(REG_LR_CRCPOLYBASEADDR, buf, 2);
        break;

    default:
        break;
    }
}

void Ebyte_E220x_SetFskPacketParams(uint8e_t size)
{
    uint8e_t buf[9] = {0};
    uint8e_t crcVal = 0;
    uint16e_t preambleLength = 0;

    Ebyte_E220x_SetCrcSeed(CRC_CCITT_SEED);
    Ebyte_E220x_SetCrcPolynomial(CRC_POLYNOMIAL_CCITT);
    crcVal = RADIO_CRC_2_BYTES_INV;
    preambleLength = FSK_PREAMBLE_LENGTH << 3; // Byteתbit  ������λ�ȼ���X8

    buf[0] = (preambleLength >> 8) & 0xFF;
    buf[1] = preambleLength;
    buf[2] = RADIO_PREAMBLE_DETECTOR_08_BITS;     // ���ټ��8bit
    buf[3] = FSK_MAC_PUBLIC_SYNCWORD_LENGTH << 3; // convert from byte to bit
    buf[4] = RADIO_ADDRESSCOMP_FILT_OFF;          // �رյ�ַ����
    buf[5] = RADIO_PACKET_VARIABLE_LENGTH;        // �ɱ䳤��Ϣ��
    buf[6] = size;                                // ��Ч��Ϣ�ĳ���
    buf[7] = crcVal;                              // �ر�:RADIO_CRC_OFF
    buf[8] = RADIO_DC_FREEWHITENING;

    Ebyte_E220x_WriteCommand(RADIO_SET_PACKETPARAMS, buf, 9);
}

/* !
 * @brief ʹ��ģ�� �������߽���
 *
 * @param timeout ��ʱʱ��
 *                @arg 0:�����г�ʱ�ж�
 */
void Ebyte_E220x_SetRx(uint32e_t timeout)
{
    uint8e_t buf[3];

    OperatingMode = MODE_RX;
    /* ����ģ������ж� */
    Ebyte_E220x_SetStandby(STDBY_XOSC);
    Ebyte_E220x_WriteRegister(REG_XTA_TRIM, XTA_TRIM_VALUE);
    Ebyte_E220x_WriteRegister(REG_XTB_TRIM, XTA_TRIM_VALUE);

    Ebyte_E220x_SetDioIrqParams(IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                                IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                                IRQ_RADIO_NONE,
                                IRQ_RADIO_NONE);

#if (RF_TX_RX_MODE == 0)
    Ebyte_E220x_SetLoraPacketParams(0xFF);
#else
    Ebyte_E220x_SetFskPacketParams(0xFF);
#endif

    /* ����жϱ�ʶ */
    Ebyte_E220x_ClearIrqStatus(IRQ_RADIO_ALL);

    /* RXEN IOʹ�� */
    Ebyte_E220x_RfIoEnterRxMode();

    /* ʹ�ܽ���ģʽ */
    buf[0] = (uint8e_t)((timeout >> 16) & 0xFF);
    buf[1] = (uint8e_t)((timeout >> 8) & 0xFF);
    buf[2] = (uint8e_t)(timeout & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_RX, buf, 3);
}

/* !
 * @brief ��ȡģ���ڲ��ж�ָʾ��ʶ
 *
 * @return  �жϼĴ���ֵ
 * @note    ��Ӧbit��1��ʾ�����˶�Ӧ�¼�
 *          bit0 : IRQ_TX_DONE                �������
 *          bit1 : IRQ_RX_DONE                �������
 *          bit2 : IRQ_PREAMBLE_DETECTED      ��⵽ǰ����
 *          bit3 : IRQ_SYNCWORD_VALID         FSKģʽ�� ��⵽��ͬ����
 *          bit4 : IRQ_HEADER_VALID           LORAģʽ�� ��⵽��֡ͷ
 *          bit5 : IRQ_HEADER_ERROR           LORAģʽ�� ֡ͷCRC����
 *          bit6 : IRQ_CRC_ERROR              �������ݰ�CRC����
 *          bit7 : IRQ_CAD_DONE               �ŵ����ü�����
 *          bit8 : IRQ_CAD_ACTIVITY_DETECTED  ��⵽�˿����ŵ�
 *          bit9 : IRQ_RX_TX_TIMEOUT          ����/���ճ�ʱ
 */
uint16e_t Ebyte_E220x_GetIrqStatus(void)
{
    uint8e_t irqStatus[2];

    Ebyte_E220x_ReadCommand(RADIO_GET_IRQSTATUS, irqStatus, 2);

    return (irqStatus[0] << 8) | irqStatus[1];
}

/* !
 * @brief ��ȡģ���ڲ�״̬
 *
 * @param timeout ��ʱʱ��
 *
 * @note  RadioStatusChipMode_t ������оƬ�ڲ�״̬
 */
RadioStatus_t Ebyte_E220x_GetStatus(void)
{
    uint8e_t stat = 0;
    RadioStatus_t status;

    Ebyte_E220x_ReadCommand(RADIO_GET_STATUS, (uint8e_t *)&stat, 1);
    status.Value = stat;
    return status;
}

/* !
 * @brief ��ȡģ���ڲ�������Ϣ
 *
 * @param timeout ��ʱʱ��
 *
 * @note  RadioError_t �����˴���״̬
 */
RadioError_t Ebyte_E220x_GetDeviceErrors(void)
{
    RadioError_t error;

    Ebyte_E220x_ReadCommand(RADIO_GET_ERROR, (uint8e_t *)&error, 2);
    return error;
}

/* !
 * @brief �趨Loraͨ��ͬ����
 *
 * @param syncWord
 */
void Ebyte_E220x_SetLoraSyncWord(uint16e_t syncWord)
{
    Ebyte_E220x_WriteRegister(REG_LR_SYNCWORD, (syncWord >> 8) & 0xFF);
    Ebyte_E220x_WriteRegister(REG_LR_SYNCWORD + 1, syncWord & 0xFF);
}

/* !
 * @brief �趨FSKͨ��ͬ����
 *
 * @param syncWord
 */
void Ebyte_E220x_SetFskSyncWord(void)
{
    uint8e_t syncWord[8] = {0};

    syncWord[0] = FSK_MAC_PUBLIC_SYNCWORD_7;
    syncWord[1] = FSK_MAC_PUBLIC_SYNCWORD_6;
    syncWord[2] = FSK_MAC_PUBLIC_SYNCWORD_5;
    syncWord[3] = FSK_MAC_PUBLIC_SYNCWORD_4;
    syncWord[4] = FSK_MAC_PUBLIC_SYNCWORD_3;
    syncWord[5] = FSK_MAC_PUBLIC_SYNCWORD_2;
    syncWord[6] = FSK_MAC_PUBLIC_SYNCWORD_1;
    syncWord[7] = FSK_MAC_PUBLIC_SYNCWORD_0;

    Ebyte_E220x_WriteRegisters(REG_LR_SYNCWORDBASEADDRESS, syncWord, 8);
}

void Ebyte_E220x_SetWhiteningSeed(uint16e_t seed)
{
    uint8e_t regValue = 0;

    switch (Ebyte_E220x_GetPacketType())
    {
    case PACKET_TYPE_GFSK:
        regValue = Ebyte_E220x_ReadRegister(REG_LR_WHITSEEDBASEADDR_MSB) & 0xFE;
        regValue = ((seed >> 8) & 0x01) | regValue;
        Ebyte_E220x_WriteRegister(REG_LR_WHITSEEDBASEADDR_MSB, regValue); // only 1 bit.
        Ebyte_E220x_WriteRegister(REG_LR_WHITSEEDBASEADDR_LSB, (uint8e_t)seed);
        break;

    default:
        break;
    }
}

uint32e_t Ebyte_E220x_GetRandom(void)
{
    uint8e_t buf[] = {0, 0, 0, 0};

    // Set radio in continuous reception
    Ebyte_E220x_SetRx(0);

    Ebyte_Port_DelayMs(1);

    Ebyte_E220x_ReadRegisters(RANDOM_NUMBER_GENERATORBASEADDR, buf, 4);

    Ebyte_E220x_SetStandby(STDBY_RC);

    return ((uint32e_t)buf[0] << 24) | ((uint32e_t)buf[1] << 16) | ((uint32e_t)buf[2] << 8) | buf[3];
    ;
}

/* !
 * @brief ģ���������ģʽ ���ر��շ���Χ��·
 *
 * @param cmd ����ָ��
 *        @arg 0x00: ����������ʱ�����Ѻ�������������ȫ��ʧ
 *        @arg 0x01: ����ͨ������RTC���г�ʱ���㡣���Ѻ�������������ȫ��ʧ
 *        @arg 0x04: ����������ʱ�����Ѻ������������˵��Ʋ���������������ʧ
 *        @arg 0x05: ����ͨ������RTC���г�ʱ���㡣���Ѻ������������˵��Ʋ���������������ʧ
 */
void Ebyte_E220x_SetSleep(uint8e_t cmd)
{
    Ebyte_Port_TxenIoControl(0);
    Ebyte_Port_RxenIoControl(0);
    Ebyte_E220x_WriteCommand(RADIO_SET_SLEEP, &cmd, 1);
    OperatingMode = MODE_SLEEP;
}

/* !
 * @brief ģ�����FSģ��
 *
 * @param �����ڵ���
 */
void Ebyte_E220x_SetFs(void)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_FS, 0, 0);
    OperatingMode = MODE_FS;
}

void Ebyte_E220x_SetRxBoosted(uint32e_t timeout)
{
    uint8e_t buf[3];

    OperatingMode = MODE_RX;

    Ebyte_E220x_WriteRegister(REG_RX_GAIN, 0x96); // max LNA gain, increase current by ~2mA for around ~3dB in sensivity

    buf[0] = (uint8e_t)((timeout >> 16) & 0xFF);
    buf[1] = (uint8e_t)((timeout >> 8) & 0xFF);
    buf[2] = (uint8e_t)(timeout & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_RX, buf, 3);
}

/* !
 * @brief ģ�������������ģʽ
 *
 * @param rxTime     ����ʱ������
 * @param sleepTime  ����ʱ������
 */
void Ebyte_E220x_SetRxDutyCycle(uint32e_t rxTime, uint32e_t sleepTime)
{
    uint8e_t buf[6];

    buf[0] = (uint8e_t)((rxTime >> 16) & 0xFF);
    buf[1] = (uint8e_t)((rxTime >> 8) & 0xFF);
    buf[2] = (uint8e_t)(rxTime & 0xFF);
    buf[3] = (uint8e_t)((sleepTime >> 16) & 0xFF);
    buf[4] = (uint8e_t)((sleepTime >> 8) & 0xFF);
    buf[5] = (uint8e_t)(sleepTime & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_RXDUTYCYCLE, buf, 6);
    OperatingMode = MODE_RX_DC;
}

/* !
 * @brief ģ������ŵ����ģʽ
 *
 * @note ��Ҫ�����ŵ�������
 */
void Ebyte_E220x_SetCad(void)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_CAD, 0, 0);
    OperatingMode = MODE_CAD;
}

/* !
 * @brief �����ڲ���
 */
void Ebyte_E220x_SetTxContinuousWave(void)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_TXCONTINUOUSWAVE, 0, 0);
}
/* !
 * @brief �����ڲ���
 */
void Ebyte_E220x_SetTxInfinitePreamble(void)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0);
}

void Ebyte_E220x_SetStopRxTimerOnPreambleDetect(uint8e_t enable)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_STOPRXTIMERONPREAMBLE, (uint8e_t *)&enable, 1);
}

void Ebyte_E220x_SetLoRaSymbNumTimeout(uint8e_t SymbNum)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_LORASYMBTIMEOUT, &SymbNum, 1);
}

/* !
 * @brief �����ڲ���ѹ������ģʽ
 *
 * @param mode ģʽ
 *        @arg USE_LDO   :Ĭ���ڲ�ʹ������LDO��ѹ��
 *        @arg USE_DCDC  :������FS��Rx��Txģʽ�����ø�Ч��DC-DC��ѹת��
 */
void Ebyte_E220x_SetRegulatorMode(RadioRegulatorMode_t mode)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_REGULATORMODE, (uint8e_t *)&mode, 1);
}

/* !
 * @brief ��̬ʱ������
 *
 * @param freq ģʽ
 */
void Ebyte_E220x_CalibrateImage(uint32e_t freq)
{
    uint8e_t calFreq[2];

    if (freq > 900000000)
    {
        calFreq[0] = 0xE1;
        calFreq[1] = 0xE9;
    }
    else if (freq > 850000000)
    {
        calFreq[0] = 0xD7;
        calFreq[1] = 0xD8;
    }
    else if (freq > 770000000)
    {
        calFreq[0] = 0xC1;
        calFreq[1] = 0xC5;
    }
    else if (freq > 460000000)
    {
        calFreq[0] = 0x75;
        calFreq[1] = 0x81;
    }
    else if (freq > 425000000)
    {
        calFreq[0] = 0x6B;
        calFreq[1] = 0x6F;
    }
    Ebyte_E220x_WriteCommand(RADIO_CALIBRATEIMAGE, calFreq, 2);
}

/* !
 * @brief �趨PA���� ������ο��ֲ�
 */
void Ebyte_E220x_SetPaConfig(uint8e_t paDutyCycle, uint8e_t hpMax, uint8e_t deviceSel, uint8e_t paLut)
{
    uint8e_t buf[4];

    buf[0] = paDutyCycle;
    buf[1] = hpMax;
    buf[2] = deviceSel;
    buf[3] = paLut;
    Ebyte_E220x_WriteCommand(RADIO_SET_PACONFIG, buf, 4);
}

void Ebyte_E220x_SetRxTxFallbackMode(uint8e_t fallbackMode)
{
    Ebyte_E220x_WriteCommand(RADIO_SET_TXFALLBACKMODE, &fallbackMode, 1);
}

/* !
 * @brief �趨�ز�Ƶ��
 *
 * @param frequency Ƶ��  ��ο��궨�� RF_FREQUENCY_START
 */
void Ebyte_E220x_SetRfFrequency(uint32e_t frequency)
{
    uint8e_t buf[4];
    uint32e_t freq = 0;

    CalibrationParams_t calibrationParams;
    calibrationParams.Value = 0xFF;
    Ebyte_E220x_Calibrate(calibrationParams);

    Ebyte_E220x_CalibrateImage(frequency);

    Ebyte_E220x_SetStandby(STDBY_XOSC);
    Ebyte_E220x_WriteRegister(REG_XTA_TRIM, XTA_TRIM_VALUE);
    Ebyte_E220x_WriteRegister(REG_XTB_TRIM, XTA_TRIM_VALUE);

    freq = (uint32e_t)((double)frequency / (double)FREQ_STEP);
    buf[0] = (uint8e_t)((freq >> 24) & 0xFF);
    buf[1] = (uint8e_t)((freq >> 16) & 0xFF);
    buf[2] = (uint8e_t)((freq >> 8) & 0xFF);
    buf[3] = (uint8e_t)(freq & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_RFFREQUENCY, buf, 4);
}

/* !
 * @brief �趨������Ʒ�ʽ
 *
 * @param packetType ���Ʒ�ʽ
 *        @arg PACKET_TYPE_GFSK: FSK����
 *        @arg PACKET_TYPE_LORA: LORA����
 */
void Ebyte_E220x_SetPacketType(RadioPacketTypes_t packetType)
{
    PacketType = packetType;
    Ebyte_E220x_WriteCommand(RADIO_SET_PACKETTYPE, (uint8e_t *)&packetType, 1);
}

/* !
 * @brief ���书�� �� PA�ȶ�ʱ������
 *
 * @param power    ���书�� -3~22 dbm
 * @param rampTime PA�ȶ�ʱ�� ��ο�RadioRampTimes_t���� ֧��10us ~ 3.4ms
 */
void Ebyte_E220x_SetTxPaParams(int8e_t power, RadioRampTimes_t rampTime)
{
    uint8e_t buf[2];

    Ebyte_E220x_SetPaConfig(0x04, 0x07, 0x00, 0x01);
    if (power > 22)
    {
        power = 22;
    }
    else if (power < -3)
    {
        power = -3;
    }
    Ebyte_E220x_WriteRegister(REG_OCP, 0x38); // current max 160mA for the whole device

    buf[0] = power;
    buf[1] = (uint8e_t)rampTime;
    Ebyte_E220x_WriteCommand(RADIO_SET_TXPARAMS, buf, 2);
}

/* !
 * @brief LoRa�ؼ������趨
 *
 * @param datarate  ��Ƶ����SF  ( ��ֵԽ��ʵ�ʴ��䳤�ȸ�����������Խ�͡����ǣ�ͬ��������������ȴ�����˿��Դ����ʵ�����ݣ����������½� )
 *                  @arg LORA_SF5
 *                  @arg LORA_SF6
 *                  @arg LORA_SF7
 *                  @arg LORA_SF8
 *                  @arg LORA_SF9
 *                  @arg LORA_SF10
 *                  @arg LORA_SF11
 *                  @arg LORA_SF12
 *
 * @param bandwidth Ԥ��Ƶ��ͨ��BW
 *                  @arg LORA_BW_500
 *                  @arg LORA_BW_250
 *                  @arg LORA_BW_125
 *                  @arg LORA_BW_062
 *                  @arg LORA_BW_041
 *                  @arg LORA_BW_031
 *                  @arg LORA_BW_020
 *                  @arg LORA_BW_015
 *                  @arg LORA_BW_010
 *                  @arg LORA_BW_007
 *
 * @param coderate  ������CR    (  LoRa����ѭ������������д�����, ������Խ�ߣ�����������Խǿ�����ǣ����俪��������)
 *                  @arg LORA_CR_4_5
 *                  @arg LORA_CR_4_6
 *                  @arg LORA_CR_4_7
 *                  @arg LORA_CR_4_8
 *
 * @note  ע��ʱ�򣬱����� Ebyte_E220x_SetPacketType()�����趨LORA����ģʽ֮�����
 *        LoRa�������ʼ��㹫ʽ Rs=BW/(2^SF)
 *        LoRa�������ʼ��㹫ʽ DR=SF*( BW/2^SF)*CR
 *        �ϵ�Ƶ�Σ�169MHz����֧��250K��500KHz�� bandwidth
 */
void Ebyte_E220x_SetModulationLoraParams(RadioLoRaSpreadingFactors_t datarate, RadioLoRaBandwidths_t bandwidth, RadioLoRaCodingRates_t coderate)
{
    uint8e_t buf[8] = {0};

    /* ��ϲ��� */
    buf[0] = datarate;
    buf[1] = bandwidth;
    buf[2] = coderate;
    buf[3] = 0x01; // �������Ż�����
    /* д��ģ�� */
    Ebyte_E220x_WriteCommand(RADIO_SET_MODULATIONPARAMS, buf, 4);
}

/* !
 * @brief FSK���Ʋ�������
 *
 * @param fdev Ƶ��ƫ�� FrequencyDeviation  �ź�1��0��Ƶ��������Ƶ��֮��Ĳ�ֵ
 * @param bandwidth Ƶ�ʴ���
 * @param shaping ��ͨ�˲���ϵ��
 *      @arg MOD_SHAPING_OFF        0x00
 *      @arg MOD_SHAPING_G_BT_03    0x08
 *      @arg MOD_SHAPING_G_BT_05    0x09
 *      @arg MOD_SHAPING_G_BT_07    0x0A
 *      @arg MOD_SHAPING_G_BT_1     0x0B
 * @param bitrate ��������
 *
 * @note fdev > datarate
 */
void Ebyte_E220x_SetModulationFskParams(uint32e_t fdev, uint8e_t bandwidth, RadioModShapings_t shaping, uint32e_t bitrate)
{
    uint32e_t tempVal = 0;
    uint8e_t buf[8] = {0};

    tempVal = (uint32e_t)(32 * ((double)XTAL_FREQ / (double)bitrate));
    buf[0] = (tempVal >> 16) & 0xFF;
    buf[1] = (tempVal >> 8) & 0xFF;
    buf[2] = tempVal & 0xFF;
    buf[3] = shaping;
    buf[4] = bandwidth;
    tempVal = (uint32e_t)((double)fdev / (double)FREQ_STEP);
    buf[5] = (tempVal >> 16) & 0xFF;
    buf[6] = (tempVal >> 8) & 0xFF;
    buf[7] = (tempVal & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_MODULATIONPARAMS, buf, 8);
}

void Ebyte_E220x_SetCadParams(RadioLoRaCadSymbols_t cadSymbolNum, uint8e_t cadDetPeak, uint8e_t cadDetMin, RadioCadExitModes_t cadExitMode, uint32e_t cadTimeout)
{
    uint8e_t buf[7];

    buf[0] = (uint8e_t)cadSymbolNum;
    buf[1] = cadDetPeak;
    buf[2] = cadDetMin;
    buf[3] = (uint8e_t)cadExitMode;
    buf[4] = (uint8e_t)((cadTimeout >> 16) & 0xFF);
    buf[5] = (uint8e_t)((cadTimeout >> 8) & 0xFF);
    buf[6] = (uint8e_t)(cadTimeout & 0xFF);
    Ebyte_E220x_WriteCommand(RADIO_SET_CADPARAMS, buf, 7);
    OperatingMode = MODE_CAD;
}

void Ebyte_E220x_SetBufferBaseAddress(uint8e_t txBaseAddress, uint8e_t rxBaseAddress)
{
    uint8e_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    Ebyte_E220x_WriteCommand(RADIO_SET_BUFFERBASEADDRESS, buf, 2);
}

/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
uint8e_t Ebyte_E220x_GetFskBandwidthRegValue(uint32e_t bandwidth)
{
    uint8e_t i;

    if (bandwidth == 0)
    {
        return (0x1F);
    }

    for (i = 0; i < (sizeof(FskBandwidths) / sizeof(FskBandwidth_t)) - 1; i++)
    {
        if ((bandwidth >= FskBandwidths[i].bandwidth) && (bandwidth < FskBandwidths[i + 1].bandwidth))
        {
            return FskBandwidths[i + 1].RegValue;
        }
    }
    // ERROR: Value not found
    while (1)
        ;
}

int8e_t Ebyte_E220x_GetRssiInst(void)
{
    uint8e_t buf[1];
    int8e_t rssi = 0;

    Ebyte_E220x_ReadCommand(RADIO_GET_RSSIINST, buf, 1);
    rssi = -buf[0] >> 1;
    return rssi;
}

void Ebyte_E220x_GetFskPacketStatus(PacketStatus_t *pktStatus)
{
    uint8e_t status[3];

    Ebyte_E220x_ReadCommand(RADIO_GET_PACKETSTATUS, status, 3);

    pktStatus->Params.Gfsk.RxStatus = status[0];
    pktStatus->Params.Gfsk.RssiSync = -status[1] >> 1;
    pktStatus->Params.Gfsk.RssiAvg = -status[2] >> 1;
    pktStatus->Params.Gfsk.FreqError = 0;
}

void Ebyte_E220x_GetLoraPacketStatus(PacketStatus_t *pktStatus)
{
    uint8e_t status[3];

    Ebyte_E220x_ReadCommand(RADIO_GET_PACKETSTATUS, status, 3);

    pktStatus->Params.LoRa.RssiPkt = -status[0] >> 1;
    (status[1] < 128) ? (pktStatus->Params.LoRa.SnrPkt = status[1] >> 2) : (pktStatus->Params.LoRa.SnrPkt = ((status[1] - 256) >> 2));
    pktStatus->Params.LoRa.SignalRssiPkt = -status[2] >> 1;
    pktStatus->Params.LoRa.FreqError = FrequencyError;
}

void Ebyte_E220x_ClearDeviceErrors(void)
{
    uint8e_t buf[2] = {0x00, 0x00};
    Ebyte_E220x_WriteCommand(RADIO_CLR_ERROR, buf, 2);
}

/* !
 * @brief ͨ��ģ�鷢������
 *
 * @param payload ָ�����������
 * @param size ���������ݳ���
 * @param timeout ��ʱʱ��
 */
void Ebyte_E220x_SendPayload(uint8e_t *payload, uint8e_t size, uint32e_t timeout)
{

#if (RF_TX_RX_MODE == 0)
    /* LoRa ǰ�����֡��ʽ���� */
    Ebyte_E220x_SetLoraPacketParams(size);
#else
    /* Fsk ǰ�����֡��ʽ���� */
    Ebyte_E220x_SetFskPacketParams(size);
#endif

    /* ������д��ģ���ڲ�FIFO */
    Ebyte_E220x_SetPayload(payload, size);

    /* ģ����뷢��״̬ ��ʼ������������ ��ɺ��ڲ��Ĵ�����λ */
    Ebyte_E220x_SetTx(timeout);

#if (EBYTE_RF_TRANSMIT_CHECK_MODE)
    /* �궨������  ����������ȡ״̬��ʽ  �ȴ�״̬��λ �������������IRQ_TX_DONE=1 �쳣��ʱ��IRQ_RX_TX_TIMEOUT=1*/
    uint16e_t irqStatus = 0;
    do
    {
        Ebyte_Port_DelayMs(1); // �����粻��ʱ�����������ȡ ��õ�0xFFFF
        irqStatus = Ebyte_E220x_GetIrqStatus();
    } while (!irqStatus);

    /* ����ڲ��жϱ�ʶ */
    Ebyte_E220x_ClearIrqStatus(irqStatus);

    /* ͬ��������ʽ�� ��ʱ�Ѿ���ɴ��� ���￪ʼ���÷��ͻص��ӿ� */
    Ebyte_Port_TransmitCallback(irqStatus);
#endif
}

/* !
 * @brief BBYTE ����ģ���ʼ��
 */
void Ebyte_E220x_Init(void)
{
    /* IO��λ+CS���� ģ��*/
    Ebyte_E220x_Reset();
    Ebyte_E220x_Wakeup();

    /* ״̬���趨 */
    OperatingMode = MODE_STDBY_RC;

    /* ���� STDBY_XOSC ��������ģʽ */
    Ebyte_E220x_SetStandby(STDBY_XOSC);
    Ebyte_E220x_WriteRegister(REG_XTA_TRIM, XTA_TRIM_VALUE);
    Ebyte_E220x_WriteRegister(REG_XTB_TRIM, XTA_TRIM_VALUE);

    /* �Ƿ����� DIO2 �Զ�����TXEN */
    Ebyte_E220x_SetDio2AsRfSwitchCtrl(1);

    /* ѡ���ڲ���ѹ������ģʽ ��ЧDC-DC */
    Ebyte_E220x_SetRegulatorMode(USE_DCDC);

    /* �ڲ�FIFO��д��ַ��λ 0x00 */
    Ebyte_E220x_SetBufferBaseAddress(0x00, 0x00);

#if (RF_TX_RX_MODE == 0)
    /* ���ݵ��Ʒ�ʽ LoRa */
    Ebyte_E220x_SetPacketType(PACKET_TYPE_LORA);

    /* LoRa �ؼ����� Ĭ��2.4K���� ��Ƶ����(SF11)  BW(500KHz) CR(4/5) */
    RadioLoRaBandwidths_t bw = (RadioLoRaBandwidths_t)(Ebyte_E220x_LoraBpsTable[RF_LORA_AIR_BPS][0]);
    RadioLoRaSpreadingFactors_t sf = (RadioLoRaSpreadingFactors_t)(Ebyte_E220x_LoraBpsTable[RF_LORA_AIR_BPS][1]);
    RadioLoRaCodingRates_t cr = ((RadioLoRaCodingRates_t)Ebyte_E220x_LoraBpsTable[RF_LORA_AIR_BPS][2]);
    Ebyte_E220x_SetModulationLoraParams(sf, bw, cr);

    /* LoRa ͬ���� */
    Ebyte_E220x_SetLoraSyncWord(LORA_MAC_PUBLIC_SYNCWORD);

    /* LoRa ǰ�����֡��ʽ���� */
    Ebyte_E220x_SetLoraPacketParams(0xFF);
#else
    /* ���ݵ��Ʒ�ʽ GFSK */
    Ebyte_E220x_SetPacketType(PACKET_TYPE_GFSK);

    /* FSK �ؼ�����  */
    uint32e_t bw = Ebyte_E220x_GetFskBandwidthRegValue(RF_FSK_BANDWIDTH);
    Ebyte_E220x_SetModulationFskParams(RF_FSK_FREQUENCY_DEVIATION, bw, MOD_SHAPING_G_BT_1, RF_FSK_DATA_RATE);

    /* FSK ǰ�����֡��ʽ���� */
    Ebyte_E220x_SetFskPacketParams(0xFF);

    /* FSK ͬ����  */
    Ebyte_E220x_SetFskSyncWord();

    Ebyte_E220x_SetWhiteningSeed(0x01FF);
#endif

    /* �����ز�Ƶ��(Ƶ��) */
    uint32e_t fs = (RF_FREQUENCY_START + (RF_FREQUENCY_CH * RF_FREQUENCY_CH_STEP));
    Ebyte_E220x_SetRfFrequency(fs);

    /* ���书��dbm  */
    Ebyte_E220x_SetTxPaParams(RF_TX_OUTPUT_POWER, RADIO_RAMP_3400_US);

    /* ģ��Ĭ�Ͻ������ģʽ */
    Ebyte_E220x_SetRx(0);
}

/* !
 * @brief ���� ������DIOӲ���ж�
 */
void Ebyte_E220x_InterruptTrigger(void)
{
}

/* !
 * @brief BBYTE ����ģ����������
 *
 * @note ���뱻�ͻ������Ե��ã������޷����յ�����
 */
void Ebyte_E220x_IntOrPollTask(void)
{
    /* ��ȡ�ڲ�״̬ */
    uint16e_t irqStatus = Ebyte_E220x_GetIrqStatus();
    uint8e_t recvLength = 0;

    /* �����������״̬ Ĭ��0 �����0�򴥷�����/�� */
    if (irqStatus != 0)
    {
        /* ���ģ���ڲ�״̬�Ĵ���*/
        Ebyte_E220x_ClearIrqStatus(irqStatus);

        /* ����ǽ������״̬ */
        if ((irqStatus & IRQ_RX_DONE) == IRQ_RX_DONE)
        {
            /* ��ȡ���ݰ�״̬��Ϣ */
#if (RF_TX_RX_MODE == 0)
            Ebyte_E220x_GetLoraPacketStatus(&PacketStatus);
#else
            Ebyte_E220x_GetFskPacketStatus(&PacketStatus);
#endif
            /* ��ȡ���ݰ�*/
            Ebyte_E220x_GetPayload(RecvPacketBuffer, &recvLength, RF_RX_BUFFER_SIZE);

            /* ������� ��ʼ�ص� */
            Ebyte_Port_ReceiveCallback(irqStatus, RecvPacketBuffer, recvLength);

            /* �������� */
            Ebyte_E220x_SetRx(0);
        }
        else
        {
            /* ��ʱ���ߴ��� ��ʼ�ص� */
            Ebyte_Port_ReceiveCallback(irqStatus, RecvPacketBuffer, recvLength);
        }
    }
}

/* !
 * @brief ��ȡģ������
 *
 * @return 32λ�ı���
 * @note ��16λ����ģ���� ����0x0220 ����E220ģ��
 *       ��16λ����Ƶ���� ����0x0400 ����400Ƶ��
 */
uint32e_t Ebyte_E220x_GetName(void)
{
    return ((((uint32e_t)EBYTE_E220_NAME_TYPE << 16)) | EBYTE_E220_FREQUENCY_TYPE);
}

/* !
 * @brief ��ȡģ�����汾
 *
 * @return 8λ�ı���
 * @note ����0x10 ����V1.0
 */
uint8e_t Ebyte_E220x_GetDriverVersion(void)
{
    return EBYTE_E220_PROGRAM_TYPE;
}
