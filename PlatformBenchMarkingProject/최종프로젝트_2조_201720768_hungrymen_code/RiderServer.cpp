#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>
#include "mysql.h"
#pragma comment(lib, "libmySQL.lib")

#define BUFSIZE 1024
#define PORT 8400

//������ ���̽� ���� ����
const char* server = "localhost";
const char* user = "root";
const char* password = "3681";

/* ���ʷ� ����ü
* �� ����ü�� flag ���� ������ � ��ƾ�� ó���ؾ� �ϴ��� �� �� �ִ�.
* flag = 1; �α���
* flag = 2; ȸ�� ����
* flag = 3; �ֹ� ���� ��û
* flag = 4; ���� ���� ��û
* flag = 5; ��� ���� ���� ��û
* flag = 6; �ش� �ֹ� �¶�
* flag = 7; ���� ���� �¶�
* flag = 8; Ư�� �ֹ� ���� ��ȭ
*/
typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;


typedef struct flagprotocol {
	Destination start;
	Destination end;
	int flag;
	char buffer[BUFSIZE];
}FlagProtocol;

//ȸ�� ���� �� �α��� �� �ʿ��� ��������
typedef struct adminprotocol {
	Destination start;
	Destination end;
	int flag;
	char id[40];		//ȸ�� ���� �̸�
	char name[40];		//���̵�.   ���н� ������ ����� ����
	char password[40];	//��й�ȣ
	int result;		// ���� �� ����. (���ڸ� ok 0�̸� ����)
}AdminProtocol;

//����Ʈ �ȿ� ���� ����ü
typedef struct StoreInfo {
	int no;
	char clientaddress[40];
	char storeaddress[40];
	char storename[30];
	char menuname[30];
	char date[20];
}StoreInfo;

//����Ʈ ��û�� �ʿ��� ��������
typedef struct ListProtocol {
	Destination start;
	Destination end;
	int flag;
	int userid;
	int numofstore;
	StoreInfo info[5];
}ListProtocol;

//�ֹ� �¶��� �ʿ��� ��������
typedef struct AcceptProtocol {
	Destination start;
	Destination end;
	int flag;
	int itemid;
	int userid;
	int result;
}AcceptProtocol;

void err_quit(char* msg)
{
	printf("%s\n", msg);
}

void err_display(char* msg)
{
	printf("%s\n", msg);
}

/*
* �α��� �ϴ� �Լ�.
* ������ ���̽��� �����Ͽ� select ������ _no ���� ��´�.
* AdminProtocol�� result�� _no ���� ��ų� ������ 0�� ��´�.
*/
void loginprocess(AdminProtocol* loginbuf, AdminProtocol* response, MYSQL* conn) {
	MYSQL_RES* res;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "select _no from rider where id=\"%s\" and password=\"%s\"", loginbuf->id, loginbuf->password);
	if (mysql_query(conn, query) != 0) {
		{
			printf("������.");
		}
	}
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	int user_no = 0;
	if (row == NULL) {
		user_no = 0;
	}
	else {
		user_no = atoi(row[0]);
	}

	memset(response, 0x00, sizeof(response));
	(response)->flag = 1;
	if (user_no != 0) {
		(response)->result = user_no;
	}
	else {
		(response)->result = 0;
		strncpy(response->name, "�ش� ������ �����", sizeof(char) * 40);
	}
	mysql_free_result(res);

	response->start.type = 1;
	response->end.type = 0;
	response->start.number = 2;
	response->end.addr = loginbuf->start.addr;
}

/*
* ȸ�� ���� �ϴ� �Լ�
* �α����ϰ� �ٸ� ���� select ���� ���� ����� ���� �ÿ�
* insert ���� �����Ų�ٴ� ���̴�.
*/
void registerprocess(AdminProtocol* registerbuf, AdminProtocol* registerresult, MYSQL* conn) {
	MYSQL_RES* res;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "select _no from rider where id=\"%s\" and password=\"%s\"", registerbuf->id, registerbuf->password);
	if (mysql_query(conn, query) != 0) {
		{
			printf("������.");
		}
	}

	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	int user_no = 0;
	if (row == NULL) {
		user_no = 0;
	}
	else {
		user_no = atoi(row[0]);
	}

	registerresult->flag = 2;
	if (user_no != 0) {
		registerresult->result = 0;
		strncpy(registerresult->name, "ȸ���� �����մϴ�.", sizeof(char) * 40);
	}
	else {
		char query[200];

		sprintf(query, "insert into rider(id, password, ridername) values (\'%s\', \'%s\', \'%s\');",
			registerbuf->id, registerbuf->password, registerbuf->name);

		mysql_query(conn, "set names euckr");
		if (mysql_query(conn, query) != 0) {
			registerresult->result = 0;
			strncpy(registerresult->name, "���� ���� �ٽ����ּ���", sizeof(char) * 40);
		}
		else {
			registerresult->result = 1;
		}
	}
	mysql_free_result(res);

	registerresult->start.type = 1;
	registerresult->end.type = 0;
	registerresult->start.number = 2;
	registerresult->end.addr = registerbuf->start.addr;
}

/*
* Ŭ���̾�Ʈ�� ��û�� ���� ����Ʈ�� �ҷ����� �Լ�
* ListProtocol�� �԰ݿ� ���� StoreInfo�� select ���� ����� ���δ�.
*/
void listprocess(ListProtocol* listbuf, ListProtocol* listresult, MYSQL* conn) {
	int flag = listbuf->flag;
	char name[50];
	char query[500];
	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	switch (flag) {
	case 3:
		mysql_query(conn, "set names euckr");
		//���̴��� ���°� 0 (�ƹ��� ��Ī ����)�� ������ �ּ�, ���� �̸�, �޴� �̸�, �մ� �ּ�, ������ _no�� ���Ѵ�.
		sprintf(query, "select o._no, o.storename, o.menuname, o.clientaddress, s.address, date_format(time_stamp, \'%%H-%%m\') from ordering as o join store as s on o.storename = s.storename where riderstatus = 0 limit 6");
		mysql_query(conn, query);
		res = mysql_store_result(conn);
		break;
	case 4:
		//���̴� �̸��� ���� ������ �̸��� _no�� ���Ѵ�.
		sprintf(query, "select _no, storename from advertisement where ridername is null limit 6");
		mysql_query(conn, query);
		res = mysql_store_result(conn);
		break;
	case 5:
		//���̴��� ���°� 1(�ֹ� ����, �ֹ� �Ϸ� ��), ������ ���°� 1 (�Ⱦ� ��� �Ϸ�)�� ������ ������ ���Ѵ�.
		mysql_query(conn, "set names euckr");
		sprintf(query, "select o._no, o.storename, o.menuname, o.clientaddress, s.address, date_format(time_stamp, \'%%H-%%m\') from ordering as o join store as s on o.storename = s.storename where riderstatus = 1 and storestatus = 1 and ridername = (select ridername from rider where _no = %d) limit 6", listbuf->userid);
		if (mysql_query(conn, query) != 0)
		{
			err_display((char*)"error when selecting the ordering info");
		}
		mysql_query(conn, query);
		res = mysql_store_result(conn);
		break;
	}

	/*
	���� ���� ���̴�.
	*/
	listresult->numofstore = 0;
	while (row = mysql_fetch_row(res)) {
		listresult->info[listresult->numofstore].no = atoi(row[0]);
		strcpy(listresult->info[listresult->numofstore].storename, row[1]);
		if (listbuf->flag != 4) {
			strcpy(listresult->info[listresult->numofstore].menuname, row[2]);
			strcpy(listresult->info[listresult->numofstore].clientaddress, row[3]);
			strcpy(listresult->info[listresult->numofstore].storeaddress, row[4]);
			strcpy(listresult->info[listresult->numofstore].date, row[5]);
		}
		listresult->numofstore++;
	}
	mysql_free_result(res);

	listresult->start.type = 1;
	listresult->end.type = 0;
	listresult->start.number = 2;
	listresult->end.addr = listbuf->start.addr;
}

/*
* Ŭ���̾�Ʈ�� ���� ���ù��� ordering�� ������ ó���� ���ִ� �Լ�.
* ���̴��� �ֹ� �Ϸ�, ���̴��� �ֹ� ����, ���̴��� ���� ���� ��Ī�� �ִ�.
* ������ update ������ �̸� �ذ��Ͽ���.
*/
void acceptprocess(AcceptProtocol* acceptbuf, AcceptProtocol* response, MYSQL* conn) {
	int flag = acceptbuf->flag;
	char query[200];
	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	sprintf(query, "select ridername from rider where _no = %d", acceptbuf->userid);
	mysql_query(conn, query);
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	char ridername[40];
	strcpy(ridername, row[0]);
	mysql_free_result(res);

	int num_rows = 0;
	switch (flag) {
	case 6:
		mysql_query(conn, "set names euckr");
		sprintf(query, "update ordering set riderstatus = 1, ridername = \"%s\" where riderstatus = 0 and _no = %d", ridername, acceptbuf->itemid);
		mysql_query(conn, query);
		num_rows = mysql_affected_rows(conn);
		break;
	case 7:
		mysql_query(conn, "set names euckr");
		sprintf(query, "update advertisement set ridername = \"%s\" where ridername is null and _no = %d", ridername, acceptbuf->itemid);
		mysql_query(conn, query);
		num_rows = mysql_affected_rows(conn);
		break;
	case 8:

		sprintf(query, "update ordering set riderstatus = 2, storestatus = 2 where riderstatus = 1 and storestatus = 1 and _no = %d", acceptbuf->itemid);
		mysql_query(conn, query);
		num_rows = mysql_affected_rows(conn);
		break;
	}
	response->result = num_rows;
	response->start.type = 1;
	response->end.type = 0;
	response->start.number = 2;
	response->end.addr = acceptbuf->start.addr;
}

int main(int argc, char* argv[])
{
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

	// ������ ��ſ� ����� ����
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	// Ŭ���̾�Ʈ�� ������ ���
	while (1) {
		//�α��� ��ȣ�� �޴´�.
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
			continue;
		}

		FlagProtocol* myflag = (FlagProtocol*)buf;
		char* response = NULL;
		AdminProtocol* adminresponse;
		ListProtocol* listresponse;
		AcceptProtocol* acceptprotocol;

		switch (myflag->flag) {
			//�α��� ��û�� �����ϴ� �κ�
		case 1:
			adminresponse = (AdminProtocol*)malloc(sizeof(AdminProtocol));
			loginprocess((AdminProtocol*)buf, (AdminProtocol*)adminresponse, conn);
			// ����� ������.
			retval = sendto(sock, (char*)adminresponse, sizeof(AdminProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}

			break;
			//ȸ�� ���� ��û�� �����ϴ� �κ�
		case 2:
			adminresponse = (AdminProtocol*)malloc(sizeof(AdminProtocol));
			registerprocess((AdminProtocol*)buf, (AdminProtocol*)adminresponse, conn);
			// ����� ������.
			retval = sendto(sock, (char*)adminresponse, sizeof(AdminProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			break;

			// ����Ʈ�� �ҷ����� ���� ��û�� �����ϴ� �κ�
		case 3: case 4: case 5:
			listresponse = (ListProtocol*)malloc(sizeof(ListProtocol));
			listprocess((ListProtocol*)buf, (ListProtocol*)listresponse, conn);
			// ����� ������.
			retval = sendto(sock, (char*)listresponse, sizeof(ListProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			break;

			// �ش� ������(�Ǵ� ����)�� ���¸� �ٲٱ� ���� �κ�
		case 6: case 7: case 8:
			acceptprotocol = (AcceptProtocol*)malloc(sizeof(AcceptProtocol));
			acceptprocess((AcceptProtocol*)buf, (AcceptProtocol*)acceptprotocol, conn);
			retval = sendto(sock, (char*)acceptprotocol, sizeof(ListProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			break;
		default:
			printf("wrong flag error\n");
			break;
		}

		free(response);
	}
	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}