#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>
#include "mysql.h"
#pragma comment(lib, "libmySQL.lib")

#define BUFSIZE 1024
#define PORT 8600

//������ ���̽� ���� ����
const char* server = "localhost";
const char* user = "root";
const char* password = "3681";

/*
type		0�� client,		1�� server
number		0�� customer,  1�� store,	2�� rider,   3�� �ܺ�
*/
typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;

/*
������� �⺻�� ���� ����ü��.
*/
typedef struct PlatformProtocol {
	Destination start;
	Destination end;
	int flag;
	char data[950];
}PlatformProtocol;

/*
�����ͺ��̽� ��� �����Դϴ�.
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
�÷����� �����ϴ� �������� ����.
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

	// ���� �ʱ�ȭ
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

	//������ ip port ���� �Լ�.
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

	// ������ ��ſ� ����� ����
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

		//���� Ŭ���̾�Ʈ �� ���.
		if ((platformbuf->end.type + platformbuf->start.type) == 1) {
			SOCKADDR_IN destination;
			int wrongflag = 0;
			/*
			�߽����� Ŭ���̾�Ʈ���
			�������� �����̰� �ش� ������ ���� ���� �÷��� ������ ��ϵ� sockaddr_in ������ �ҷ��´�.
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
					printf("�̿� ������ ���� �����ϴ�.");
					wrongflag = 1;
					break;
				}
			}
			/*
			�߽����� �������
			�̹� platformbuf���� �������� sockaddr_in�� ����Ǿ� �ִ�. �̸� destincation���� �޾ƿ���.
			*/
			else {
				destination = platformbuf->end.addr;
			}

			/*
			���� ���� �÷��� �������� sockaddr_in ���� ���� �������
			wrongflag�� 1�� �ȴ�.
			*/
			if (wrongflag) {
				printf("���� ���ο� ������ Ȯ����� ���� �����Դϴ�.\n");
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
		//������ ������ (������ ���� Ȱ��) (�ڱ��ڽ� �� �÷����� ��� ���� Ȱ��)
		else if (platformbuf->end.type == 1 && platformbuf->start.type == 1) {
			SOCKADDR_IN destination;
			int wrongflag = 0;
			//�ڱ��ڽ��� ������ ��� �÷����� �����ͺ��̽� ������ �䱸�մϴ�.
			if (platformbuf->start.number == platformbuf->end.number) {
				MYSQL_RES* res;
				MYSQL_ROW row;

				if (mysql_query(conn, platformbuf->data) != 0) {
					{
						printf("������.");
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
			//�÷��� ���� ������ ����� �����մϴ�.
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
					printf("�÷����� �������� �ʴ� �����Դϴ�.");
					wrongflag = 1;
					break;
				}
				if (wrongflag) {
					printf("���� ���ο� ������ Ȯ����� ���� �����Դϴ�.\n");
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
		Ŭ���̾�Ʈ �� ����� private IP ������ ��ٷӴ�.
		�׷��� �ϴ��� �������� �ʾҴ�.
		*/
		else {
			printf("������ Ŭ���̾�Ʈ �� ����� ������ �ʽ��ϴ�. \n");
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
