
#include <stdio.h>
#include <winsock.h>
#include <string.h>
#include "mysql.h"

#pragma comment(lib, "libmySQL.lib")
#define KOREA_FOOD "한식"
#define CHINA_FOOD "중식"
#define WESTERN_FOOD "양식"
#define JAPAN_FOOD "일식"
/*한글로 변경 UTF-8 필요*/

#define BUFSIZE 512

void err_quit(const char* msg)
{
    printf("%s", msg);
    exit(-1);
}

// 소켓 함수 오류 출력
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
    char cartegory[70] = "  ===카테고리선택===\n  1.한식\n  2.양식\n  3.중식\n  4.일식\n";
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

/*동일한 storename 있는지 체크*/
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
        //printf("store에 있는 row : %s\n", row[3]);
        if (strcmp(o.recvstore, row[3]) == 0) {
            count = 1;
            break;
        }
        else
            count = 0;
    }
    return count;
}

/*로그인 관련 함수*/
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
    /*로그인 성공*/
    else
        return 0;
}

/*회원가입 관련 함수*/
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

    /*이미 같은 user name 존재*/
    if (user_no != 0)
        return 1;
    /*회원가입 성공*/
    else {
        sprintf(query, "insert into customer(id, password) values (\'%s\', \'%s\');",
            s.id, s.pw);
        mysql_query(conn, query);
    }
}

int main(void) {

    /*데이터베이스 관련 선언*/
    MYSQL* conn; //mysql과의 커넥션을 잡는데 지속적으로 사용되는 변수에요.   
    MYSQL_RES* res;  //쿼리문에 대한 result값을 받는 위치변수에요.   
    MYSQL_ROW row;   //쿼리문에 대한 실제 데이터값이 들어있는 변수에요.   
    int len;
    char query[200];

    const char* server = "localhost";  //서버의 경로인데요 localhost로 하면 자기 컴퓨터란 의미랍니다.   
    const char* user = "root"; //mysql로그인 아이디인데요. 기본적으로 별다른 설정이 없으면 root에요   
    const char* password = "3681"; /* set me first */   //password를 넣는부분이에요   
    //const char* database = "netprotocol";  //Database 이름을 넣어주는 부분이에요.   

    /*UDP 관련 변수 선언*/
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE];
    char buftemp[BUFSIZE] = "";



    /*개인적으로 선언한 변수*/
    int logincount = 0;
    infotoClient clientinfo;
    Orderinfo orderinfo;
    countStore storearr[30];
    char storecountstr[20];

    // 윈속 초기화
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

    conn = mysql_init(NULL); //connection 변수를 초기화 시켜요.   

    /* Connect to database *///DB없이 MYSQL만 접속시킬꺼에요   
    if (!mysql_real_connect(conn, server,    //mysql_real_connect()함수가 연결을 시켜주는 함수에요   
        user, password, NULL, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    mysql_query(conn, "use sys");
    mysql_query(conn, "set names euckr");

    while (true) {

        addrlen = sizeof(clientaddr);

        while (logincount == 0) {
            /*회원가입과 로그인버전*/
            retval = recvfrom(sock, clientinfo.recvlogin, sizeof(clientinfo), 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recv id Error !");
                continue;
            }
            clientinfo.recvlogin[retval] = '\0';
            clientinfo.flag = atoi(clientinfo.recvlogin);


            /*받는 메세지 flag, id, pw 순으로 구분하기*/
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

            /*로그인 파트*/
            if (clientinfo.flag == 1) {
                if (loginClient(conn, clientinfo) == 0) {
                    sendto(sock, "해당 유저가 없습니다.\n", sizeof("해당 유저가 없습니다.\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    continue;
                }
                else {
                    sendto(sock, "로그인 완료 !\n", sizeof("로그인 완료 !\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    logincount = 1;
                    break;
                }
            }

            /*회원가입 파트*/
            else if (clientinfo.flag == 2) {
                if (registerClient(conn, clientinfo) == 1) {
                    sendto(sock, "회원이 존재합니다.\n", sizeof("회원이 존재합니다.\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    continue;
                }
                else {
                    sendto(sock, "회원가입 완료\n", sizeof("회원가입 완료\n"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                    logincount = 1;
                    break;
                }
            }
        }

        /*2. 카테고리 전달하고, 카테고리 양식 선택 받기
        : 만약 일치하는 상점이 없다면 다시 카테고리 전달해서 물어보기*/
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
            //printf("선택받은 카테고리 : %d", orderinfo.selectnum);

            /*3.카테고리에 맞는 상점 리스트 전송하기*/


            mysql_query(conn, "select * from store");
            res = mysql_store_result(conn);
            while (row = mysql_fetch_row(res)) {
                /*카테고리1) 한식*/
                if (orderinfo.selectnum == 1) {
                    if (strcmp(row[5], KOREA_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }
                /*카테고리2) 양식*/
                else if (orderinfo.selectnum == 2) {
                    if (strcmp(row[5], WESTERN_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }

                /*카테고리 3) 중식*/
                else if (orderinfo.selectnum == 3) {
                    if (strcmp(row[5], CHINA_FOOD) == 0) {
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount + 1, row[3]);
                        storecount += 1;
                    }
                    else
                        continue;
                }
                /*카테고리 4) 일식*/
                else if (orderinfo.selectnum == 4) {
                    if (strcmp(row[5], JAPAN_FOOD) == 0)
                        sprintf(storearr[storecount].storeinfo, "  %d)%s\n", storecount++, row[3]);
                    else
                        continue;
                }

            }

            /*store 정보에 대해 전송*/
            sprintf(storecountstr, "%d", storecount);
            retval = sendto(sock, storecountstr, sizeof(storecountstr), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
                printf("send error()");
                continue;
            }
            if (storecount == 0) {
                sendto(sock, "카테고리 내 주문 가능한 상점이 존재하지 않습니다.", sizeof("카테고리 내 주문 가능한 상점이 존재하지 않습니다."), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
            }
            else {
                for (int i = 0; i < storecount; i++) {
                    printf("%s\n", storearr[i].storeinfo);
                    retval = sendto(sock, storearr[i].storeinfo, sizeof(storearr[i].storeinfo), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                }
                break;
            }

        }
        /*4. 상점 이름과 메뉴 이름 주문 받기*/
        while (1) {

            /*상점 정보받고, 리스트에 있는 상점인지 체크*/
            retval = recvfrom(sock, orderinfo.recvstore, sizeof(orderinfo.recvstore), 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recv Error !");
                continue;
            }
            orderinfo.recvstore[retval] = '\0';
            if (checkstorename(conn, orderinfo) == 1) {
                /*메뉴이름에 대한 정보 send*/
                retval = sendto(sock, "  메뉴 이름 : ", sizeof("  메뉴 이름 : "), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                if (retval == SOCKET_ERROR) {
                    printf("send error()");
                    continue;
                }
                break;
            }
            else {
                /*다시 입력하라고 flag 0 전송*/
                retval = sendto(sock, "0", sizeof("0"), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
                if (retval == SOCKET_ERROR) {
                    printf("send error()");
                    continue;
                }
                continue;
            }
        }

        /*메뉴에 대한 정보 recv*/
        retval = recvfrom(sock, orderinfo.menuname, sizeof(orderinfo.menuname), 0, (SOCKADDR*)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recv Error !");
            continue;
        }
        orderinfo.menuname[retval] = '\0';

        /*주문자 주소 물어보고 받기*/
        retval = sendto(sock, "  주문자 주소 : ", sizeof("  주문자 주소 : "), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
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

        //5. order에 대한 info sql에 업데이트하고 order정보 send
        sprintf(query, "insert into ordering (storename,menuname,ridername,storestatus,riderstatus,clientaddress) values (\"%s\",\"%s\", NULL,%d,%d,\"%s\");", orderinfo.recvstore, orderinfo.menuname, 0, 0, orderinfo.addr);
        mysql_query(conn, query);

        sprintf(orderinfo.state, "\n\n  ━━━━━  주문접수완료━━━━━\n  상점이름 : %s\n  메뉴이름 : %s\n   주소 : %s\n   ━━━━━━━━━━━━━━━━━━━━\n", orderinfo.recvstore, orderinfo.menuname, orderinfo.addr);
        //주소 업데이트해서 전송하면 된다
        retval = sendto(sock, orderinfo.state, sizeof(orderinfo.state), 0, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
        if (retval == SOCKET_ERROR) {
            printf("send error()");
            continue;
        }

        break;
    }
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    mysql_close(conn);
}