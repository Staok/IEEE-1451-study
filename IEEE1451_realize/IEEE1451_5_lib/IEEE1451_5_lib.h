#ifndef IEEE1451_5_LIB_H
#define IEEE1451_5_LIB_H

#include <stdint.h>

/* 以下程序分为四个部分：
    TEDS 格式定义
    TEDS API 定义
    Message 格式定义
    Message API 定义 

    关于占用内存空间，在 TEDS 和 Message 结构体构建时候 的
        联合体 里面都是按照一帧最大 200 字节定义的：
        
        四个 TEDS 就 四个 200 byte，
        Message 和 ReplyMessage 共两个 200 byte 
        另外再加 Message_temp 和 ReplyMessage_temp 共两个 200 byte
        
        就是至少 1600 Byte
*/

/* 宏 NEED_SWITCH_LITTLE_BIG_END
    是否需要大小端切换
    在本版本中，存储和发送都是按照所在平台的大小端来进行，
    但对于两个不同大小端之间的数据传输，接收时候需要大小端转换，
    如果 NEED_SWITCH_LITTLE_BIG_END 宏 为 真，则表示需要在 接收的时候进行大小端转换， 0 为不需要。
    
    本库内部根据 NEED_SWITCH_LITTLE_BIG_END 的值自动做处理，用户只需 按需修改 NEED_SWITCH_LITTLE_BIG_END 即可。

    注：小端存储为以字节为最小单位按照 MSB 顺序排列，即变量的高字节放在寄存器的高位，低字节放在地位，大端与之相反。
*/
#define NEED_SWITCH_LITTLE_BIG_END		0

#ifdef __cplusplus
	extern "C" 
	{
#endif

enum TIM_status_enum
{
    Initializing = 0,
    sleep,
    Idle,
    Operating,
};

extern enum TIM_status_enum TIM_status;

/* TIM 枚举，用于标识，动态入网的时候，标记自己是哪一个 TIM */
enum TIM_enum
{
    TIM_1 = 0,
    TIM_2,
    TIM_3,
    TIM_4,
    TIM_5,
    TIM_6,
    TIM_7,
    TIM_8,
    TIM_9,
    TIM_10,
    TIM_11,
    TIM_12,
    TIM_MAX /* 表示 ALL */
};

/* TIM 下 传感器通道的枚举，用于标识 */
enum TransducerChannel_enum
{
    TC_1 = 0,
    TC_2,
    TC_3,
    TC_4,
    TC_5,
    TC_6,
    TC_7,
    TC_8,
    TC_9,
    TC_10,
    TC_11,
    TC_12,
    TC_13,
    TC_14,
    TC_15,
    TC_16,
    TC_17,
    TC_18,
    TC_19,
    TC_20,
    TC_21,
    TC_22,
    TC_23,
    TC_24,
    TC_MAX /* 表示 ALL */
};


extern uint8_t temp_load[200];
extern uint32_t temp_load_valid_length;

                                    /*************\
*************************************     TEDS    *****************************************************
                                    *   格式定义  *
                                    \*************/

#define M_TEDS_ACCESS_CODE      1
#define TC_TEDS_ACCESS_CODE     3
#define UTN_TEDS_ACCESS_CODE    12
#define PHY_TEDS_ACCESS_CODE    13

/* 这个数要大于下面 TEDS 结构体整体的大小，不要太小 */
#define MAX_TEDS_LOAD_SIZE 200

/*************************** TEDS 属性 结构体 定义 ***************************/
struct TEDS_attributes_struct
{
    uint8_t ReadOnly    :1;     /* Read-only—Set to true if TEDS may be read but not written. */
    
    uint8_t NotAvail    :1;     /* Unsupported—Set to true if TEDS is not supported by this TransducerChannel.  */
    
    uint8_t Invalid     :1;     /* Invalid—Set to true if the current TEDS image is invalid.  */
   
    uint8_t  Virtual     :1;    /* Virtual TEDS—This bit is set to true if this is a virtual 
                                TEDS. (A virtual TEDS is any TEDS that is not stored in 
                                the TIM. The responsibility for accessing a virtual TEDS 
                                is vested in the NCAP or host processor.)  */
   
    uint8_t TextTEDS    :1;     /* Text TEDS—Set to true if the TEDS is text based.  */
   
    uint8_t Adaptive    :1;     /* Adaptive—Set to true if the contents of the TEDS can be 
                                changed by the TIM or TransducerChannel without the 
                                NCAP issuing a WriteTEDS segment command.  */
   
    uint8_t MfgrDefine  :1;     /* MfgrDefine—Set to True if the contents of this TEDS are 
                                defined by the manufacturer and will only conform to the 
                                structures defined in the standard if the TextTEDS 
                                attribute is also set.   */
   
    uint8_t Reserved    :1;     /* Reserved */
};

/*************************** TEDS 状态 结构体定义 ***************************/
struct TEDS_status_struct
{
    uint8_t TooLarge        :1; /* Too Large—The last TEDS image received was too large to fit in the memory allocated to this TEDS.  */
    
    uint8_t Reserved_1      :1; /* 为 IEEE 1451 标准做保留 */
    uint8_t Reserved_2      :1;
    uint8_t Reserved_3      :1;

    uint8_t Open_4          :1; /* Open to manufacturers */
    uint8_t Open_5          :1;
    uint8_t Open_6          :1;
    uint8_t Open_7          :1;
};

#pragma pack(1) /* 让编译器做 1 字节对齐 */

/*************************** TEDS 头 通用结构体 ***************************/
struct TEDS_ID_struct
{
	uint8_t Type;    		/* TEDS 头的域类号为 3 */
	uint8_t Length;  		/* 总为 4 */
	uint8_t Family;  		/* IEEE 1451.x */
	uint8_t access_code; 	/* TEDS 的代码 */
	uint8_t Version;		/* TEDS 版本，1 为初始版本，往后排 */
	uint8_t Tuple_Length;	/* TLV 中 Length 占的字节数，为 1*/
};

/*************************** Meta TEDS 定义 ***************************/

/* Meta TEDS 数据区 每一个域类一个结构体 */
struct M_TEDS_UUID_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value[10];
};

struct M_TEDS_OholdOff_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct M_TEDS_SHoldOff_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct M_TEDS_TestTime_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct M_TEDS_MaxChan_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

/* Meta TEDS 结构构建 */
struct Meta_TEDS_struct
{
	uint32_t Length;			/* 包括 DATA BLOCK 和 CHECKSUM 的 长度 */
	
	struct TEDS_ID_struct ID;	/* TEDS 头，Value 占 4 个字节 */
	
	struct M_TEDS_UUID_TLV_struct UUID;		    /* Globally Unique Identifier，Value 占 10 个字节 */
	struct M_TEDS_OholdOff_TLV_struct OholdOff; /* Operational time-out，整数，单位秒，Value 占 4 个字节 */
	struct M_TEDS_SHoldOff_TLV_struct SHoldOff; /* Slow-access time-out，整数，单位秒，Value 占 4 个字节 */
	struct M_TEDS_TestTime_TLV_struct TestTime;	/* Self-Test Time，整数，单位秒，Value 占 4 个字节 */
	struct M_TEDS_MaxChan_TLV_struct MaxChan;	/* Number of implemented TransducerChannels，整数，Value 占 2 个字节 */
	
	/* Group 相关内容过于复杂，略过 */

	uint16_t Checksum;			/* TEDS 校验，从 TED Length（最开头） 到 DATA BLOCK 的最后一个字节（本域类的上一个字节） 的加和，再用 0xFFFF 减去该加和值。 */
};

/*************************** TransducerChannel TEDS 定义 ***************************/
enum TC_TEDS_CalKey_Value_enum
{
	CAL_NONE = 0,
	CAL_SUPPLIED,
	CAL_CUSTOM,
	TIM_CAL_SUPPLIED,
	TIM_CAL_SELF,
	TIM_CAL_CUSTOM,
	Reserved_for_CalKey
};

struct TC_TEDS_CalKey_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

enum TC_TEDS_ChanType_Value_enum
{
	Sensor = 0,
	Actuator,
	Event_sensor,
	Reserved_for_ChanType
};

struct TC_TEDS_ChanType_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

enum Physical_Units_interpretation_enum
{
    PUI_SI_UNITS = 0,
    PUI_RATIO_SI_UNITS,
    PUI_LOG10_SI_UNITS,
    PUI_LOG10_RATIO_SI_UNITS,
    PUI_DIGITAL_DATA,
    PUI_ARBITRARY
};

struct TC_TEDS_PhyUnits_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    struct Units_struct
    {
        uint8_t interpretation;
        uint8_t radians; 
        uint8_t steradians; 
        uint8_t meters; 
        uint8_t kilograms; 
        uint8_t seconds; 
        uint8_t amperes; 
        uint8_t kelvins; 
        uint8_t moles; 
        uint8_t candelas;
        uint8_t Units_Extension_TEDS_Access_Code;
    }Value;
};

struct TC_TEDS_LowLimit_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_HiLimit_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_OError_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_SelfTest_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_MRange_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_Sample_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_DataSet_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_UpdateT_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_WSetupT_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_RSetupT_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_SPeriod_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_WarmUpT_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_RDelayT_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_TestTime_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_TimeSrc_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_InPropDl_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_OutPropD_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_TSError_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_Sampling_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_DataXmit_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_Buffered_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_EndOfSet_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_EdgeRpt_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_ActHalt_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct TC_TEDS_Directon_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value;
};

struct TC_TEDS_DAngles_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    float Value[2];
};

struct TC_TEDS_ESOption_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};


struct TransducerChannel_TEDS_struct
{
	uint32_t Length;

	struct TEDS_ID_struct ID;

	struct TC_TEDS_CalKey_TLV_struct CalKey; /* 10  CalKey  Calibration key  uint8_t  1  */
	struct TC_TEDS_ChanType_TLV_struct ChanType; /* 11  ChanType  TransducerChannel type key  uint8_t  1 */
	struct TC_TEDS_PhyUnits_TLV_struct PhyUnits; /* 12  PhyUnits  Physical Units  UNITS  11  */
    /* 域类 50 ~ 60略 */
	struct TC_TEDS_LowLimit_TLV_struct LowLimit; /* 13  LowLimit  Design operational lower range limit  Float32  4 */
	struct TC_TEDS_HiLimit_TLV_struct HiLimit; /* 14  HiLimit  Design operational upper range limit  Float32  4 */
	struct TC_TEDS_OError_TLV_struct OError; /* 15  OError  Worst-case uncertainty  Float32  4 */
	struct TC_TEDS_SelfTest_TLV_struct SelfTest; /* 16  SelfTest  Self-test key  uint8_t  1 */
	struct TC_TEDS_MRange_TLV_struct MRange; /* 17  MRange  Multi-range capability  uint8_t  1 */
	struct TC_TEDS_Sample_TLV_struct Sample; /* 18  Sample 略 */
	struct TC_TEDS_DataSet_TLV_struct DataSet; /* 19  DataSet 略 */
	struct TC_TEDS_UpdateT_TLV_struct UpdateT; /* 20  UpdateT  TransducerChannel update time (tu)  Float32  4 */
	struct TC_TEDS_WSetupT_TLV_struct WSetupT; /* 21  WSetupT  TransducerChannel write setup time (tws)  Float32  4  */
	struct TC_TEDS_RSetupT_TLV_struct RSetupT; /* 22  RSetupT  TransducerChannel read setup time (trs)  Float32  4  */
	struct TC_TEDS_SPeriod_TLV_struct SPeriod; /* 23  SPeriod  TransducerChannel sampling period (tsp)  Float32  4  */
	struct TC_TEDS_WarmUpT_TLV_struct WarmUpT; /* 24  WarmUpT  TransducerChannel warm-up time  Float32  4  */
	struct TC_TEDS_RDelayT_TLV_struct RDelayT; /* 25  RDelayT  TransducerChannel read delay time (tch)  Float32  4  */
	struct TC_TEDS_TestTime_TLV_struct TestTime; /* 26  TestTime  TransducerChannel self-test time requirement  Float32  4  */
	struct TC_TEDS_TimeSrc_TLV_struct TimeSrc; /* 27  TimeSrc  Source for the time of sample  uint8_t  1  */
	struct TC_TEDS_InPropDl_TLV_struct InPropDl; /* 28  InPropDl  Incoming propagation delay through the data transport logic  Float32  4  */
	struct TC_TEDS_OutPropD_TLV_struct OutPropD; /* 29  OutPropD  Outgoing propagation delay through the data transport logic  Float32  4  */
	struct TC_TEDS_TSError_TLV_struct TSError; /* 30  TSError  Trigger-to-sample delay uncertainty  Float32  4  */
	struct TC_TEDS_Sampling_TLV_struct Sampling; /* 31  Sampling  Sampling attribute 略 */
	struct TC_TEDS_DataXmit_TLV_struct DataXmit; /* 32  DataXmit  Data transmission attribute  uint8_t  1 */
	struct TC_TEDS_Buffered_TLV_struct Buffered; /* 33  Buffered  Buffered attribute  uint8_t  1  */
	struct TC_TEDS_EndOfSet_TLV_struct EndOfSet; /* 34  EndOfSet  End-of-data-set operation attribute  uint8_t  1  */
	struct TC_TEDS_EdgeRpt_TLV_struct EdgeRpt; /* 35  EdgeRpt  Edge-to-report attribute  uint8_t  1  */
	struct TC_TEDS_ActHalt_TLV_struct ActHalt; /* 36  ActHalt  Actuator-halt attribute  uint8_t  1  */
	struct TC_TEDS_Directon_TLV_struct Directon; /* 37  Directon  Sensitivity direction  Float32  4  */
	struct TC_TEDS_DAngles_TLV_struct DAngles; /* 38  DAngles  Direction angles  Two Float32 8 */
	struct TC_TEDS_ESOption_TLV_struct ESOption; /* 39  ESOption  Event sensor options  uint8_t  1  */

	uint16_t Checksum;
};

/*************************** User’s transducer name TEDS 定义 ***************************/
struct UTN_TEDS_Format_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct UTN_TEDS_TCName_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    char Value[25];
};


struct User_Transducer_Name_TEDS_struct
{
	uint32_t Length;

	struct TEDS_ID_struct ID;

    struct UTN_TEDS_Format_TLV_struct Format;
    struct UTN_TEDS_TCName_TLV_struct TCName;

    uint16_t Checksum;
};

/*************************** PHY TEDS 定义 ***************************/
enum PHY_TEDS_RadioType_Value_enum
{
    IEEE_802_11 = 0,
    Bluetooth,
    ZigBee,
    LoWPAN,
    Reserved_for_future_expansion_RadioType, 
};

struct PHY_TEDS_RadioType_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct PHY_TEDS_MaxBPS_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct PHY_TEDS_MaxCDev_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

struct PHY_TEDS_MaxRDev_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

enum PHY_TEDS_Encrypt_Value_enum
{
    No_encryption = 0,
    AES,
    SAFER,
    _3DES,
    Reserved_for_future_expansion_Encrypt,
};

struct PHY_TEDS_Encrypt_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value[2];
};

struct PHY_TEDS_Authent_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct PHY_TEDS_MinKeyL_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

struct PHY_TEDS_MaxKeyL_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

struct PHY_TEDS_MaxSDU_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

struct PHY_TEDS_MinALat_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct PHY_TEDS_MinTLat_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint32_t Value;
};

struct PHY_TEDS_MaxXact_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct PHY_TEDS_Battery_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint8_t Value;
};

struct PHY_TEDS_RadioVer_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};

struct PHY_TEDS_MaxRetry_TLV_struct
{
    uint8_t Type;
    uint8_t Length;
    uint16_t Value;
};


struct PHY_TEDS_struct
{
	uint32_t Length;

	struct TEDS_ID_struct ID;

    struct PHY_TEDS_RadioType_TLV_struct RadioType; /* 10  Radio  Radio Type  UInt8  1  See Table 4 */
    struct PHY_TEDS_MaxBPS_TLV_struct MaxBPS; /* 11  MaxBPS  Max data throughput  UInt32  4  Bits per second */
    struct PHY_TEDS_MaxCDev_TLV_struct MaxCDev; /* 12  MaxCDev  Max Connected Devices UInt16  2  Maximum number of devices that may be simultaneously operational with this device  */
    struct PHY_TEDS_MaxRDev_TLV_struct MaxRDev; /* 13  MaxRDev  Max Registered Devices UInt16  2  Maximum number of devices that may be simultaneously registered with this device */
    struct PHY_TEDS_Encrypt_TLV_struct Encrypt; /* 14  Encrypt  Encryption  UInt16  2  Encryption type and key length  */
    struct PHY_TEDS_Authent_TLV_struct Authent; /* 15  Authent  Authentication  Boolean  1  TRUE = Authentication supported FALSE = not supported  */
    struct PHY_TEDS_MinKeyL_TLV_struct MinKeyL; /* 16  MinKeyL  Min Key Length  UInt16  2  Minimum key length for security functions  */
    struct PHY_TEDS_MaxKeyL_TLV_struct MaxKeyL; /* 17  MaxKeyL  Max Key Length  UInt16  2  Maximum key length for security functions  */
    struct PHY_TEDS_MaxSDU_TLV_struct MaxSDU; /* 18   MaxSDU  Max SDU Size  UInt16  2  Maximum payload size for transfer between devices */
    struct PHY_TEDS_MinALat_TLV_struct MinALat; /* 19  MinALat  Min Access Latency  UInt32  4  Minimum period for initiating a first transmission to an unconnected device  */
    struct PHY_TEDS_MinTLat_TLV_struct MinTLat; /* 20  MinTLat  Min Transmit Latency  UInt32  4  The period for transmitting an N-octet payload, where N is the minimum payload size  */
    struct PHY_TEDS_MaxXact_TLV_struct MaxXact; /* 21  MaxXact  Max Simultaneous Transactions UInt8  1  Maximum number of queued, uncompleted commands  */
    struct PHY_TEDS_Battery_TLV_struct Battery; /* 22  Battery   Device is battery powered UInt 8  1  A nonzero value indicates battery power  */
    struct PHY_TEDS_RadioVer_TLV_struct RadioVer; /* 23  RadioVer  Radio Version #  UInt16  2  A zero value indicates unknown  */
    struct PHY_TEDS_MaxRetry_TLV_struct MaxRetry; /* 24  MaxRetry  Maximum Retries before Disconnect Uint16  2  A zero value indicates never disconnect  */
    
    uint16_t Checksum;
};

#pragma pack() /* 取消 1 字节对齐，恢复为默认对齐 */

/*************************** TEDS 联合体定义 ***************************/
    /* 为了可以逐字节的处理 TEDS 结构体 */

union Meta_TEDS_union
{
    struct Meta_TEDS_struct M_TEDS;
    uint8_t TEDS_load[MAX_TEDS_LOAD_SIZE];
};

union TransducerChannel_TEDS_union
{
    struct TransducerChannel_TEDS_struct TC_TEDS;
    uint8_t TEDS_load[MAX_TEDS_LOAD_SIZE];
};

union User_Transducer_Name_TEDS_union
{
    struct User_Transducer_Name_TEDS_struct UTN_TEDS;
    uint8_t TEDS_load[MAX_TEDS_LOAD_SIZE];
};

union PHY_TEDS_union
{
    struct PHY_TEDS_struct PHY_TEDS;
    uint8_t TEDS_load[MAX_TEDS_LOAD_SIZE];
};


/*************************** TEDS 总结构体，用户使用 ***************************/
struct TEDS_struct
{
        /* 下面 TEDS 联合、属性 结构体 和 状体 结构体 都是 指针，用于承接 .c 文件里面 例化的 实体结构体 的 地址  */
    union Meta_TEDS_union *                  M_TEDS_u;   uint32_t M_TEDS_load_Length;
    union TransducerChannel_TEDS_union *     TC_TEDS_u;  uint32_t TC_TEDS_load_Length;
    union User_Transducer_Name_TEDS_union *  UTN_TEDS_u; uint32_t UTN_TEDS_load_Length;
    union PHY_TEDS_union *                   PHY_TEDS_u; uint32_t PHY_TEDS_load_Length;

    struct TEDS_attributes_struct * M_TEDS_attr;
    struct TEDS_attributes_struct * TC_TEDS_attr;
    struct TEDS_attributes_struct * UTN_TEDS_attr;
    struct TEDS_attributes_struct * PHY_TEDS_attr;

    struct TEDS_status_struct * M_TEDS_status;
    struct TEDS_status_struct * TC_TEDS_status;
    struct TEDS_status_struct * UTN_TEDS_status;
    struct TEDS_status_struct * PHY_TEDS_status;

	/* TEDS 的名称字符串 */
    uint8_t* M_TEDS_str;
    uint8_t* TC_TEDS_str;
    uint8_t* UTN_TEDS_str;
    uint8_t* PHY_TEDS_str;
};


extern struct TEDS_struct   TEDS;

                                    /*************\
*************************************     TEDS    *****************************************************
                                    *   API定义   *
                                    \*************/

/* API 具体注释看 .c 文件 函数定义处 */

void TEDS_init(void);

void TEDS_pack_up(uint8_t* dest_loader,uint32_t* length,uint8_t access_code);

void TEDS_decode(uint8_t* TEDS_load);   /* TODO ，暂不实现 */

                                    /*************\
*************************************   Message    *****************************************************
                                    *   格式定义   *
                                    \*************/

#define MAX_Message_dependent_SIZE            200

/* 命令类 枚举（Command Class） */
enum Command_class_enum
{
    Reserved_Command_class = 0,
    CommonCmd,      /* 给 TIM 的通用命令（Commands common to the TIM and TransducerChannel）  */
    
    XdcrIdle,       /* 传感器 空闲状态命令（XdcrIdle，Transducer idle state commands）  */
    XdcrOperate,    /* 传感器 工作状态命令（XdcrOperate，Transducer operating state commands） */
    XdcrEither,     /* 传感器 空闲状态或工作状态命令（XdcrEither，Transducer either idle or operating state commands） */
    
    TIMsleep,       /* TIM 睡眠状态命令（TIM sleep state commands） */
    TIMActive,      /* TIM 激活状态命令（TIM active state commands） */
    AnyState,       /* TIM 任何状态命令（TIM any state commands） */
    
    ReservedClass = 8,    /* 编号 8—127 做为 1451 标准的保留 */
    ClassN = 128,         /* 编号 128—255 为用户保留，用户可以自定 */
};

/* 给 TIM 的通用命令枚举 CommonCmd（Commands common to the TIM and TransducerChannel）  */
    /* 即 Command Class 为 CommonCmd 时，Command function 具体的值 如下 */
enum CommonCmd_commands_enum
{
    Reserved_Common_command = 0,
    
    Query_TEDS,                     /* Query TEDS command（询问 TEDS 信息命令） */
    Read_TEDS_segment,
    Write_TEDS_segment,
    Update_TEDS,
    
    Run_self_test,
    
    Write_service_request_mask,
    Read_service_request_mask,
    
    Read_StatusEvent_register,
    Read_StatusCondition_register,
    Clear_StatusEvent_register, 
    Write_StatusEvent_protocol_state, 
    Read_StatusEvent_protocol_state,

    ReservedCommands = 13,       /* 编号 13–127 做为 1451 标准的保留  */
    Open_Common_commands_for_manufacturers = 128, /* 编号 128—255 为用户保留，用户可以自定 */

    /* 这里自定 TIM 初始化完毕标志 */
    TIM_ALL_TC_initiated = 130,
};

/* 传感器 空闲状态命令枚举（XdcrIdle，Transducer idle state commands）  */
    /* 即 Command Class 为 XdcrIdle 时，Command function 具体的值 如下，以此类推 */
enum XdcrIdle_commands_enum
{
    Reserved_XdcrIdle_command = 0,
    Set_TransducerChannel_data_repetition_count, 
    Set_TransducerChannel_pre_trigger_count,
    AddressGroup_definition,
    Sampling_mode,
    Data_Transmission_mode,
    Buffered_state,
    End_of_data_set_operation,
    Actuator_halt_mode,
    Edge_to_report,
    Calibrate_TransducerChannel,
    Zero_TransducerChannel,
    Write_corrections_state,
    Read_corrections_state,
    Write_TransducerChannel_initiate_trigger_state,
    Write_TransducerChannel_initiate_trigger_configuration,
    
    Reserved_XdcrIdle_commands, /* 编号 16—127 做为 1451 标准的保留 */
    XdcrIdle_commands_Open_for_manufacturers,   /* 编号 128—255 为用户保留，用户可以自定 */
};

/* 传感器 工作状态命令枚举（XdcrOperate，Transducer operating state commands） */
enum XdcrOperate_commands_enum
{
    Reserved_XdcrOperate_command = 0,
    Read_TransducerChannel_data_set_segment,
    Write_TransducerChannel_data_set_segment,
    Trigger_command,
    Abort_Trigger,

    Reserved_XdcrOperate_commands,  /* 编号 5—127 做为 1451 标准的保留 */
    Open_XdcrOperate_commands_for_manufacturers, /* 编号 128—255 为用户保留，用户可以自定 */
};

/* 其他 Command class 的 commands 在这挨个枚举 ... 
    
    传感器 空闲状态或工作状态命令（XdcrEither，Transducer either idle or operating state commands），
    此命令类下的具体命令见 1451.0 标注源文的 Table-34。
    
    TIM 睡眠状态命令（TIM sleep state commands），
    该命令只在 TIM 睡眠状态下有效，此命令类下的具体命
    令见 1451.0 标注源文的 Table-35，用于唤醒，目前只此一个命令，其他均作保留。
    
    TIM 激活状态命令（TIM active state commands），
    该命令只在 TIM 激活状态下有效，此命令类下的具体命
    令见 1451.0 标注源文的 Table-36。
    
    TIM 任何状态命令（TIM any state commands），
    此命令类下的具体命令见 1451.0 标注源文的 Table-37，
    用于复位 TIM，目前只此一个命令，其他均作保留。 
*/


enum Data_Transmission_mode_enum
{
    Data_Transmission_Mode_reserved = 0,
    OnCommand,  /* A TIM shall transmit a data set only in response to a Read TransducerChannel data-set segment command */   
    BufferFull,
    Interval,   /* TIM 固定周期性上传数据 */
    
    /* 编号 4 ~ 127 给标准做保留 */
    /* 编号 128 ~ 255 给 用户保留 */

    OnCommand_custom = 129,/* 自己定义上传数据模式
                                NCAP 发送 读 数据 命令 给 TIM 的时候， TIM 空闲时候则响应，开始采集 1s 数据并上传；
                                若TIM 正在工作则继续原来的采集和上传，不予回应 */
    
    BufferHalfFull = 130,   /* 自己定义上传数据模式， BufferHalfFull，即 缓冲区半满 上传一次数据*/
    Interval_1s = 131,      /* 自己定义上传数据模式，每秒发送一帧数据，但上位机显示的是连续的 */
};

enum TIM_and_TC_enum
{
    TIM_enum = 0,
    TC_enum
};

#pragma pack(1) /* 让编译器做 1 字节对齐 */

/* 消息/命令 帧 结构体，也用于 TIM 发送初始化完毕的消息 */
struct Message_struct
{
    uint8_t Dest_TIM_and_TC_Num[2]; /* This field gives the 16 bit TransducerChannel number for the destination of the message.  */
    
    uint8_t Command_class;
    uint8_t Command_function;
    
    uint16_t dependent_Length; /* Length is the number of command-dependent octets in this message.  */
    uint8_t dependent_load[MAX_Message_dependent_SIZE]; /* 用于暂存 command_dependent，有效长度为 dependent_Length */
};

/* 回复/响应 帧 结构体，还用于 TIM 发传感器数据 */
struct ReplyMessage_struct
{
    uint8_t Flag; /* If this octet is nonzero, it indicates that the command was successfully completed. If it is zero, the 
                    command failed and the system should check the status to determine why. */
    uint16_t dependent_Length; /* Length is the number of reply-dependent octets in this message.  */
    /* Peply_Message_struct 中的附带参数长度 dependent_Length 占俩字节，最大 65535，即一帧回复帧中 Reply dependent 的最大字节数 */
    uint8_t dependent_load[MAX_Message_dependent_SIZE]; /* 用于暂存 Reply dependent，有效长度为 dependent_Length */
    /* 这里我自己再定义，dependent 的尾部再添加两个字节，表回复对应命令的 class 和 command */
};

/* 24 位数字麦克风以 20KHz 采样，一秒的数据量字节数为 3 * 20 * 1000 =  60,000 个字节，十六进制为 0xEA60，
    即用 Peply_Message_struct 来发数据，可以一次发一秒的音频数据 但实际不采用使用 ReplyMessage 来上传数据
    这里只简单计算，不为实际参数 */

#pragma pack() /* 取消 1 字节对齐，恢复为默认对齐 */


/*************************** Message 和 ReplyMessage 联合体定义 ***************************/
    /* 这么做 让 Message 和 ReplyMessage 结构体可以 逐字节 处理 */

union Message_union
{
    struct Message_struct Message;
    uint8_t Message_load[MAX_Message_dependent_SIZE + 10]; 
        /* 数组空间 里面 +10 是因为 Message 结构体里面 除了 dependent 的 MAX_Message_dependent_SIZE 还有 前面几个 不超过 10 byte 的变量  */
};

union ReplyMessage_union
{
    struct ReplyMessage_struct ReplyMessage;
    uint8_t ReplyMessage_load[MAX_Message_dependent_SIZE + 10];
};

/*************************** Message 和 ReplyMessage 总结构体，供用户用 ***************************/
struct MES_struct
{
    /* 联合指针形式，在 Message_init() 中 将 .c 中的 联合体 实体的地址赋给这里 */
    union Message_union*        Message_u;      uint32_t Message_load_Length;
    union ReplyMessage_union*   ReplyMessage_u; uint32_t ReplyMessage_load_Length;
};

extern struct MES_struct MES;

extern struct Message_struct Message_temp;
extern struct ReplyMessage_struct ReplyMessage_temp;

                                    /*************\
*************************************   Message   *****************************************************
                                    *   API定义   *
                                    \*************/

/* API 具体注释看 .c 文件 函数定义处，具体使用实例看 .c 最上面 注释 */

/**************************** Message init，用户使用 ****************************/
void Message_init(void);

/**************************** 根据不同命令 填充 Message 结构体 并打包数据的 API，用户使用 ****************************/
/* 打包好的消息数据在 MES.Message_u->Message_load 里面，有效数据长度为 MES.Message_load_Length */
void Message_CommonCmd_Query_TEDS_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint8_t which_TEDS);
void Message_CommonCmd_Read_TEDS_segment_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint8_t which_TEDS, uint32_t TEDSOffset);
void Message_XdcrIdle_Set_Data_Transmission_mode_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC,enum Data_Transmission_mode_enum mode);
void Message_XdcrOperate_Read_TC_data_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint32_t Offset);
void Message_XdcrOperate_Trigger_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC);
void Message_XdcrOperate_Abort_Trigger_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC);
void Message_TIM_initiated_pack_up(void);

/**************************** 消息的 发送，用户使用 ****************************/
void Message_pack_up_And_send(void);

/**************************** 解析接收到的 Message 的 API ****************************/
// void Message_decode(struct Message_struct* messageReceived,uint8_t* mes_load);

/**************************** 根据相应 Message 填充 ReplyMessage 结构体 并打包数据的 API， 的 API ****************************/
/* 打包好后的回复消息数据在 MES.ReplyMessage_u->ReplyMessage_load 里面，有效数据长度为 MES.ReplyMessage_load_Length */
// void ReplyMessage_CommonCmd_Query_TEDS_pack_up(uint8_t which_TEDS);
// void ReplyMessage_CommonCmd_Read_TEDS_segment_pack_up(uint8_t which_TEDS, uint32_t TEDSOffset);
// void ReplyMessage_XdcrIdle_Data_Transmission_mode_pack_up(void);
// void ReplyMessage_XdcrOperate_Read_TC_data_pack_up(void);
// void ReplyMessage_XdcrOperate_Trigger_pack_up(void);
// void ReplyMessage_XdcrOperate_Abort_Trigger_pack_up(void);
// void ReplyMessage_TIM_initiated_pack_up(void);

/**************************** 回复消息的 发送 ****************************/
// uint8_t ReplyMessage_send(uint8_t class, uint8_t command);

/**************************** 解析接收到的 回复消息 的 API ****************************/
void ReplyMessage_decode(struct ReplyMessage_struct* replyMessageReceived,uint8_t* received_rep_mes_load);

/**************************** ReplyMessage 服务程序，自动解析 Message 并回复，用户使用  ****************************/
void ReplyMessage_Server(uint8_t* received_mes_load);

#ifdef __cplusplus
	}
#endif

#endif


