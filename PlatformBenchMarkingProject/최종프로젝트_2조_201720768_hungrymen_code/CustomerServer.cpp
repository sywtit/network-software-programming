
#include <stdio.h>
#include <winsock.h>
#include <string.h>
#include "mysql.h"

#pragma comment(lib, "libmySQL.lib")
#define KOREA_FOOD "�ѽ�"
#define CHINA_FOOD "�߽�"
#define WESTERN_FOOD "���"
#define JAPAN_FOOD "�Ͻ�"
/*�ѱ۷� ���� UTF-8 �ʿ�*/

#define BUFSIZE 512

void err_quit(const char* msg)
{
    printf("%s", msg);
    exit(-1);
}

// ���� �Լ� ���� ���
void err_display(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}
struct infotoClient {
    char id[20];
    char pw[20];
    char recvlogin[100];
    int flag;
};

struct Orderinfo {
    char cartegory[70] = "  ===ī�װ�����===\n  1.�ѽ�\n  2.���\n  3.�߽�\n  4.�Ͻ�\n";
    char recvnum[100];
    int selectnum;
    char storename[100];
    char recvstore[100];
    char menuname[100];
    char addr[100];
    char state[200];
    int flag;

};

struct countStore {
    char storeinfo[100];
};

/*������ storename �ִ��� üũ*/
int checkstorename(MYSQL* conn, Orderinfo o) {
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];
    int  count = 3;

    sprintf(query, "select * from store");
    mysql_query(conn, query);
    res = mysql_store_result(conn);
    //printf("input : %s\n", o.recvstore);
    while (row = mysql_fetch_row(res)) {
        //printf("store�� �ִ� row : %s\n", row[3]);
        if (strcmp(o.recvstore, row[3]) == 0) {
            count = 1;
            break;
        }
        else
            count = 0;
    }
    return count;
}

/*�α��� ���� �Լ�*/
int loginClient(MYSQL* conn, infotoClient s) {
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];
    int user_no = 0;

    sprintf(query, "select _no from customer where id=\"%s\" and password=\"%s\"", s.id, s.pw);
    mysql_query(conn, query);
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (row == NULL)
        user_no = 0;
    else {
        user_no = atoi(row[0]);
    }
    if (user_no != 0)
        return 1;
    /*�α��� ����*/
    else
        return 0;
}

/*ȸ������ ���� �Լ�*/
int registerClient(MYSQL* conn, infotoClient s) {
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];
    int user_no = 0;

    sprintf(query, "select _no from customer where id=\"%s\" and password=\"%s\"", s.id, s.pw);
    mysql_query(conn, query);
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if (row == NULL)
        user_no = 0;
    else
        user_no = atoi(row[0]);

    /*�̹� ���� user name ����*/
    if (user_no != 0)
        return 1;
    /*ȸ������ ����*/
    else {
        sprintf(query, "insert into customer(id, password) values (\'%s\', \'%s\');",
            s.id, s.pw);
        mysql_query(conn, query);
    }
}

int main(void) {

    /*�����ͺ��̽� ���� ����*/
    MYSQL* conn; //mysql���� Ŀ�ؼ��� ��µ� ���������� ���Ǵ� ��������.   
    MYSQL_RES* res;  //�������� ���� result���� �޴� ��ġ��������.   
    MYSQL_ROW row;   //�������� ���� ���� �����Ͱ��� ����ִ� ��������.   
    int len;
    char query[200];

    const char* server = "localhost";  //������ ����ε��� localhost�� �ϸ� �ڱ� ��ǻ�Ͷ� �ǹ̶��ϴ�.   
    const char* user = "root"; //mysql�α��� ���̵��ε���. �⺻������ ���ٸ� ������ ������ root����   
    const char* password = "3681"; /* set me first */   //password�� �ִºκ��̿���   
    //const char* database = "netprotocol";  //Database �̸��� �־��ִ� �κ��̿���.   

    /*UDP ���� ���� ����*/
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE];
    char buftemp[BUFSIZE] = "";



    /*���������� ������ ����*/
    int logincount = 0;
    infotoClient clientinfo;
    Orderinfo orderinfo;
    countStore storearr[30];
    char storecountstr[20];

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    // socket()
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8000);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    conn = mysql_init(NULL); //connection ������ �ʱ�ȭ ���ѿ�.   

    /* Connect to database *///DB���� MYSQL�� ���ӽ�ų������   
    if (!mysql_real_connect(conn, server,    //mysql_real_connect()�Լ��� ������ �����ִ� �Լ�����   
        user, password, NULL, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    mysql_query(conn, "use sys");
    mysql_query(conn, "set names euckr");

    while (true) {

        addrlen = sizeof(clientaddr);

        while (logincount == 0) {
            /*ȸ�����԰� �α��ι���*/
            retval = recvfrom(sock, clientinfo.recvlogin, sizeof(clientinfo), 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recv id Error !");
                continue;
            }
            clientinfo.recvlogin[retval] = '\0';
            clientinfo.flag = atoi(clientinfo.recvlogin);


            /*�޴� �޼��� flag, id, pw ������ �����ϱ�*/
            char* ptr = strtok(clientinfo.recvlogin, " ");
            for (int i = 0; i < 3; i++) {
                if (i == 0)
                    clientinfo.flag = atoi(ptr);
                else if (i == 1) {
                    strcpy(clientinfo.id, ptr);
                    len = strlen(ptr);
                    clientinfo.id[len] = '\0';
                }
                else {
                    strcpy(clientinfo.pw, ptr);
                    len = strlen(ptr);
                    clientinfo.pw[len] = '\0';
                }
                ptr = strtok(NULL, " ");
            }

            /*�α��� ��Ʈ*/
            if (clientinfo.flag == 1) {
                if (loginClient(conn, clientinfo) == 0) {
                    sendto(sock, "�ش� ������ �����ϴ�.\n", sizeof("�ش� ������ �����ϴ�.\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    continue;
                }
                else {
                    sendto(sock, "�α��� �Ϸ� !\n", sizeof("�α��� �Ϸ� !\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    logincount = 1;
                    break;
                }
            }

            /*ȸ������ ��Ʈ*/
            else if (clientinfo.flag == 2) {
                if (registerClient(conn, clientinfo) == 1) {
                    sendto(sock, "ȸ���� �����մϴ�.\n", sizeof("ȸ���� �����մϴ�.\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    continue;
                }
                else {
                    sendto(sock, "ȸ������ �Ϸ�\n", sizeof("ȸ������ �Ϸ�\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    logincount = 1;
                    break;
                }
            }
        }

        /*2. ī�װ� �����ϰ�, ī�װ� ��� ���� �ޱ�
        : ���� ��ġ�ϴ� ������ ���ٸ� �ٽ� ī�װ� �����ؼ� �����*/
        int storecount = 0;
        while (1) {
            retval = sendto(sock, orderinfo.cartegory, sizeof(orderinfo.cartegory), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
                printf("send error()");
                continue;
            }
            retval = recvfrom(sock, orderinfo.recvnum, sizeof(orderinfo.recvnum), 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recv id Error !");
                continue;
            }
            orderinfo.recvnum[retval] = '\0';
            orderinfo.selectnum = atoi(orderinfo.recvnum);
            //printf("���ù��� ī�װ� : %d", orderinfo.selectnum);

            /*3.ī�װ��� �´� ���� ����Ʈ �����ϱ�*/


            mysql_query(conn, "select * from store");
            res = mysql_store_result(conn);
            while (row = mysql_fetch_row(res)) {
                /*ī�װ�1) �ѽ�*/
                if (orderinfo.selectnum == 1) {
                    if (strcmp(row[5], KOREA_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }
                /*ī�װ�2) ���*/
                else if (orderinfo.selectnum == 2) {
                    if (strcmp(row[5], WESTERN_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }

                /*ī�װ� 3) �߽�*/
                else if (orderinfo.selectnum == 3) {
                    if (strcmp(row[5], CHINA_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }
                /*ī�װ� 4) �Ͻ�*/
                else if (orderinfo.selectnum == 4) {
                    if (strcmp(row[5], JAPAN_FOOD) == 0)
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount++, row[3]);
                    else
                        continue;
                }

            }

            /*store ������ ���� ����*/
            sprintf(storecountstr, "%d", storecount);
            retval = sendto(sock, storecountstr, sizeof(storecountstr), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
                printf("send error()");
                continue;
            }
            if (storecount == 0) {
                sendto(sock, "ī�װ� �� �ֹ� ������ ������ �������� �ʽ��ϴ�.", sizeof("ī�װ� �� �ֹ� ������ ������ �������� �ʽ��ϴ�."), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
            }
            else {
                for (int i = 0; i < storecount; i++) {
                    printf("%s\n", storearr[i].storeinfo);
                    retval = sendto(sock, storearr[i].storeinfo, sizeof(storearr[i].storeinfo), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                }
                break;
            }

        }
        /*4. ���� �̸��� �޴� �̸� �ֹ� �ޱ�*/
        while (1) {

            /*���� �����ް�, ����Ʈ�� �ִ� �������� üũ*/
            retval = recvfrom(sock, orderinfo.recvstore, sizeof(orderinfo.recvstore), 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recv Error !");
                continue;
            }
            orderinfo.recvstore[retval] = '\0';
            if (checkstorename(conn, orderinfo) == 1) {
                /*�޴��̸��� ���� ���� send*/
                retval = sendto(sock, "  �޴� �̸� : ", sizeof("  �޴� �̸� : "), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                if (retval == SOCKET_ERROR) {
                    printf("send error()");
                    continue;
                }
                break;
            }
            else {
                /*�ٽ� �Է��϶�� flag 0 ����*/
                retval = sendto(sock, "0", sizeof("0"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                if (retval == SOCKET_ERROR) {
                    printf("send error()");
                    continue;
                }
                continue;
            }
        }

        /*�޴��� ���� ���� recv*/
        retval = recvfrom(sock, orderinfo.menuname, sizeof(orderinfo.menuname), 0, (SOCKADDR*)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recv Error !");
            continue;
        }
        orderinfo.menuname[retval] = '\0';

        /*�ֹ��� �ּ� ����� �ޱ�*/
        retval = sendto(sock, "  �ֹ��� �ּ� : ", sizeof("  �ֹ��� �ּ� : "), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
        if (retval == SOCKET_ERROR) {
            printf("send error()");
            continue;
        }
        retval = recvfrom(sock, orderinfo.addr, sizeof(orderinfo.addr), 0, (SOCKADDR*)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recv Error !");
            continue;
        }
        orderinfo.addr[retval] = '\0';

        //5. order�� ���� info sql�� ������Ʈ�ϰ� order���� send
        sprintf(query, "insert into ordering (storename,menuname,ridername,storestatus,riderstatus,clientaddress) values (\"%s\",\"%s\", NULL,%d,%d,\"%s\");", orderinfo.recvstore, orderinfo.menuname, 0, 0, orderinfo.addr);
        mysql_query(conn, query);

        sprintf(orderinfo.state, "\n\n  ����������  �ֹ������Ϸ᦬��������\n  �����̸� : %s\n  �޴��̸� : %s\n   �ּ� : %s\n   ����������������������������������������\n", orderinfo.recvstore, orderinfo.menuname, orderinfo.addr);
        //�ּ� ������Ʈ�ؼ� �����ϸ� �ȴ�
        retval = sendto(sock, orderinfo.state, sizeof(orderinfo.state), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
        if (retval == SOCKET_ERROR) {
            printf("send error()");
            continue;
        }

        break;
    }
    closesocket(sock);

    // ���� ����
    WSACleanup();
    mysql_close(conn);
}