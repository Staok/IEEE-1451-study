/*************************************************
    IEEE 1451 & IEEE 1451.5 实现库
Version:     2.0

Description: 
    看下面 “流程”

Others:      
    其它内容的说明：
    IEEE 1451 标准特别絮叨，以下列举没有在本程序中 实现的部分（主要部分，不是全部）：
    - TransducerChannel TEDS 的一些 标准源文要求 “必要” 有 的 域类没有加入到结构体中。
    - 没有细致读标准源文，所以不清楚如果一个 TIM 下面挂接 多个 传感器通道，同时其种类有很多种，这种情形下该如何用 TEDS 描述（一定又相当繁琐）。
    - 在下面 Message API 定义 里面可以看到只实现了 6 个左右的信息命令及其回复信息。
    - 标准源文描述的功能非常全面了，但实现起来又只是纯体力活，所以 这里只实现最最最基本的功能。
        TEDS 和 Message 的 基本数据结构 在 .h 文件里面。
        
    但最后来看实现的水平也一般，应该先好好规划规划；
    感觉有的地方实现的比较笨，比较直接不巧妙，个人业余去复现标准，大佬包容着看
        改进的地方比如：
        1、TEDS 区分目前用 switch-case 但不如 表格驱动 写法好
        2、域类 TLV 里面的 TL 两个域的值 一般都是固定的，应该用 static 去在定义时候就初始化好值 并 不能再变
        3、填入结构体 有关的 打包、解包 API 里面都应该先 对参数 检测是否合法，打印警告信息 或 限幅

    本实现 对应的 IEEE 1451 标准协议 的 github/gitee 仓库 和 系列介绍文章：
        https://github.com/Staok/IEEE-1451-study
        https://gitee.com/staok/IEEE-1451-study

        https://blog.csdn.net/staokgo/category_11531131.html
        https://www.zhihu.com/column/c_1399448816085524480
        https://staok.gitee.io/categories/%E8%AF%BB%E6%A0%87%E5%87%86/


Log:            日期        发布版本    作者   描述
                2021.8.3    V1.0        xu
                2021.8.12   V2.0        xu
                2022.11     V2.5        瞰百  为实际用起来做一定微调
*************************************************/

#include "IEEE1451_5_lib.h"
#include <string.h>

/* 
流程：
    0、NCAP 发出 WiFi 热点， TIM 们上电开机后分别自动的连上 NCAP
    0.5、TIM 给 NCAP 发送初始化完毕消息（Message_TIM_initiated）
    1、NCAP 自动询问 TIM 的 TEDS（CommonCmd_Query_TEDS），TIM 自动做回应
    2、NCAP 再自动读 TIM 的 TEDS（CommonCmd_Read_TEDS），TIM 自动做回应
    3、以上步骤均无误后，NCAP 根据一个标志位判断自动启动 TIM 还是等待上级进一步操作
        3.1、若是前者，即自动启动 TIM
            3.1.1、NCAP 根据预先设定去设 TIM 的上传数据模式（XdcrIdle_Data_Transmission_mode），TIM 自动做回应
                我这里设计三种模式：
                    OnCommand_custom：（这个暂不实现）
                        NCAP 发送 读 数据 命令 给 TIM 的时候， TIM 空闲时候则响应，开始采集 1s 数据并上传；若TIM 正在工作则继续原来的采集和上传，不予回应
                    Interval_1s：TIM 定时 上传数据，在我这里是 每秒 发送一帧数据，但上位机显示的是连续的
                    BufferHalfFull：缓冲区半满时候 上传一次数据
            3.1.2、NCAP 发送 触发命令（XdcrOperate_Trigger），TIM 按照 Interval_1s 或 BufferHalfFull 模式 上传数据
            到此，以上所有步骤都是 TIM 上电后自动完成的
            3.1.3、NCAP 发送 终止触发命令（XdcrOperate_Abort_Trigger），TIM 停止上传，停止采集
        3.2、若是后者，即等待上级进一步操作，则 NCAP 受上级的具体控制（启、停、设置模式、待机等等）

关于 状态的实现，这里做简化：
    TIM 和 传感器通道的状态一致，是同一个标志位，分为三个模式：上电初始化中（Initializing）、空闲中（Idle） 和 工作中（Operating）

    状态是 （FPGA + 麦克风阵列） 的空闲或者工作（空闲就是 FPGA 不采集，麦克风阵列失能），TIM 上的 i.mx8mm 运行着 linux，是一直工作的

    上电和初始化完毕后，在上面 3 步骤结束时候， TIM 进入空闲状态（Idle）
    接下来：
        TIM 接收到 终止触发命令（XdcrOperate_Abort_Trigger）则进入 空闲状态
        TIM 接收到除了上个命令以外的其他命令都进入 工作状态（Operating）
    
本库使用流程：（NCAP 和 TIM 同用此库，只调用不同 API 即可）
    
    上电 TEDS 初始化，调用如下：一般是 TIM 调用
        TEDS_init();
    
    上电 Message 初始化，调用如下：一般是 NCAP 调用
        Message_init();

    发送一次命令，这里一般是 NCAP 调用的 API：
        比如 NCAP 自动 发送 询问 TIM 的 TEDS 的命令（CommonCmd_Query_TEDS），则例子如下，其他命令同理
            Message_CommonCmd_Query_TEDS_pack_up(TIM_3, TC_12, PHY_TEDS_ACCESS_CODE);
            Message_pack_up_And_send();

        NCAP 解析回复消息 API：填入接收到的回复消息字符串，接收回复消息解析，并讲结果存在 replyMessageReceived 结构体地址里
            void ReplyMessage_decode(struct ReplyMessage_struct* replyMessageReceived,uint8_t* received_rep_mes_load)

    回复消息，这里一般是 TIM 调用的 API：
        TIM 首先主动发送初始化完毕消息：
            Message_TIM_initiated_pack_up();
            Message_pack_up_And_send();

        TIM 处理 接收消息 和 自动回复消息 API：填入接收到的消息字符串，会根据已经实现的消息解码字符串和自动回应
            void ReplyMessage_Server(uint8_t* received_mes_load)
*/

/* 调试 / 测试 / 样例 程序：
    见 1451_tcp_test_server.c
    与 1451_tcp_test_client.c
*/





/* ----------- 以下 IEEE 1451.5 lib 程序 开始 ----------- */

/* 定义发送数据 函数指针，初始化时候应该填入 */
unsigned int (*mes_1451_send)(unsigned char * data, unsigned int len);

/* TIM 自己的固定 IP */ /* 固定 IP 吧，省心，在下面设定 */
uint8_t NCAP_IP[4] = {192,168,120,0};

uint8_t TIM_IP[TIM_MAX][4] = 
{
    {192,168,120,1},{192,168,120,2},{192,168,120,3},{192,168,120,4},{192,168,120,5},{192,168,120,6},
    {192,168,120,7},{192,168,120,8},{192,168,120,9},{192,168,120,10},{192,168,120,11},{192,168,120,12}
};
/*
    自己 TIM 的 IP 地址索引方式：   TIM_IP[ Self_TIM ][0 ~ 3]; 
    目标 TIM 的 IP 地址索引方式：   TIM_IP[ MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] ][0 ~ 3]; 
*/

/* 自己是哪个 TIM，由此可确定自己的 IP 地址，可以随着动态分配而更改 */
enum TIM_enum Self_TIM = TIM_3;

enum TIM_status_enum TIM_status = Initializing;

uint8_t temp_load[200];
uint32_t temp_load_valid_length = 0;

                                    /*************\
*************************************  TEDS 部分  *****************************************************
                                    \*************/

/* 只在 本 .c 中使用的 API，非用户使用 */
uint16_t TEDS_calc_Checksum(uint8_t access_code);
void TEDS_pack_up(uint8_t* dest_loader,uint32_t* length,uint8_t access_code);
void memcpy_with_BitLittle_switch(uint8_t* dest, uint8_t* src, uint32_t size, uint8_t is_switch);
void Big_Little_End_Switch(uint8_t* dest, uint8_t* src, uint32_t size);


/**************************** TEDS 总结构体，用户使用 ****************************/
struct TEDS_struct TEDS;

struct TEDS_attributes_struct M_TEDS_attr = 
{
    .ReadOnly = 0,
    .NotAvail = 0,
    .Invalid = 0,
    .Virtual = 0,
    .TextTEDS = 0,
    .Adaptive = 0,
    .MfgrDefine = 0,
    .Reserved = 1
};
struct TEDS_attributes_struct TC_TEDS_attr = 
{
    .ReadOnly = 0,
    .NotAvail = 0,
    .Invalid = 0,
    .Virtual = 0,
    .TextTEDS = 0,
    .Adaptive = 0,
    .MfgrDefine = 0,
    .Reserved = 1
};
struct TEDS_attributes_struct UTN_TEDS_attr = 
{
    .ReadOnly = 0,
    .NotAvail = 0,
    .Invalid = 0,
    .Virtual = 0,
    .TextTEDS = 0,
    .Adaptive = 0,
    .MfgrDefine = 0,
    .Reserved = 1
};
struct TEDS_attributes_struct PHY_TEDS_attr = 
{
    .ReadOnly = 0,
    .NotAvail = 0,
    .Invalid = 0,
    .Virtual = 0,
    .TextTEDS = 0,
    .Adaptive = 0,
    .MfgrDefine = 0,
    .Reserved = 1
};

struct TEDS_status_struct M_TEDS_status = 
{
    .TooLarge = 0,
    .Reserved_1 = 1,
    .Reserved_2 = 0,
    .Reserved_3 = 1,

    .Open_4 = 1,
    .Open_5 = 1,
    .Open_6 = 0,
    .Open_7 = 1,
};
struct TEDS_status_struct TC_TEDS_status = 
{
    .TooLarge = 0,
    .Reserved_1 = 1,
    .Reserved_2 = 0,
    .Reserved_3 = 1,

    .Open_4 = 1,
    .Open_5 = 1,
    .Open_6 = 0,
    .Open_7 = 1,
};
struct TEDS_status_struct UTN_TEDS_status = 
{
    .TooLarge = 0,
    .Reserved_1 = 1,
    .Reserved_2 = 0,
    .Reserved_3 = 1,

    .Open_4 = 1,
    .Open_5 = 1,
    .Open_6 = 0,
    .Open_7 = 1,
};
struct TEDS_status_struct PHY_TEDS_status = 
{
    .TooLarge = 0,
    .Reserved_1 = 1,
    .Reserved_2 = 0,
    .Reserved_3 = 1,

    .Open_4 = 1,
    .Open_5 = 1,
    .Open_6 = 0,
    .Open_7 = 1,
};

/* 开始装填 M_TEDS 结构体 */
union Meta_TEDS_union M_TEDS_u = 
{
    /* Length 和 Checksum 会在 TEDS_init() 中自动计算装填 */

    .M_TEDS.ID.Type         = 3,    /* TEDS 头的域类号为 3 */
    .M_TEDS.ID.Length       = 4,    /* 总为 4 */
    .M_TEDS.ID.Family       = 5,    /* IEEE 1451.x */
    .M_TEDS.ID.access_code  = M_TEDS_ACCESS_CODE,   /* TEDS 的代码 */
    .M_TEDS.ID.Version      = 1,    /* TEDS 版本，1 为初始版本，往后排 */
    .M_TEDS.ID.Tuple_Length = 1,    /* TLV 中 Length 占的字节数，为 1*/

    /* Globally Unique Identifier，Value 占 10 个字节 */
    .M_TEDS.UUID.Type = 4,
    .M_TEDS.UUID.Length = 10,
    .M_TEDS.UUID.Value = {0x8d,0x4d,0x9d,0xa6,0x52,0x81,0xf7,0x00,0x00,0x00},
    
    /* Operational time-out，整数，单位秒，Value 占 4 个字节 */
    .M_TEDS.OholdOff.Type = 10,
    .M_TEDS.OholdOff.Length = 4,
    .M_TEDS.OholdOff.Value = 5,
    
    /* Slow-access time-out，整数，单位秒，Value 占 4 个字节 */
    .M_TEDS.SHoldOff.Type = 11,
    .M_TEDS.SHoldOff.Length = 4,
    .M_TEDS.SHoldOff.Value = 10,
    
    /* Self-Test Time，整数，单位秒，Value 占 4 个字节 */
    .M_TEDS.TestTime.Type = 12,
    .M_TEDS.TestTime.Length = 4,
    .M_TEDS.TestTime.Value = 10,
    
    /* Number of implemented TransducerChannels，整数，Value 占 2 个字节 */
    .M_TEDS.MaxChan.Type = 13,
    .M_TEDS.MaxChan.Length = 2,
    .M_TEDS.MaxChan.Value = 1,

    /* ... */
};
/* 开始装填 TC_TEDS 结构体 */
union TransducerChannel_TEDS_union TC_TEDS_u = 
{
    /* Length 和 Checksum 会在 TEDS_init() 中自动计算装填 */

    .TC_TEDS.ID.Type         = 3,    /* TEDS 头的域类号为 3 */
    .TC_TEDS.ID.Length       = 4,    /* 总为 4 */
    .TC_TEDS.ID.Family       = 5,    /* IEEE 1451.x */
    .TC_TEDS.ID.access_code  = TC_TEDS_ACCESS_CODE,   /* TEDS 的代码 */
    .TC_TEDS.ID.Version      = 1,    /* TEDS 版本，1 为初始版本，往后排 */
    .TC_TEDS.ID.Tuple_Length = 1,    /* TLV 中 Length 占的字节数，为 1*/

    /* 10  CalKey  Calibration key  uint8_t  1 */
    .TC_TEDS.CalKey.Type = 10,
    .TC_TEDS.CalKey.Length = 1,
    .TC_TEDS.CalKey.Value = (uint8_t)CAL_NONE,    /* 选择校准方式 */

    /* 11  ChanType  TransducerChannel type key  uint8_t  1 */
    .TC_TEDS.ChanType.Type = 11,
    .TC_TEDS.ChanType.Length = 1,
    .TC_TEDS.ChanType.Value = (uint8_t)Sensor,    /* 选择传感器、执行器或其他 */
    
    /* 12  PhyUnits  Physical Units  UNITS  11  */
    /* 传感器通道 测量的物理量表示，以 1451.0 标准 Table K.5—Pressure 为例 */
    .TC_TEDS.PhyUnits.Type = 12,
    .TC_TEDS.PhyUnits.Length = 11,
    .TC_TEDS.PhyUnits.Value.interpretation = PUI_SI_UNITS,
    .TC_TEDS.PhyUnits.Value.radians = 128,.TC_TEDS.PhyUnits.Value.steradians = 128,.TC_TEDS.PhyUnits.Value.meters = 126,
    .TC_TEDS.PhyUnits.Value.kilograms = 130,.TC_TEDS.PhyUnits.Value.seconds = 124,.TC_TEDS.PhyUnits.Value.amperes = 128,
    .TC_TEDS.PhyUnits.Value.kelvins = 130,.TC_TEDS.PhyUnits.Value.moles = 124,.TC_TEDS.PhyUnits.Value.candelas = 128,.TC_TEDS.PhyUnits.Value.Units_Extension_TEDS_Access_Code = (uint8_t)TC_TEDS_ACCESS_CODE,

    /* 域类 50 ~ 60略 */

    /* 13  LowLimit  Design operational lower range limit  Float32  4 */
    /* 最小，最大值和错误，float 占 4 字节 */
    .TC_TEDS.LowLimit.Type = 13, .TC_TEDS.LowLimit.Length = 4,
    .TC_TEDS.LowLimit.Value = -46,
    
    /* 14  HiLimit  Design operational upper range limit  Float32  4 */
    .TC_TEDS.HiLimit.Type = 14, .TC_TEDS.HiLimit.Length = 4,
    .TC_TEDS.HiLimit.Value = 102.5,
    
    /* 15  OError  Worst-case uncertainty  Float32  4 */
    .TC_TEDS.OError.Type = 15, .TC_TEDS.OError.Length = 4,
    .TC_TEDS.OError.Value = 120,

    /* 16  SelfTest  Self-test key  uint8_t  1 */
    .TC_TEDS.SelfTest.Type = 16, .TC_TEDS.SelfTest.Length = 1,
    .TC_TEDS.SelfTest.Value = 0, /* 0:No self-test function needed or provided ,1:Self-test function provided */
    
    /* 17  MRange  Multi-range capability  uint8_t  1 */
    .TC_TEDS.MRange.Type = 17, .TC_TEDS.MRange.Length = 1,
    .TC_TEDS.MRange.Value = 0, /* 值为真表示具有多量程，否则不具有 */
    
    /* 太多了 T_T 后面略 */
    
    // .Sample = , 略
    // .DataSet = , 略
    
    // .UpdateT = 0.1,
    // .WSetupT = ,
    // .RSetupT = ,
    // .SPeriod = ,
    // .WarmUpT = ,
    // .RDelayT = ,
    // .TestTime = ,
    // .TimeSrc = ,
    // .InPropDl = ,
    // .OutPropD = ,
    // .TSError = ,

    // // .Sampling = , 略
    
    // .DataXmit = ,
    // .Buffered = ,
    // .EndOfSet = ,
    // .EdgeRpt = ,
    // .ActHalt = ,
    // .Directon = ,
    // .DAngles[0] = ,DAngles[1] = ,
    // .ESOption = ,
};
/* 开始装填 UTN_TEDS 结构体 */
union User_Transducer_Name_TEDS_union UTN_TEDS_u = 
{
    /* Length 和 Checksum 会在 TEDS_init() 中自动计算装填 */

    .UTN_TEDS.ID.Type         = 3,    /* TEDS 头的域类号为 3 */
    .UTN_TEDS.ID.Length       = 4,    /* 总为 4 */
    .UTN_TEDS.ID.Family       = 5,    /* IEEE 1451.x */
    .UTN_TEDS.ID.access_code  = UTN_TEDS_ACCESS_CODE,   /* TEDS 的代码 */
    .UTN_TEDS.ID.Version      = 1,    /* TEDS 版本，1 为初始版本，往后排 */
    .UTN_TEDS.ID.Tuple_Length = 1,    /* TLV 中 Length 占的字节数，为 1*/

    .UTN_TEDS.Format.Type = 4, .UTN_TEDS.Format.Length = 1,
    .UTN_TEDS.Format.Value = 1,

    .UTN_TEDS.TCName.Type = 5, .UTN_TEDS.TCName.Length = 25,
    .UTN_TEDS.TCName.Value = "microphone array / 24\0",
};
/* 开始装填 PHY_TEDS 结构体 */
union PHY_TEDS_union PHY_TEDS_u = 
{
    /* Length 和 Checksum 会在 TEDS_init() 中自动计算装填 */
    
    .PHY_TEDS.ID.Type         = 3,    /* TEDS 头的域类号为 3 */
    .PHY_TEDS.ID.Length       = 4,    /* 总为 4 */
    .PHY_TEDS.ID.Family       = 5,    /* IEEE 1451.x */
    .PHY_TEDS.ID.access_code  = PHY_TEDS_ACCESS_CODE,   /* TEDS 的代码 */
    .PHY_TEDS.ID.Version      = 1,    /* TEDS 版本，1 为初始版本，往后排 */
    .PHY_TEDS.ID.Tuple_Length = 1,    /* TLV 中 Length 占的字节数，为 1*/

    /* 10  Radio  Radio Type  UInt8  1  See Table 4 */
    .PHY_TEDS.RadioType.Type = 10, .PHY_TEDS.RadioType.Length = 1,
    .PHY_TEDS.RadioType.Value = IEEE_802_11, 
    
    /* 11  MaxBPS  Max data throughput  UInt32  4  Bits per second */ /* 8388608 bps = 1 MBps */
    .PHY_TEDS.MaxBPS.Type = 11, .PHY_TEDS.MaxBPS.Length = 4,
    .PHY_TEDS.MaxBPS.Value = 8388608 * 3, 

    /* 12  MaxCDev  Max Connected Devices UInt16  2  Maximum number of devices that may be simultaneously operational with this device  */
    .PHY_TEDS.MaxCDev.Type = 12, .PHY_TEDS.MaxCDev.Length = 2,
    .PHY_TEDS.MaxCDev.Value = 1, 
    
    /* 13  MaxRDev  Max Registered Devices UInt16  2  Maximum number of devices that may be simultaneously registered with this device */
    .PHY_TEDS.MaxRDev.Type = 13, .PHY_TEDS.MaxRDev.Length = 2,
    .PHY_TEDS.MaxRDev.Value = 1, 
    
    /* 14  Encrypt  Encryption  UInt16  2  Encryption type and key length  */
    .PHY_TEDS.Encrypt.Type = 14, .PHY_TEDS.Encrypt.Length = 2,
    .PHY_TEDS.Encrypt.Value[0] = No_encryption, .PHY_TEDS.Encrypt.Value[1] = 0xaa, 
    
    /* 15  Authent  Authentication  Boolean  1  TRUE = Authentication supported FALSE = not supported */
    .PHY_TEDS.Authent.Type = 15, .PHY_TEDS.Authent.Length = 1,
    .PHY_TEDS.Authent.Value = 0, 
    
    /* 16  MinKeyL  Min Key Length  UInt16  2  Minimum key length for security functions */
    .PHY_TEDS.MinKeyL.Type = 16, .PHY_TEDS.MinKeyL.Length = 2,
    .PHY_TEDS.MinKeyL.Value = 0, 
    
    /* 17  MaxKeyL  Max Key Length  UInt16  2  Maximum key length for security functions */
    .PHY_TEDS.MaxKeyL.Type = 17, .PHY_TEDS.MaxKeyL.Length = 2,
    .PHY_TEDS.MaxKeyL.Value = 32, 
    
    /* 18   MaxSDU  Max SDU Size  UInt16  2  Maximum payload size for transfer between devices */
    .PHY_TEDS.MaxSDU.Type = 18, .PHY_TEDS.MaxSDU.Length = 2,
    .PHY_TEDS.MaxSDU.Value = 1024 * 10, 
    
    /* 19  MinALat  Min Access Latency  UInt32  4  Minimum period for initiating a first transmission to an unconnected device, in nanoseconds */
    .PHY_TEDS.MinALat.Type = 19, .PHY_TEDS.MinALat.Length = 4,
    .PHY_TEDS.MinALat.Value = 1000000000, 
    
    /* 20  MinTLat  Min Transmit Latency  UInt32  4  The period for transmitting an N-octet payload, where N is the minimum payload size, in nanoseconds */
    .PHY_TEDS.MinTLat.Type = 20, .PHY_TEDS.MinTLat.Length = 4,
    .PHY_TEDS.MinTLat.Value = 1000000000, 
    
    /* 21  MaxXact  Max Simultaneous Transactions UInt8  1  Maximum number of queued, uncompleted commands */
    .PHY_TEDS.MaxXact.Type = 21, .PHY_TEDS.MaxXact.Length = 1,
    .PHY_TEDS.MaxXact.Value = 24, 
    
    /* 22  Battery   Device is battery powered UInt 8  1  A nonzero value indicates battery power */
    .PHY_TEDS.Battery.Type = 22, .PHY_TEDS.Battery.Length = 1,
    .PHY_TEDS.Battery.Value = 123, 
    
    /* 23  RadioVer  Radio Version #  UInt16  2  A zero value indicates unknown */
    .PHY_TEDS.RadioVer.Type = 23, .PHY_TEDS.RadioVer.Length = 2,
    .PHY_TEDS.RadioVer.Value = 0x0102, 
    
    /* 24  MaxRetry  Maximum Retries before Disconnect Uint16  2  A zero value indicates never disconnect */
    .PHY_TEDS.MaxRetry.Type = 24, .PHY_TEDS.MaxRetry.Length = 2,
    .PHY_TEDS.MaxRetry.Value = 5, 
};

/**************************** TEDS init，用户使用 ****************************/
void TEDS_init(void)
{
    
    M_TEDS_u.M_TEDS.Length = sizeof(struct Meta_TEDS_struct) - 4;       /* 计算 TEDS 的数据区长度 */ /* 42，宇宙的终极答案，yyds */
                        /* 这个长度 只包括 TEDS 的 DATA BLOCK 和 CHECKSUM，不包括 占 4 byte 的 length */
    TEDS.M_TEDS_load_Length = M_TEDS_u.M_TEDS.Length + 4;               /* 计算 TEDS 总长度 */
    TEDS.M_TEDS_u = &M_TEDS_u;                                          /* TEDS 的 M_TEDS_u 指针填充，后面 TEDS_calc_Checksum() 要用 */
    M_TEDS_u.M_TEDS.Checksum = TEDS_calc_Checksum(M_TEDS_ACCESS_CODE);  /* 计算 Checksum  */

    TC_TEDS_u.TC_TEDS.Length = sizeof(struct TransducerChannel_TEDS_struct) - 4;
    TEDS.TC_TEDS_load_Length = TC_TEDS_u.TC_TEDS.Length + 4;
    TEDS.TC_TEDS_u = &TC_TEDS_u;
    TC_TEDS_u.TC_TEDS.Checksum = TEDS_calc_Checksum(TC_TEDS_ACCESS_CODE);

    UTN_TEDS_u.UTN_TEDS.Length = sizeof(struct User_Transducer_Name_TEDS_struct) - 4;
    TEDS.UTN_TEDS_load_Length = UTN_TEDS_u.UTN_TEDS.Length + 4;
    TEDS.UTN_TEDS_u = &UTN_TEDS_u;
    UTN_TEDS_u.UTN_TEDS.Checksum = TEDS_calc_Checksum(UTN_TEDS_ACCESS_CODE);

    PHY_TEDS_u.PHY_TEDS.Length = sizeof(struct PHY_TEDS_struct) - 4;
    TEDS.PHY_TEDS_load_Length = PHY_TEDS_u.PHY_TEDS.Length + 4;
    TEDS.PHY_TEDS_u = &PHY_TEDS_u;
    PHY_TEDS_u.PHY_TEDS.Checksum = TEDS_calc_Checksum(PHY_TEDS_ACCESS_CODE);


    /* 开始装填 每个 TEDS 的全名字符串 */
    TEDS.M_TEDS_str     = "Meta-TEDS";
    TEDS.TC_TEDS_str    = "TransducerChannel TEDS";
    TEDS.UTN_TEDS_str   = "User’s transducer name TEDS";
    TEDS.PHY_TEDS_str   = "PHY TEDS";

    /* 指定对应 TEDS 的 属性和状态 */
    TEDS.M_TEDS_attr = &M_TEDS_attr;
    TEDS.TC_TEDS_attr = &TC_TEDS_attr;
    TEDS.UTN_TEDS_attr = &UTN_TEDS_attr;
    TEDS.PHY_TEDS_attr = &PHY_TEDS_attr;

    TEDS.M_TEDS_status = &M_TEDS_status;
    TEDS.TC_TEDS_status = &TC_TEDS_status;
    TEDS.UTN_TEDS_status = &UTN_TEDS_status;
    TEDS.PHY_TEDS_status = &PHY_TEDS_status;
}

/* TEDS 打包函数
    第一个参数给一个存放 TEDS 字节数据的空间，至少 200 个字节空间；
    第二个参数指定 有效数据长度（字节数）；
    第三个参数选择哪一个 TEDS。
*/
/* 说明，可以不必用这个 API，这是又拷贝一遍，直接 掉用 ReplyMessage_Server() 就好，
    里面的 ReplyMessage_CommonCmd_Read_TEDS_segment_pack_up() 就已经将 TEDS 以 ReplyMessage 形式 打包了，
    或者自己 直接按照 TEDS.M_TEDS_load_Length 长度 发送 TEDS.M_TEDS_u->TEDS_load 这个大数组 即可~ */
void TEDS_pack_up(uint8_t* dest_loader,uint32_t* length,uint8_t access_code)
{
    uint8_t* load_ptr;
    uint32_t whole_length = 0;
    
    switch (access_code)
    {
        case M_TEDS_ACCESS_CODE:
            
            whole_length = TEDS.M_TEDS_load_Length;
            
            load_ptr = TEDS.M_TEDS_u->TEDS_load;
            
            break;

        case TC_TEDS_ACCESS_CODE:
            
            whole_length = TEDS.TC_TEDS_load_Length;
            
            load_ptr = TEDS.TC_TEDS_u->TEDS_load;

            break;

        case UTN_TEDS_ACCESS_CODE:
            
            whole_length = TEDS.UTN_TEDS_load_Length;
            
            load_ptr = TEDS.UTN_TEDS_u->TEDS_load;

            break;

        case PHY_TEDS_ACCESS_CODE:
            
            whole_length = TEDS.PHY_TEDS_load_Length;
            
            load_ptr = TEDS.PHY_TEDS_u->TEDS_load;
            
            break;
        
        default:
            break;
    }

    /* 长度限幅 */
    whole_length = whole_length > MAX_TEDS_LOAD_SIZE ? \
        MAX_TEDS_LOAD_SIZE : whole_length;

    *length = whole_length;
    memcpy(dest_loader, load_ptr, whole_length);
    
}

/* 
    计算 TEDS 校验，
    从 TED Length（最开头） 到 DATA BLOCK 的最后一个字节（本域类的上一个字节） 的加和，
    再用 0xFFFF 减去该加和值。 
*/
uint16_t TEDS_calc_Checksum(uint8_t access_code)
{
    uint32_t i = 0;
    uint8_t* load_ptr;
    uint32_t length_except_checksum = 0;
    uint16_t Checksum = 0;

    switch (access_code)
    {
        case M_TEDS_ACCESS_CODE:
            
            length_except_checksum = TEDS.M_TEDS_load_Length - 2;
            
            load_ptr = TEDS.M_TEDS_u->TEDS_load;
            
            break;

        case TC_TEDS_ACCESS_CODE:
            
            length_except_checksum = TEDS.TC_TEDS_load_Length - 2;
            
            load_ptr = TEDS.TC_TEDS_u->TEDS_load;

            break;

        case UTN_TEDS_ACCESS_CODE:
            
            length_except_checksum = TEDS.UTN_TEDS_load_Length - 2;
            
            load_ptr = TEDS.UTN_TEDS_u->TEDS_load;

            break;

        case PHY_TEDS_ACCESS_CODE:
            
            length_except_checksum = TEDS.PHY_TEDS_load_Length - 2;
            
            load_ptr = TEDS.PHY_TEDS_u->TEDS_load;
            
            break;
        
        default:
            break;
    }

    for(i = 0;i < length_except_checksum;i++)
    {
        Checksum += *load_ptr++;
    }
    
    Checksum = 0xFFFF - Checksum;

    return Checksum;
}

void TEDS_decode(uint8_t* TEDS_load);   /* TODO ，暂不实现 */

/*
    带大小端区分的 memcpy，若 is_switch 为 真 则 src 大端转小端或者小端转大端 幅值给 dest，连续 size 的内存空间
    用例：
    memcpy_with_BitLittle_switch( (uint8_t*)(&(TEDS.M_TEDS.Length)), (uint8_t*)(&M_TEDS_Value.Length), sizeof(TEDS.M_TEDS.Length), LITTLE_BIG_END);  数据拷贝，根据大小端选择是否要转换
    memcpy_with_BitLittle_switch( (uint8_t*)(&(TEDS.UTN_TEDS.Length)), (uint8_t*)(&UTN_TEDS_Value.Length), sizeof(TEDS.UTN_TEDS.Length), LITTLE_BIG_END);
*/
void memcpy_with_BitLittle_switch(uint8_t* dest, uint8_t* src, uint32_t size, uint8_t is_switch)
{
    if(is_switch)
    {
        Big_Little_End_Switch(dest, src, size);
    }else
    {
        memcpy(dest, src, size);
    }
}

/* 大端 小端 转换，src 如果是大端，出来 dest 为小端，否则反之，
    总之就是 src 数组 倒过来给 dest */
void Big_Little_End_Switch(uint8_t* dest, uint8_t* src, uint32_t size)
{
    uint32_t i = 0;
    for(i = 0;i < size;i++)
    {
        *dest++ = *(src + size - 1 - i);
    }
}


                                    /*************\
************************************* Message 部分 *****************************************************
                                    \*************/

struct MES_struct MES;

/* 给 Message_u 和 ReplyMessage_u 填充默认值  */
union Message_union Message_u = 
{
    .Message.Dest_TIM_and_TC_Num[TIM_enum] = TIM_3,
    .Message.Dest_TIM_and_TC_Num[TC_enum] = TC_12,

    .Message.Command_class = CommonCmd,
    .Message.Command_function = Query_TEDS,
    .Message.dependent_Length = 1,

    .Message.dependent_load[0] = PHY_TEDS_ACCESS_CODE,
};

union ReplyMessage_union ReplyMessage_u = 
{
    .ReplyMessage.Flag = 0,
    .ReplyMessage.dependent_Length = 1,
    .ReplyMessage.dependent_load[0] = 0xAA,
};

/**************************** Message init，用户使用 ****************************/
void Message_init(void)
{
    MES.Message_u = &Message_u;
    MES.ReplyMessage_u = &ReplyMessage_u;

    /* MES 的 Message_load_Length 和 ReplyMessage_load_Length 根据不同的情景在打包发送数据的时候再填 */
}

/**************************** 根据不同命令 填充 Message 结构体 的 API，用户使用 ****************************/
/* 打包好的消息数据在 MES.Message_u->Message_load 里面，有效数据长度为 MES.Message_load_Length */

/* 发送 Message API，CommonCmd 的 Query_TEDS 命令*/
void Message_CommonCmd_Query_TEDS_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint8_t which_TEDS)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = CommonCmd;
    MES.Message_u->Message.Command_function = Query_TEDS;
    MES.Message_u->Message.dependent_Length = 1;    /* 本命令的附带参数固定长度 */
    
    MES.Message_u->Message.dependent_load[0] = which_TEDS;

    /* 固定信息长度 + 附带参数长度 */
    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_CommonCmd_Read_TEDS_segment_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint8_t which_TEDS, uint32_t TEDSOffset)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = CommonCmd;
    MES.Message_u->Message.Command_function = Read_TEDS_segment;
    MES.Message_u->Message.dependent_Length = 5;
    
    MES.Message_u->Message.dependent_load[0] = which_TEDS;
    memcpy(&(MES.Message_u->Message.dependent_load[1]), &TEDSOffset, sizeof(TEDSOffset));

    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_XdcrIdle_Set_Data_Transmission_mode_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC,enum Data_Transmission_mode_enum mode)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = XdcrIdle;
    MES.Message_u->Message.Command_function = Data_Transmission_mode;
    MES.Message_u->Message.dependent_Length = 1;
    
    MES.Message_u->Message.dependent_load[0] = mode;

    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_XdcrOperate_Read_TC_data_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC, uint32_t Offset)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = XdcrOperate;
    MES.Message_u->Message.Command_function = Read_TransducerChannel_data_set_segment;
    MES.Message_u->Message.dependent_Length = 4;
    
    memcpy(&(MES.Message_u->Message.dependent_load[0]), &Offset, sizeof(Offset));

    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_XdcrOperate_Trigger_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = XdcrOperate;
    MES.Message_u->Message.Command_function = Trigger_command;
    MES.Message_u->Message.dependent_Length = 0;
    
    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_XdcrOperate_Abort_Trigger_pack_up(uint8_t Dest_TIM, uint8_t Dest_TC)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Dest_TIM;
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = Dest_TC;
    MES.Message_u->Message.Command_class = XdcrOperate;
    MES.Message_u->Message.Command_function = Abort_Trigger;
    MES.Message_u->Message.dependent_Length = 0;
    
    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

void Message_TIM_initiated_pack_up(void)
{
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] = Self_TIM; /* 我是谁 */
    MES.Message_u->Message.Dest_TIM_and_TC_Num[TC_enum] = TC_MAX; /* 表示 ALL */
    MES.Message_u->Message.Command_class = XdcrIdle;
    MES.Message_u->Message.Command_function = TIM_ALL_TC_initiated;
    MES.Message_u->Message.dependent_Length = 0;
    
    MES.Message_load_Length = 6 + MES.Message_u->Message.dependent_Length;
}

/* 剩余其他 Message 在这里 挨个实现 ... 相当繁琐了 */

/**************************** 消息的 发送，用户使用 ****************************/
/* 发送 Message，一般是 NCAP 用 */
void Message_pack_up_And_send(void)
{
    /* 进行发送 */
    /*
        目标 TIM 的 IP 地址：   TIM_IP[ MES.Message_u->Message.Dest_TIM_and_TC_Num[TIM_enum] ][0 ~ 3]; 
        调用 Message_xxx_pack_up() 即 填充 MES 结构体：
            待发送数组：        MES.Message_u->Message_load
            有效数据长度为：    MES.Message_load_Length
        然后在 这里 发送数据。
    */

   mes_1451_send(MES.Message_u->Message_load, MES.Message_load_Length);

}

/* 临时用到 */
struct Message_struct Message_temp = { 0 };
struct ReplyMessage_struct ReplyMessage_temp = { 0 };

/**************************** 解析接收到的 Message 的 API ****************************/
/* 接收 信息 字节数组 received_mes_load 并 解析 然后 将 结果放在 messageReceived 地址的结构体里 */
void Message_decode(struct Message_struct* messageReceived,uint8_t* received_mes_load)
{
    uint16_t i = 0;

    memset(messageReceived, 0, sizeof(struct Message_struct));
    
    messageReceived->Dest_TIM_and_TC_Num[TIM_enum] = received_mes_load[0];
    messageReceived->Dest_TIM_and_TC_Num[TC_enum] = received_mes_load[1];

    messageReceived->Command_class = received_mes_load[2];
    messageReceived->Command_function = received_mes_load[3];

    /* 这一步填充 dependent_Length */
    /* 根据 NEED_SWITCH_LITTLE_BIG_END 来判断本平台与 发送数据的 平台的 大小端是否一致，若一致则进行大小端转换，否则不转换 */
    memcpy_with_BitLittle_switch((uint8_t*)(&(messageReceived->dependent_Length)),   \
            (uint8_t*)(&(received_mes_load[4])), sizeof((messageReceived->dependent_Length)), NEED_SWITCH_LITTLE_BIG_END);
    
    /* 长度限幅 */
    messageReceived->dependent_Length = messageReceived->dependent_Length > MAX_Message_dependent_SIZE ? \
        messageReceived->dependent_Length = MAX_Message_dependent_SIZE : messageReceived->dependent_Length;

    for(i = 0;i < messageReceived->dependent_Length;i++)
    {
        messageReceived->dependent_load[i] = received_mes_load[6 + i];
    }
}

/**************************** 根据相应 Message 填充 ReplyMessage 结构体 并打包数据的 API， 的 API ****************************/
/* 打包好后的回复消息数据在 MES.ReplyMessage_u->ReplyMessage_load 里面，有效数据长度为 MES.ReplyMessage_load_Length */

void ReplyMessage_CommonCmd_Query_TEDS_pack_up(uint8_t which_TEDS)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 12;
    
    switch (which_TEDS)
    {
        case M_TEDS_ACCESS_CODE:
            
            MES.ReplyMessage_u->ReplyMessage.dependent_load[0] = *((uint8_t*)(TEDS.M_TEDS_attr));
            MES.ReplyMessage_u->ReplyMessage.dependent_load[1] = *((uint8_t*)(TEDS.M_TEDS_status));

            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[2]),&TEDS.M_TEDS_load_Length,sizeof(TEDS.M_TEDS_load_Length));
            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[6]),&TEDS.M_TEDS_u->M_TEDS.Checksum,sizeof(TEDS.M_TEDS_u->M_TEDS.Checksum));

            break;

        case TC_TEDS_ACCESS_CODE:

            MES.ReplyMessage_u->ReplyMessage.dependent_load[0] = *((uint8_t*)(TEDS.TC_TEDS_attr));
            MES.ReplyMessage_u->ReplyMessage.dependent_load[1] = *((uint8_t*)(TEDS.TC_TEDS_status));

            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[2]),&TEDS.TC_TEDS_load_Length,sizeof(TEDS.TC_TEDS_load_Length));
            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[6]),&TEDS.TC_TEDS_u->TC_TEDS.Checksum,sizeof(TEDS.TC_TEDS_u->TC_TEDS.Checksum));
            
            break;
            
        case UTN_TEDS_ACCESS_CODE:

            MES.ReplyMessage_u->ReplyMessage.dependent_load[0] = *((uint8_t*)(TEDS.UTN_TEDS_attr));
            MES.ReplyMessage_u->ReplyMessage.dependent_load[1] = *((uint8_t*)(TEDS.UTN_TEDS_status));

            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[2]),&TEDS.UTN_TEDS_load_Length,sizeof(TEDS.UTN_TEDS_load_Length));
            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[6]),&TEDS.UTN_TEDS_u->UTN_TEDS.Checksum,sizeof(TEDS.UTN_TEDS_u->UTN_TEDS.Checksum));
            
            break;

        case PHY_TEDS_ACCESS_CODE:

            MES.ReplyMessage_u->ReplyMessage.dependent_load[0] = *((uint8_t*)(TEDS.PHY_TEDS_attr));
            MES.ReplyMessage_u->ReplyMessage.dependent_load[1] = *((uint8_t*)(TEDS.PHY_TEDS_status));

            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[2]),&TEDS.PHY_TEDS_load_Length,sizeof(TEDS.PHY_TEDS_load_Length));
            memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[6]),&TEDS.PHY_TEDS_u->PHY_TEDS.Checksum,sizeof(TEDS.PHY_TEDS_u->PHY_TEDS.Checksum));

            break;
        default:
            break;
    }

    /* 最后四个字节为 max_TEDS_size */
    temp_load_valid_length = (uint32_t)MAX_TEDS_LOAD_SIZE;
    memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[8]),&temp_load_valid_length, 4);

    /* 回复消息附带参数的长度 + Flag、dependent_Length 的变量长度 */
    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

void ReplyMessage_CommonCmd_Read_TEDS_segment_pack_up(uint8_t which_TEDS, uint32_t TEDSOffset)
{
    /* 经过下面的函数，一个 完整 TEDS 数据存在 temp_load 中，TEDS 有效长度为 temp_load_valid_length */
    TEDS_pack_up(temp_load, &temp_load_valid_length, which_TEDS);

    /* 填充 回复命令 结构体 */
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = (uint16_t)temp_load_valid_length + 5; /* TEDS 长度 +  which_TEDS 和 TEDSOffset */
   
    MES.ReplyMessage_u->ReplyMessage.dependent_load[0] = which_TEDS;
    memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[1]), &TEDSOffset, sizeof(TEDSOffset));
    /* 拷贝 TEDS 数据 到 回复命令的 附带参数中 */
    memcpy(&(MES.ReplyMessage_u->ReplyMessage.dependent_load[5]), &temp_load, temp_load_valid_length);

    /* 回复消息附带参数的长度 + Flag、dependent_Length 的变量长度 */
    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

void ReplyMessage_XdcrIdle_Data_Transmission_mode_pack_up(void)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 0;

    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

/* 使用静态内存，而不是动态申请，为 1s 的数据开静态内存太大，所以这里暂不实现 */

void ReplyMessage_XdcrOperate_Read_TC_data_pack_up(void)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 0;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 0;

    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

void ReplyMessage_XdcrOperate_Trigger_pack_up(void)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 0;

    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

void ReplyMessage_XdcrOperate_Abort_Trigger_pack_up(void)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 0;

    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

void ReplyMessage_TIM_initiated_pack_up(void)
{
    MES.ReplyMessage_u->ReplyMessage.Flag = 1;
    MES.ReplyMessage_u->ReplyMessage.dependent_Length = 0;

    MES.ReplyMessage_load_Length = MES.ReplyMessage_u->ReplyMessage.dependent_Length + 3;
}

/**************************** 回复消息的 发送 ****************************/
    /* 发送 ReplyMessage，一般是 TIM 用 */
uint8_t ReplyMessage_send(uint8_t class, uint8_t command)
{
    /* 这里我自己再定义，回复消息的 dependent 的尾部再添加两个字节，
        标识回复消息所对应命令名校的 class 和 command （但是 MES.ReplyMessage_u->ReplyMessage.dependent_Length 就不再动了）*/
    MES.ReplyMessage_u->ReplyMessage.dependent_load[MES.ReplyMessage_u->ReplyMessage.dependent_Length] = class;
    MES.ReplyMessage_u->ReplyMessage.dependent_load[MES.ReplyMessage_u->ReplyMessage.dependent_Length + 1] = command;

    /* 进行发送 */
    /*
        目标 NCAP 的 IP 地址：  NCAP_IP[0 ~ 3]
        调用 ReplyMessage_xxx_pack_up() 即填充 MES 结构体
            待发送数组：        MES.ReplyMessage_u->ReplyMessage_load
            有效数据长度为：    MES.ReplyMessage_load_Length
        然后在这里 发送数据。
    */

    mes_1451_send(MES.ReplyMessage_u->ReplyMessage_load, MES.ReplyMessage_load_Length);

}

/**************************** 解析接收到的 回复消息 的 API ****************************/
/* 填入接收到的回复消息字符串，接收回复消息解析，并讲结果存在 replyMessageReceived 结构体地址里 */
void ReplyMessage_decode(struct ReplyMessage_struct* replyMessageReceived,uint8_t* received_rep_mes_load)
{
    uint16_t i = 0;

    memset(replyMessageReceived, 0, sizeof(struct ReplyMessage_struct));

    replyMessageReceived->Flag = received_rep_mes_load[0];
    
    /* 根据 NEED_SWITCH_LITTLE_BIG_END 来判断本平台与 发送数据的 平台的 大小端是否一致，若一致则进行大小端转换，否则不转换 */
    memcpy_with_BitLittle_switch((uint8_t*)(&(replyMessageReceived->dependent_Length)),   \
            (uint8_t*)(&(received_rep_mes_load[1])), sizeof((replyMessageReceived->dependent_Length)), NEED_SWITCH_LITTLE_BIG_END);
    
    /* 长度限幅 */
    replyMessageReceived->dependent_Length = replyMessageReceived->dependent_Length > MAX_Message_dependent_SIZE ? \
        replyMessageReceived->dependent_Length = MAX_Message_dependent_SIZE : replyMessageReceived->dependent_Length;

    for(i = 0;i < replyMessageReceived->dependent_Length;i++)
    {
        replyMessageReceived->dependent_load[i] = received_rep_mes_load[3 + i];
    }
}

/**************************** ReplyMessage 服务程序，自动解析 Message 并回复，用户使用  ****************************/
/* 填入接收到的消息字符串，会根据已经实现的消息解码字符串和自动回应 */
void ReplyMessage_Server(uint8_t* received_mes_load)
{
    Message_decode(&Message_temp,received_mes_load);

    switch (Message_temp.Command_class)
    {
        case CommonCmd:
        {
            switch (Message_temp.Command_function)
            {
                case Query_TEDS:
ReplyMessage_CommonCmd_Query_TEDS_pack_up(Message_temp.dependent_load[0]);
                    break;
                case Read_TEDS_segment:
ReplyMessage_CommonCmd_Read_TEDS_segment_pack_up(Message_temp.dependent_load[0], *((uint32_t*)(&Message_temp.dependent_load[1])));
                    break;
                default:
                    break;
            }
            break;
        }
        case XdcrIdle:
        {
            switch (Message_temp.Command_function)
            {
                case Data_Transmission_mode:
ReplyMessage_XdcrIdle_Data_Transmission_mode_pack_up();
                    break;
                case TIM_ALL_TC_initiated:
ReplyMessage_TIM_initiated_pack_up();
                    break;
                
                default:
                    break;
            }
            break;
        }
        case XdcrOperate:
        {
            switch (Message_temp.Command_function)
            {
                case Read_TransducerChannel_data_set_segment:
ReplyMessage_XdcrOperate_Read_TC_data_pack_up();
                    break;

                case Trigger_command:
ReplyMessage_XdcrOperate_Trigger_pack_up();
                    break;
                
                case Abort_Trigger:
ReplyMessage_XdcrOperate_Abort_Trigger_pack_up();
                    break;

                default:
                    break;
            }
            break;
        }
        case XdcrEither:
        {
            break;
        }
        case TIMsleep:
        {
            break;
        }
        case TIMActive:
        {
            break;
        }
        case AnyState:
        {
            break;
        }
        default:
            break;
    }

    ReplyMessage_send(Message_temp.Command_class, Message_temp.Command_function);
}


