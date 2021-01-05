#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>
#include "mysql.h"
#pragma comment(lib, "libmySQL.lib")

#define BUFSIZE 1024
#define PORT 8600

//데이터 베이스 접근 정보
const char* server = "localhost";
const char* user = "root";
const char* password = "3681";

/*
type		0은 client,		1은 server
number		0은 customer,  1은 store,	2는 rider,   3은 외부
*/
typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;

/*
라우팅의 기본인 방향 구조체다.
*/
typedef struct PlatformProtocol {
	Destination start;
	Destination end;
	int flag;
	char data[950];
}PlatformProtocol;

/*
데이터베이스 출력 형식입니다.
*/

char DataFrom[7][5][20];

void err_quit(char* msg)
{
	printf("%s\n", msg);
}

void err_display(char* msg)
{
	printf("%s\n", msg);
}

/*
플랫폼에 존재하는 서버들의 정보.
*/

void setServerAddr(void);

int main(void) {
	int retval;
	MYSQL* conn;
	conn = mysql_init(NULL);

	if (!mysql_real_connect(conn, server,
		user, password, NULL, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}
	mysql_query(conn, "use sys");

	// 윈속 초기화
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

	SOCKET sendsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	//서버의 ip port 정비 함수.
	SOCKADDR_IN rideraddr;
	ZeroMemory(&rideraddr, sizeof(rideraddr));
	rideraddr.sin_family = AF_INET;
	rideraddr.sin_port = htons(8400);
	rideraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	SOCKADDR_IN storeaddr;
	ZeroMemory(&storeaddr, sizeof(storeaddr));
	storeaddr.sin_family = AF_INET;
	storeaddr.sin_port = htons(8700);
	storeaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
			continue;
		}

		PlatformProtocol* platformbuf = (PlatformProtocol*)buf;
		platformbuf->start.addr = clientaddr;

		//서버 클라이언트 간 통신.
		if ((platformbuf->end.type + platformbuf->start.type) == 1) {
			SOCKADDR_IN destination;
			int wrongflag = 0;
			/*
			발신인이 클라이언트라면
			목적지는 서버이고 해당 서버에 가기 위해 플랫폼 서버에 등록된 sockaddr_in 값들을 불러온다.
			*/
			if (platformbuf->start.type == 0) {
				switch (platformbuf->end.number) {
				case 0:
					//destination = customserver;
					break;
				case 1:
					destination = storeaddr;
					break;
				case 2:
					destination = rideraddr;
					break;
				default:
					printf("이외 서버는 아직 없습니다.");
					wrongflag = 1;
					break;
				}
			}
			/*
			발신인이 서버라면
			이미 platformbuf에는 목적지의 sockaddr_in이 저장되어 있다. 이를 destincation으로 받아오자.
			*/
			else {
				destination = platformbuf->end.addr;
			}

			/*
			만약 아직 플랫폼 서버에서 sockaddr_in 값이 없는 서버라면
			wrongflag가 1이 된다.
			*/
			if (wrongflag) {
				printf("아직 새로운 서버는 확장되지 않은 상태입니다.\n");
				platformbuf->flag = -1;
				retval = sendto(sock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
					(SOCKADDR*)&clientaddr, sizeof(SOCKADDR_IN));
				if (retval == SOCKET_ERROR) {
					err_display((char*)"sendto()");
					continue;
				}
				continue;
			}

			retval = sendto(sock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
				(SOCKADDR*)&destination, sizeof(SOCKADDR_IN));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
		}
		//서버서 서버로 (서버간 서비스 활용) (자기자신 시 플랫폼의 디비 접근 활용)
		else if (platformbuf->end.type == 1 && platformbuf->start.type == 1) {
			SOCKADDR_IN destination;
			int wrongflag = 0;
			//자기자신의 루프인 경우 플랫폼의 데이터베이스 접근을 요구합니다.
			if (platformbuf->start.number == platformbuf->end.number) {
				MYSQL_RES* res;
				MYSQL_ROW row;

				if (mysql_query(conn, platformbuf->data) != 0) {
					{
						printf("오류다.");
					}
				}

				res = mysql_store_result(conn);
				memset(DataFrom, 0x00, sizeof(700));
				int j = 0;
				while (row = mysql_fetch_row(res)) {
					for (int i = 0; i < mysql_field_count((MYSQL*)res); i++) {
						strcpy(DataFrom[j][i], row[i]);
					}
				}
				strcpy(platformbuf->data, (char*)DataFrom);
				retval = sendto(sock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
					(SOCKADDR*)&clientaddr, sizeof(SOCKADDR_IN));
				if (retval == SOCKET_ERROR) {
					err_display((char*)"sendto()");
					continue;
				}
			}
			//플랫폼 내의 서버간 통신을 지원합니다.
			else {
				switch (platformbuf->end.number) {
				case 0:
					//destination = customserver;
					break;
				case 1:
					destination = storeaddr;
					break;
				case 2:
					destination = rideraddr;
					break;
				default:
					printf("플랫폼이 지원하지 않는 서버입니다.");
					wrongflag = 1;
					break;
				}
				if (wrongflag) {
					printf("아직 새로운 서버는 확장되지 않은 상태입니다.\n");
					platformbuf->flag = -1;
					retval = sendto(sock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
						(SOCKADDR*)&clientaddr, sizeof(SOCKADDR_IN));
					if (retval == SOCKET_ERROR) {
						err_display((char*)"sendto()");
						continue;
					}
					continue;
				}

				retval = sendto(sendsock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
					(SOCKADDR*)&destination, sizeof(SOCKADDR_IN));
				if (retval == SOCKET_ERROR) {
					err_display((char*)"sendto()");
					continue;
				}
			}
		}
		/*
		클라이언트 간 통신은 private IP 때문에 까다롭다.
		그래서 일단은 지원하지 않았다.
		*/
		else {
			printf("아직은 클라이언트 간 통신이 허용되지 않습니다. \n");
			platformbuf->flag = -1;
			retval = sendto(sock, (char*)platformbuf, sizeof(PlatformProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
		}
	}
}
