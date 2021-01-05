#include<winsock.h>
#include<stdlib.h>
#include<stdio.h>

#define BUFSIZE 1500

// ���� �Լ� ���� ��� �� ����
void err_quit(const char* msg)
{
    printf("Error [%s] ... program terminated \n", msg);
    exit(-1);
}

// ���� �Լ� ���� ���
void err_display(const char* msg)
{
    printf("socket function error [%s]\n", msg);
}

typedef struct UDP_PROTO {
    int type;
    int port;
}UDP_PROTO;

typedef struct CONNECT_ARG {
    int type;
    char ip[20];
    int port;
}CONNECT_ARG;

typedef struct CodeAndValue {
    char code[6];
    float value;
}codeandvalue;

//snmp �������� ������ ����ü syntax
typedef struct mibStructure {
    int pdu_type;
    codeandvalue array[5];
}mibStructure;

DWORD WINAPI TCP_client(LPVOID arg) {
    CONNECT_ARG* params = (CONNECT_ARG*)arg;
    int port = params->port;
    char ip[100];
    strcpy(ip, params->ip);
    SOCKADDR_IN serveraddr;
    int retval, len;
    char buf[BUFSIZE + 1];

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    //tcp ��¸� �̰ɷ� �ϸ� �˴ϴ�.

    serveraddr.sin_addr.s_addr = inet_addr(ip);
    //printf("\n\nTCP Connect : IP - %s Port - %d ,", params->ip, params->port);

    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // communication with the server
    while (1) {
        // input data to be sent to the server
        ZeroMemory(buf, sizeof(buf));

        sprintf(buf, "request\0");

        // send message
        retval = send(sock, buf, strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("socket error: send()");
            break;
        }
        //printf("[TCP Clinet: send] message %d bytes sent ... \n\n", retval);
        printf("TCP Client send Request\n");
        printf(": Connect : IP - %s Port - %d , %d\n", ip, port);
        // receive echo message from the server


        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("socket error: recv()");
            break;
        }
        else if (retval == 0)
            continue;
        buf[retval] = '\0';

        //���� ���� ��� 
        mibStructure* input;
        input = (mibStructure*)malloc(sizeof(mibStructure));
        input = (mibStructure*)buf;
        printf(" ------------------------------------------------\n");
        printf("|  IP  | \t1.IP PACKET \t:\t %.1f\t|\n", input->array[0].value);
        printf("|                                               |\n");
        printf("|-----------------------------------------------|\n");
        printf("|  TCP | \t1. TCP PACKET \t:\t %.1f\t|\n", input->array[1].value);
        printf("|      | \t2. Syn PACKET \t:\t %.1f\t|\n", input->array[2].value);
        printf("|      | \t3. BIT RATE \t:\t %.1f\t|\n", input->array[3].value);
        printf("|                                               |\n");
        printf("|-----------------------------------------------|\n");
        printf("|  UDP | \t1.UDP PACKET \t:\t %.1f\t|\n", input->array[4].value);
        printf(" ------------------------------------------------\n");
        printf("\n\n");
        /*��� �׽�Ʈ*/

        _sleep(5000);
    }

    // closesocket()
    closesocket(sock);

    // ���� ����
    return 0;
}

int main(int argc, char* argv[]) {
    int retval;
    HANDLE hThread;
    DWORD ThreadId;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8000);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    //printf("\n");
    while (1) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, sizeof(UDP_PROTO), 0, (SOCKADDR*)&clientaddr, &addrlen);

        if (retval == SOCKET_ERROR) {
            err_display("recvfrom()");
            continue;
        }

        buf[retval] = '\0';

        UDP_PROTO* recvbuf;
        //mibs�� ���������� ����Ѵ�.
        recvbuf = (UDP_PROTO*)buf;
        //udp �ȿ� tcp ������ ���ư��� ��

        if (recvbuf->type == 0) {
            //Syn Flooding �߻������� ��� �޽��� ���
            printf("|***********************************************|\n");
            printf("| \tSyn Flooding Attack !!\t|\n");
            printf("| where ip : %s, port : %d\t|\n", inet_ntoa(clientaddr.sin_addr), recvbuf->port);
            printf("|***********************************************|\n");
        }
        else {

            //��������°� type1�� agent���� �����س���
            CONNECT_ARG* params = (CONNECT_ARG*)malloc(sizeof(CONNECT_ARG));
            params->type = recvbuf->type;
            strcpy(params->ip, inet_ntoa(clientaddr.sin_addr));
            params->port = recvbuf->port;

            //tcp ������ ����
            hThread = CreateThread(NULL, 0, TCP_client, (LPVOID)params, 0, &ThreadId);
            if (hThread == NULL)
                err_display((char*)"error: failure of thread creation!!!");
            else
                CloseHandle(hThread);
        }

        //echo �޼������� ���� �޾ҽ��ϴٶ�� ���
        sprintf(buf, "check\0");
        retval = sendto(sock, buf, strlen(buf), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

        continue;
    }
}