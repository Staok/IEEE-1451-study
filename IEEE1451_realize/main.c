#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "IEEE1451_5_lib\IEEE1451_5_lib.h"


int main()
{
    char sys[2][10] = {"start!","end!"},overstate;
    srand((int)time(0));
    printf("\n%s\n",sys[0]);

    TEDS_init();
    Message_init();

    /* TEDS 外设初始化 */

    /* TEDS 初始化完毕 */
    TIM_status = Idle;
    Message_TIM_initiated_pack_up();
    /* 加上 message 标识用的帧头，并将 temp_load 发送出去，有效长度 temp_load_valid_length */

    Message_CommonCmd_Query_TEDS_pack_up(TIM_3, TC_12, PHY_TEDS_ACCESS_CODE);
    Message_pack_up_And_send();

    ReplyMessage_Server(MES.Message_u->Message_load);

    ReplyMessage_decode(&ReplyMessage_temp,MES.ReplyMessage_u->ReplyMessage_load);
    

	printf("\n%s\n",sys[1]);
    printf("Press Enter to exit");
    scanf("%c",&overstate);
    return 0;
}
