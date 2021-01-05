#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>
#include "mysql.h"
#pragma comment(lib, "libmySQL.lib")

#define BUFSIZE 1024
#define PORT 8400

//데이터 베이스 접근 정보
const char* server = "localhost";
const char* user = "root";
const char* password = "3681";

/* 제너럴 구조체
* 이 구조체의 flag 값은 서버가 어떤 루틴을 처리해야 하는지 알 수 있다.
* flag = 1; 로그인
* flag = 2; 회원 가입
* flag = 3; 주문 정보 요청
* flag = 4; 광고 정보 요청
* flag = 5; 배달 과정 정보 요청
* flag = 6; 해당 주문 승락
* flag = 7; 광고 정보 승락
* flag = 8; 특정 주문 상태 변화
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

//회원 가입 및 로그인 시 필요한 프로토콜
typedef struct adminprotocol {
	Destination start;
	Destination end;
	int flag;
	char id[40];		//회원 가입 이름
	char name[40];		//아이디.   실패시 원인을 여기다 담음
	char password[40];	//비밀번호
	int result;		// 서버 쪽 응답. (숫자면 ok 0이면 실패)
}AdminProtocol;

//리스트 안에 들어가는 구조체
typedef struct StoreInfo {
	int no;
	char clientaddress[40];
	char storeaddress[40];
	char storename[30];
	char menuname[30];
	char date[20];
}StoreInfo;

//리스트 요청시 필요한 프로토콜
typedef struct ListProtocol {
	Destination start;
	Destination end;
	int flag;
	int userid;
	int numofstore;
	StoreInfo info[5];
}ListProtocol;

//주문 승락시 필요한 프로토콜
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
* 로그인 하는 함수.
* 데이터 베이스에 접근하여 select 문으로 _no 값을 얻는다.
* AdminProtocol의 result에 _no 값을 담거나 없으면 0를 담는다.
*/
void loginprocess(AdminProtocol* loginbuf, AdminProtocol* response, MYSQL* conn) {
	MYSQL_RES* res;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "select _no from rider where id=\"%s\" and password=\"%s\"", loginbuf->id, loginbuf->password);
	if (mysql_query(conn, query) != 0) {
		{
			printf("오류다.");
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
		strncpy(response->name, "해당 유저가 없어요", sizeof(char) * 40);
	}
	mysql_free_result(res);

	response->start.type = 1;
	response->end.type = 0;
	response->start.number = 2;
	response->end.addr = loginbuf->start.addr;
}

/*
* 회원 가입 하는 함수
* 로그인하고 다른 점은 select 문을 통해 결과가 없을 시에
* insert 문을 실행시킨다는 점이다.
*/
void registerprocess(AdminProtocol* registerbuf, AdminProtocol* registerresult, MYSQL* conn) {
	MYSQL_RES* res;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "select _no from rider where id=\"%s\" and password=\"%s\"", registerbuf->id, registerbuf->password);
	if (mysql_query(conn, query) != 0) {
		{
			printf("오류다.");
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
		strncpy(registerresult->name, "회원이 존재합니다.", sizeof(char) * 40);
	}
	else {
		char query[200];

		sprintf(query, "insert into rider(id, password, ridername) values (\'%s\', \'%s\', \'%s\');",
			registerbuf->id, registerbuf->password, registerbuf->name);

		mysql_query(conn, "set names euckr");
		if (mysql_query(conn, query) != 0) {
			registerresult->result = 0;
			strncpy(registerresult->name, "생성 실패 다시해주세요", sizeof(char) * 40);
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
* 클라이언트의 요청에 따라 리스트를 불러오는 함수
* ListProtocol의 규격에 따라 StoreInfo에 select 문의 결과가 쌓인다.
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
		//라이더의 상태가 0 (아무도 매칭 안함)인 상점의 주소, 상점 이름, 메뉴 이름, 손님 주소, 오더링 _no를 구한다.
		sprintf(query, "select o._no, o.storename, o.menuname, o.clientaddress, s.address, date_format(time_stamp, \'%%H-%%m\') from ordering as o join store as s on o.storename = s.storename where riderstatus = 0 limit 6");
		mysql_query(conn, query);
		res = mysql_store_result(conn);
		break;
	case 4:
		//라이더 이름이 없는 광고의 이름과 _no를 구한다.
		sprintf(query, "select _no, storename from advertisement where ridername is null limit 6");
		mysql_query(conn, query);
		res = mysql_store_result(conn);
		break;
	case 5:
		//라이더의 상태가 1(주문 접수, 주문 완료 전), 상점의 상태가 1 (픽업 대기 완료)인 오더링 정보를 구한다.
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
	정보 주입 중이다.
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
* 클라이언트로 부터 선택받은 ordering에 적절한 처리를 해주는 함수.
* 라이더의 주문 완료, 라이더의 주문 예약, 라이더의 상점 광고 매칭이 있다.
* 간략한 update 문으로 이를 해결하였다.
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

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	// 클라이언트와 데이터 통신
	while (1) {
		//로그인 신호를 받는다.
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
			//로그인 요청을 수행하는 부분
		case 1:
			adminresponse = (AdminProtocol*)malloc(sizeof(AdminProtocol));
			loginprocess((AdminProtocol*)buf, (AdminProtocol*)adminresponse, conn);
			// 결과를 보낸다.
			retval = sendto(sock, (char*)adminresponse, sizeof(AdminProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}

			break;
			//회원 가입 요청을 수행하는 부분
		case 2:
			adminresponse = (AdminProtocol*)malloc(sizeof(AdminProtocol));
			registerprocess((AdminProtocol*)buf, (AdminProtocol*)adminresponse, conn);
			// 결과를 보낸다.
			retval = sendto(sock, (char*)adminresponse, sizeof(AdminProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			break;

			// 리스트를 불러오기 위한 요청을 수행하는 부분
		case 3: case 4: case 5:
			listresponse = (ListProtocol*)malloc(sizeof(ListProtocol));
			listprocess((ListProtocol*)buf, (ListProtocol*)listresponse, conn);
			// 결과를 보낸다.
			retval = sendto(sock, (char*)listresponse, sizeof(ListProtocol), 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			break;

			// 해당 오더링(또는 광고)의 상태를 바꾸기 위한 부분
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

	// 윈속 종료
	WSACleanup();
	return 0;
}