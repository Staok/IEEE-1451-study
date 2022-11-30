#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "socket\socket.h"
#include "IEEE1451_5_lib\IEEE1451_5_lib.h"

/* 编译命令：这里是 win 下 使用 MinGW 编译
    gcc 1451_tcp_test_client.c .//socket//socket.c .//IEEE1451_5_lib//IEEE1451_5_lib.c -I .//socket -I .//IEEE1451_5_lib -lwsock32 -o 1451_tcp_test_client.exe
*/

/* 我是 TIM 程序 */

/* IEEE 1451 Message 数据 发送 接口 API */
// SOCKET socket_ncap = 0;
SOCKET socket_tim = 0;
unsigned int mes_1451_send_g(unsigned char * data, unsigned int len)
{
    return send(socket_tim, data, len, 0);
}

int main()
{
    char sys[2][50] = {" ---------- start! ---------- ", \
                       " ----------  end! ----------- "},overstate;
    srand((int)time(0));
    printf("\n%s\n",sys[0]);

    unsigned int recv_num = 0;

    mes_1451_send = mes_1451_send_g;

    TIM_status = Idle;
    TEDS_init();
    Message_init();

    /* 各种外设初始化 */

    /* TIM 的 TCP 初始化 */
    socket_tim = win_socket_TCP_client_init(
        1,
        TEST_CLIENT_AIM_ADDR_STR,
        TEST_CLIENT_AIM_PORT
    );

    /* 开始执行 */

    system("pause");
    
    /* TIM 发送 初始化完毕 的 Message，
        在这里，不等待 NCAP 的 ReplyMessage 了（1451_tcp_test_server.c 那里不写） */
    printf("TIM send TIM_initiated Message\n");
    Message_TIM_initiated_pack_up();
    Message_pack_up_And_send();

    while(1)
    {
        /* TIM 接收 Message 到 MES.Message_u->Message_load 这个数组，
            然后 ReplyMessage_Server() 会自动解析 Message 并予以回复 ReplyMessage */
        recv_num = recv(socket_tim, MES.Message_u->Message_load, 
            sizeof(MES.Message_u->Message_load), 0);

        if(recv_num > 0)
        {
            Message_decode(&Message_temp,MES.Message_u->Message_load);

            printf("TIM Message recv:        \
                \n   Dest_TIM:%d                 \
                \n   Dest_TC:%d                  \
                \n   Command_class:%d            \
                \n   Command_function:%d         \
                \n   dependent_Length:%d\n",       \
                Message_temp.Dest_TIM_and_TC_Num[TIM_enum], \
                Message_temp.Dest_TIM_and_TC_Num[TC_enum], \
                Message_temp.Command_class,         \
                Message_temp.Command_function,      \
                Message_temp.dependent_Length       \
            );
            // break;
        }else
        {
            break;
        }
        
        ReplyMessage_Server(MES.Message_u->Message_load);
    }
    

	printf("\n%s\n",sys[1]);
    printf("Press Enter to exit");
    scanf("%c",&overstate);

    closesocket(socket_tim);
	WSACleanup();

    return 0;
}
