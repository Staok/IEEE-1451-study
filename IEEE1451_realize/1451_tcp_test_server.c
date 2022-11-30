#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "socket\socket.h"
#include "IEEE1451_5_lib\IEEE1451_5_lib.h"

/* 编译命令：这里是 win 下 使用 MinGW 编译
    gcc 1451_tcp_test_server.c .//socket//socket.c .//IEEE1451_5_lib//IEEE1451_5_lib.c -I .//socket -I .//IEEE1451_5_lib -lwsock32 -o 1451_tcp_test_server.exe
*/

/* 我是 NCAP 程序 */

/* IEEE 1451 Message 数据 发送 接口 API */
SOCKET socket_ncap = 0;
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

    // TIM_status = Idle;
    // TEDS_init();
    Message_init();

    /* 各种外设初始化 */

    /* NCAP 的 TCP 初始化 */
    socket_ncap = win_socket_TCP_server_init(
        1,
        TEST_SERVER_ADDR_STR,
        TEST_SERVER_PORT,
        5
    );

    /* 阻塞等待 TIM 的 TCP 连接 */
    struct sockaddr_in caddr = { 0 };
    int csize = sizeof(caddr);
    socket_tim = accept(socket_ncap, (struct sockaddr*)&caddr, &csize);
        if (socket_tim == INVALID_SOCKET)
        {
            perror("server accept error\n");
            exit(-1);
        }
    printf("socket_tim = %#X\n", socket_tim);
    printf("client ip:%s port:%d\n",
            inet_ntoa(caddr.sin_addr),
            ntohs(caddr.sin_port));

    /* 开始执行 */
    
    /* TIM 会发送 初始化完毕 的 Message */

    /* NCAP 阻塞等待接收 TIM 的初始化完毕 的 Message，
        NCAP 接收 Message，接收的 数据 直接放入 MES.Message_u->Message_load 这个数组，
        然后 Message_decode() 会解析 并填充到 Message_temp 结构体里面 */
    recv_num = recv(socket_tim, MES.Message_u->Message_load, 
        sizeof(MES.Message_u->Message_load), 0);
    Message_decode(&Message_temp,MES.Message_u->Message_load);

    printf("NCAP Message recv:        \
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

    system("pause");

    /* NCAP 发送 Query_TEDS 的 Message */
    printf("NCAP send Query_TEDS Message\n");
    Message_CommonCmd_Query_TEDS_pack_up(TIM_3, TC_12, PHY_TEDS_ACCESS_CODE);
    Message_pack_up_And_send();

    /* NCAP 阻塞等待接收 TIM 的 ReplyMessage，
        NCAP 接收 ReplyMessage，接收的 数据 直接放入 MES.ReplyMessage_u->ReplyMessage_load 这个数组，
        然后 ReplyMessage_decode() 会解析 并填充到 ReplyMessage_temp 结构体里面 */
    recv_num = recv(socket_tim, MES.ReplyMessage_u->ReplyMessage_load, 
        sizeof(MES.ReplyMessage_u->ReplyMessage_load), 0);
    ReplyMessage_decode(&ReplyMessage_temp, MES.ReplyMessage_u->ReplyMessage_load);
    
    printf("NCAP ReplyMessage recv:         \
        \n   Flag:%d                        \
        \n   dependent_Length:%d\n",        \
        ReplyMessage_temp.Flag,             \
        ReplyMessage_temp.dependent_Length  \
    );

    system("pause");

	printf("\n%s\n",sys[1]);
    printf("Press Enter to exit");
    scanf("%c",&overstate);

    closesocket(socket_tim);
    closesocket(socket_ncap);
	WSACleanup();

    return 0;
}
