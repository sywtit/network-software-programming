/* File Name: StoreServer
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description:Store과 관련된 db를 관리하고, Platform Server와 통신하는 Store Server
 * Last Changed: 2020-06-29
*/

#include <stdio.h>
#include "mysql.h"
#pragma comment(lib, "libmySQL.lib")
#include<winsock.h>
#include<stdlib.h>
#include<iostream>
#define BUFSIZE 1024
#define BUFFERSIZE 1024
#define addressBUFSIZE 128
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PORT 8700 //Store Server의 포트 번호는 8700 //이것을 이용해 Platform Server와 통신한다.
#pragma warning(disable:4996)

//mysql 환경 정보
const char* server = "localhost";
const char* user = "root";
const char* password = "3681";

void err_quit(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)(*msg), MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(-1);
}


// 소켓 함수 오류 출력
void err_display(char* msg)
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

//client인지 server인지 구별하고, rider인지 store인지 구별하는 구조체
//Store Server : type=1, number=1
typedef struct Destination {
    int type;
    int number;
    SOCKADDR_IN addr;
}Destination;

//모든 syntax를 flag별로 분류할 수 있는
//역할을 가진 구조체
/*
* flag = 1; 로그인
* flag = 2; 회원 가입
* flag = 3; 주문 정보 요청
* flag = 4: 광고 등록 요청
* flag = 5: 주분 정보 변경 요청
*/
typedef struct flagprotocol
{
    Destination start;
    Destination end;
    int flag;
    char buffer[BUFSIZE];
}FlagProtocol;

//광고관련 syntax
typedef struct advertisementprotocol
{
    Destination start;
    Destination end;
    int flag;
    int store_index;
    int result;
}AdvertisementProtocol;

//로그인||회원가입 관련 syntax
typedef struct adminprotocol
{
    Destination start;
    Destination end;
    int flag;
    char id[40];
    char password[40];
    char storename[40];
    char address[40];
    char category[10];
    int store_index;
    int result;
}AdminProtocol;

//주문 list관련 구조체
typedef struct orderingInfo
{
    char foodname[30];
    char address[40];
    char status[20];
    char ridername[30];
    char storename[30];
    char timestamp[30];
}OrderingInfo;

//orderingInfo 구조체 포함하는 syntax 
typedef struct orderingInfoList
{
    Destination start;
    Destination end;
    int flag;
    int store_index;
    int whole_row;
    OrderingInfo Info[5];
}OrderingInfoList;

//orderingInfo store 관련 update syntax
typedef struct acceptProtocol
{
    Destination start;
    Destination end;
    int flag;
    int itemid;
    int store_index;
    int statusid;
    int result;
}AcceptProtocol;


/*
로그인시 id와 password정보가 일치하거나, 회원가입시 중복하는지 검사하는 함수
input : loginbuf, AdminProtol 구조체 포인터 형식으로 id, password값이 존재
output : 실패시 -1, 성공시 store DB에 존재하는 store의 _no값
*/
int checkWithLogin(MYSQL* conn, AdminProtocol* loginbuf)
{
    MYSQL_RES* res;
    MYSQL_ROW row;

    char query[200];
    sprintf(query, "select _no from store where id=\"%s\" and password=\"%s\"", loginbuf->id, loginbuf->password);
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);

    if (row == NULL)
    {
        mysql_free_result(res);
        return -1;
    }
    else
    {
        int store_num = atoi(row[0]);
        mysql_free_result(res);
        return store_num;
    }
}

/*
회원가입 시 store의 정보를 store DB에 insert하는 함수
input : loginbuf, store 과 관련된 모든 정보들 포함, id, password, 가게 이름, 가게 주소, 카테고리 포함
*/
void InsertStoreInfo(MYSQL* conn, AdminProtocol* loginbuf)
{
    char query[200];
    sprintf(query, "insert into store(id, password, storename, address, category) values (\'%s\', \'%s\', \'%s\',\'%s\',\'%s\');",
        loginbuf->id, loginbuf->password, loginbuf->storename, loginbuf->address, loginbuf->category);

    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when inserting");
    }

}

/*
Store이 전체 정보를 store DB를 통해 가져와서 그 상점 이름을 반환해주는 함수
input : InfoList 에 담긴 store DB의 _no값
output : store _no 에 해당하는 store 이름을 가져와 commandResponse 변수에 값을 넣어주고 commandResponse 반환
*/
void getStoreInfo(MYSQL* conn, OrderingInfoList* InfoList, OrderingInfoList* commandResponse)
{
    MYSQL_RES* res;
    MYSQL_ROW row;

    char query[200];
    sprintf(query, "select * from store where _no=%d", InfoList->store_index);

    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    while (row = mysql_fetch_row(res))
    {
        memset(commandResponse->Info[0].storename, 0, sizeof(commandResponse->Info[0].storename));
        strncpy(commandResponse->Info[0].storename, row[3], strlen(row[3]));
    }
 
    mysql_free_result(res);

}

/*
전체 주문 정보를 가져오는 함수
input : InfoList, store의 _no의 값을 가지고 있는 데이터
output : 해당 store의 storename과 일치하는 모든 주문 정보를 받아와 commandRespone값에 넣어 반환
*/
void getWholeOrderingInfo(MYSQL* conn, OrderingInfoList* InfoList, OrderingInfoList* commandResponse)
{
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];
    memset(commandResponse, NULL, sizeof(commandResponse));

    //date_format(time_stamp, \'%%H-%%m\')

    getStoreInfo(conn, InfoList, commandResponse);
    sprintf(query, "select _no,storename,menuname,ridername,storestatus, riderstatus, date_format(time_stamp,\'%%H-%%m\'),clientaddress from ordering where storename = \"%s\"", commandResponse->Info[0].storename);

    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    int rowindex = 0;
    while (row = mysql_fetch_row(res))
    {
        memset(commandResponse->Info[rowindex].address, 0, sizeof(commandResponse->Info[0].address));
        memset(commandResponse->Info[rowindex].foodname, 0, sizeof(commandResponse->Info[0].foodname));
        memset(commandResponse->Info[rowindex].ridername, 0, sizeof(commandResponse->Info[0].ridername));
        memset(commandResponse->Info[rowindex].status, 0, sizeof(commandResponse->Info[0].status));
        memset(commandResponse->Info[rowindex].storename, 0, sizeof(commandResponse->Info[0].storename));
        memset(commandResponse->Info[rowindex].timestamp, 0, sizeof(commandResponse->Info[0].timestamp));


        strncpy(commandResponse->Info[rowindex].address, row[7], strlen(row[7]));
        strncpy(commandResponse->Info[rowindex].foodname, row[2], strlen(row[2]));
        if (row[3] != NULL)
        {
            strncpy(commandResponse->Info[rowindex].ridername, row[3], strlen(row[3]));
        }

        if (atoi(row[4]) == 0) strncpy(commandResponse->Info[rowindex].status, "조리 대기", 10);
        else if ((atoi)(row[4]) == 1) strncpy(commandResponse->Info[rowindex].status, "조리 완료", 10);
        else if (atoi(row[4]) == 2) strncpy(commandResponse->Info[rowindex].status, "픽업 완료", 10);
        strncpy(commandResponse->Info[rowindex].storename, commandResponse->Info[0].storename, strlen(commandResponse->Info[0].storename));
        strncpy(commandResponse->Info[rowindex].timestamp, row[6], strlen(row[6]));
        rowindex++;
    }
    commandResponse->whole_row = rowindex;

    commandResponse->start.type = 1;
    commandResponse->end.type = 0;
    commandResponse->start.number = 1;
    commandResponse->end.addr = InfoList->start.addr;
}

/*
Store이 조리 완료를 선택했을때 모든 주문 정보를 update해주는 함수
input : changeInfo에서 변경해줄 store의 _no 값을 포함하는 변수
output : 디비 업데이트 성공 여부를 result로 받아 change_command_response에 담아서 반환
*/
void changeOrderingStatus(MYSQL* conn, AcceptProtocol* changeInfo, AcceptProtocol* change_command_response)
{
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];
    memset(change_command_response, NULL, sizeof(change_command_response));

    sprintf(query, "select storename from store where _no = %d", changeInfo->store_index);
    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);


    sprintf(query, "select * from ordering where storename = \"%s\"", row[0]);
    mysql_free_result(res);


    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    int rowindex = 0;
    while (row = mysql_fetch_row(res))
    {
        if (changeInfo->itemid == rowindex)
        {

            sprintf(query, "update ordering set storestatus = 1 where _no = %d", atoi(row[0]));
            mysql_query(conn, "set names euckr");

            if (mysql_query(conn, query) != 0)
            {
                change_command_response->result = 0;
                err_display((char*)"error when selecting");
            }
            else
            {
                change_command_response->result = 1;
            }
        }
        rowindex++;
    }
    change_command_response->start.type = 1;
    change_command_response->end.type = 0;
    change_command_response->start.number = 1;
    change_command_response->end.addr = changeInfo->start.addr;
}

/*
광고를 등록해 광고 db에 store의 이름을 올리는 함수
input : advertisementInfo즉 해당 store의 _no값을 가지고 있는 데이터
output : advertisement db에 insert를 성공했는지 실패했는지를 설명해주는 result값 넣어서 advertisement_command_response 반환
*/
void insertAdvertisementInfo(MYSQL* conn, AdvertisementProtocol* advertisementInfo, AdvertisementProtocol* advertisement_command_response)
{
    memset(advertisement_command_response, NULL, sizeof(advertisement_command_response));

    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[200];

    sprintf(query, "select storename from store where _no = %d", advertisementInfo->store_index);
    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when selecting");
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);


    mysql_free_result(res);
    sprintf(query, "insert into advertisement(storename) values (\'%s\')", row[0]);


    mysql_query(conn, "set names euckr");
    if (mysql_query(conn, query) != 0)
    {
        err_display((char*)"error when inserting");
        advertisement_command_response->result = 0;
    }
    else
    {
        advertisement_command_response->result = 1;
    }
    advertisement_command_response->start.type = 1;
    advertisement_command_response->end.type = 0;
    advertisement_command_response->start.number = 1;
    advertisement_command_response->end.addr = advertisementInfo->start.addr;

}

/*
로그인 과정, checkWithLogin함수를 부르고 로그인 성공 실패 여부를 반환하는 함수
*/
void loginprocess(AdminProtocol* loginbuf, AdminProtocol* response, MYSQL* conn)
{
    memset(response, NULL, sizeof(response));

    int checkStoreNum = 0;

    if ((checkStoreNum = checkWithLogin(conn, loginbuf)) != -1)
    {
        response->store_index = checkStoreNum;
        response->result = 1;
    }

    else response->result = 0;

    response->flag = 1;

    response->start.type = 1;
    response->end.type = 0;
    response->start.number = 1;
    response->end.addr = loginbuf->start.addr;
}

/*
회원가입 과정, InsertStoreInfo()함수를 부르고 성공 여부를 반환하는 함수
*/
void registerprocess(AdminProtocol* loginbuf, AdminProtocol* response, MYSQL* conn)
{
    memset(response, NULL, sizeof(response));
    if (checkWithLogin(conn, loginbuf) != -1) response->result = 0;
    else
    {
        InsertStoreInfo(conn, loginbuf);
        response->store_index = checkWithLogin(conn, loginbuf);
        response->result = 1;
    }

    response->flag = 2;
    response->start.type = 1;
    response->end.type = 0;
    response->start.number = 1;
    response->end.addr = loginbuf->start.addr;
}

/*
platform server와 udp통신하는 main함수
*/
int main(void) {


    int retval;
    HANDLE			hThread;
    DWORD			ThreadId;
    MYSQL* conn;
    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, server,
        user, password, NULL, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    mysql_query(conn, "use sys");


    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    // socket()
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

    //데이터에 필요한 정보들
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];
    int len;
    int inputNum = 0;
    char buffer[BUFFERSIZE + 1];
    int wholeLoginProcess = 1;

    while (1)
    {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
            (SOCKADDR*)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display((char*)"recvfrom()");
            continue;
        }

        FlagProtocol* command = (FlagProtocol*)buffer;
        AdminProtocol* response;
        OrderingInfoList* command_response;
        AcceptProtocol* change_command_response;
        AdvertisementProtocol* advertisement_command_response;

        switch (command->flag)
        {
        case 1:

            response = (AdminProtocol*)malloc(sizeof(AdminProtocol));
            loginprocess((AdminProtocol*)buffer, (AdminProtocol*)response, conn);

            //로그인 완료
            retval = sendto(sock, (char*)response, sizeof(AdminProtocol), 0,
                (SOCKADDR*)&clientaddr, sizeof(clientaddr));

            if (retval == SOCKET_ERROR)
            {
                err_display((char*)"sendto()");
                continue;
            }

            break;

        case 2:

            response = (AdminProtocol*)malloc(sizeof(AdminProtocol));
            registerprocess((AdminProtocol*)buffer, (AdminProtocol*)response, conn);

            //회원가입 완료
            retval = sendto(sock, (char*)response, sizeof(AdminProtocol), 0,
                (SOCKADDR*)&clientaddr, sizeof(clientaddr));

            if (retval == SOCKET_ERROR)
            {
                err_display((char*)"sendto()");
                continue;
            }

            break;

        case 3:

            command_response = (OrderingInfoList*)malloc(sizeof(OrderingInfoList));
            getWholeOrderingInfo(conn, (OrderingInfoList*)buffer, (OrderingInfoList*)command_response);

            command_response->flag = 1;

            //주문 리스트 반환 완료
            retval = sendto(sock, (char*)command_response, sizeof(OrderingInfoList), 0,
                (SOCKADDR*)&clientaddr, sizeof(clientaddr));

            if (retval == SOCKET_ERROR)
            {
                err_display((char*)"sendto()");
                continue;
            }

            break;

        case 4:
            advertisement_command_response = (AdvertisementProtocol*)malloc(sizeof(AdvertisementProtocol));
            insertAdvertisementInfo(conn, (AdvertisementProtocol*)buffer, (AdvertisementProtocol*)advertisement_command_response);

            //광고 등록 완료
            retval = sendto(sock, (char*)advertisement_command_response, sizeof(AdvertisementProtocol), 0,
                (SOCKADDR*)&clientaddr, sizeof(clientaddr));

            if (retval == SOCKET_ERROR)
            {
                err_display((char*)"sendto()");
                continue;
            }
            break;

        case 5:
            change_command_response = (AcceptProtocol*)malloc(sizeof(AcceptProtocol));
            changeOrderingStatus(conn, (AcceptProtocol*)buffer, (AcceptProtocol*)change_command_response);

            //업데이트 완료
            retval = sendto(sock, (char*)change_command_response, sizeof(AcceptProtocol), 0,
                (SOCKADDR*)&clientaddr, sizeof(clientaddr));

            if (retval == SOCKET_ERROR)
            {
                err_display((char*)"sendto()");
                continue;
            }

            break;
        default:
            break;
        }

    }



}