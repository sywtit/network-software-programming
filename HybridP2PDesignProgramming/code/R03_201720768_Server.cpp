/* File Name: R03_201720768_Server
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: client의 아이디와 비밀번호 정보, 
 client의 IP, port number를 저장해서 이를 client과 TCP 통신을 이용해서 
 데이터 교환을 하는 프로그램
 * Last Changed: 2020-05-11
*/

#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include<string>
#include<cstring>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUFSIZE 1500
//concurrent server에 접속할 n개의 client 
//client뒤에 붙는 숫자는 최대 한자리 수 인 9까지 가능하기 때문에 max를 9로 지정한다.
#define client_max_size 9


//client 가 등록할 id, IP,  portnumber 구조체 배열선언
typedef struct clientInfo {
	char id[19];
	ULONG IP;
	USHORT portnumber;
};

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
	printf("Error [%s] ... program terminated \n", msg);
	exit(-1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
	printf("socket function error [%s]\n", msg);
}

clientInfo* clientInfos;
//client의 id와 비밀번호를 저장한 데이터 베이스
char*** client_identity = new char** [client_max_size];

//TCPserver가 concurrent한 방식으로 동작하게 해주는 thread함수
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET				client_sock = (SOCKET)arg;
	char				buf[BUFSIZE + 1];
	SOCKADDR_IN			clientaddr;
	int					addrlen;
	int					retval;
	int		            msglen;
	int retval_send = -1;
	// 2번 option에서 찾으려는 client에 해당하는 client다음의 특수 숫자
	int option_number = 0;


	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	int id = 0;
	//ID 혹은 password 가 틀리면 다시 보내는 while문 생성
	while (1) {

		//ID를 물어보는 syntax 보내기
		strcpy(buf, "Hi! Insert your ID : ");
		buf[22] = '\n';
		retval_send = send(client_sock, (char*)buf, 22, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		// ID 받기
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';

		//ID가 일치하는것이 있는지 확인 + 일치하지 않으면 다시 ID 물어봄
		if (strncmp(buf, "client", 6) == 0 && (int)buf[6] - '0' >= 1
			&& (int)(buf[6] - '0') <= client_max_size
			&& strncmp(&buf[7], "@ajou.ac.kr", 11) == 0
			&& msglen == 19) break;
		else continue;

	}

	//다음 option들을 물어봐서 각각의 데이터를 저장할 배열의 위치를 가르키는 변수
	id = (int)buf[6] - (int)'0';

	//password 를 물어보는 syntax 보내기
	while (1) {

		strcpy(buf, "ID confirmed, Insert your PASSWORD : ");
		buf[38] = '\n';
		retval_send = send(client_sock, (char*)buf, 38, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		// password 받기
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';

		//password가 일치하는지 확인 + 일치하지 않으면 다시 password 물어봄
		if (strncmp(buf, "easternmossy", 12) == 0 && (int)buf[12] - '0' == id) break;
		else continue;
	}

	//위 id 와 password 를 확인하는 절차를 거치면
	//다음 해당하는 option number 를 물어보는 단계
	//1. registration
	//2. query client
	//3. quit
	while (option_number != 3) {
		//option number 를 보내는 과정
		strcpy(buf, "(1=registration, 2=query client 3=quit) option number? : ");
		buf[58] = '\n';
		retval_send = send(client_sock, (char*)buf, 58, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}

		//option number 를 받아옴
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';
		option_number = (int)buf[0] - (int)'0';

		//1,2,3에 해당하지 않는 숫자를 client가 입력하면 다시 option number를 물어봄
		//3번을 선택하면 while문 을 나옴
		if (option_number != 1 && option_number != 2 && option_number != 3) continue;
		if (option_number == 3)break;

		//option number를 1,2를 선택하는 경우
		switch (option_number) {
		case 1:
		{
			while (1) {
				//등록할 client의 정보를 물어본다.
				strcpy(buf, "insert your id, IP, portnumber please ");
				buf[39] = '\n';
				retval_send = send(client_sock, (char*)buf, 39, 0);
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}
				//id, IP, portnumber 받아오기
				msglen = recv(client_sock, buf, BUFSIZE, 0);
				if (msglen == SOCKET_ERROR) {
					err_display((char*)"recv()");
					break;
				}
				else if (msglen == 0)
					break;

				buf[msglen] = '\0';

				//나중에 다른 client가 query 할 경우 돌려주기 위해서 구조체 배열에 저장
				memcpy(&clientInfos[id], buf, sizeof clientInfos[id]);


				//first client Info echo and 
				//and send confirm message
				retval_send = send(client_sock, (char*)&clientInfos[id], msglen, 0);
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}

				char* echo_confirm_message = (char*)malloc(sizeof(char) * (32));
				strncpy(echo_confirm_message, "and will you confirm? yes or no", 32);
				retval_send = send(client_sock, echo_confirm_message, 32, 0);
				
			
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}
				//confirm 에 대한 message 가 yes 이면 다시 option number 물어보는곳으로
				//confirm 에 대한 message 가 no 이면 다시 client 등록 정보를 물어보는 send 로 이동
				msglen = recv(client_sock, buf, BUFSIZE, 0);
				if (msglen == SOCKET_ERROR) {
					err_display((char*)"recv()");
					break;
				}
				else if (msglen == 0)
					break;

				buf[msglen] = '\0';

				if (strncmp(buf, "yes", 3) == 0)break;
				else if (strncmp(buf, "no", 2) == 0) continue;
			}
			break;
		}
		case 2:
		{
			//찾으려는 client의 ID를 물어보는 메시지 전송
			strcpy(buf, "tell me the client ID that you are finding");
			buf[43] = '\n';
			retval_send = send(client_sock, (char*)buf, 43, 0);
			if (retval_send == SOCKET_ERROR) {
				err_display((char*)"send()");
				break;
			}
			//client ID를 받는 부분
			msglen = recv(client_sock, buf, BUFSIZE, 0);
			if (msglen == SOCKET_ERROR) {
				err_display((char*)"recv()");
				break;
			}
			else if (msglen == 0) break;
			buf[msglen] = '\0';
			int client_num = (int)(buf[6]) - (int)'0';

			if (strncmp(clientInfos[client_num].id, buf, msglen) != 0 || msglen != 19 ||
				strncmp(buf, "client", 6) != 0 || strncmp(&buf[7], "@ajou.ac.kr", 11) != 0) {
				//해당하는 clientID가 없으므로 다시 optionnumber 로 넘어간다.
				strcpy(buf, "id do not exist");
				buf[16] = '\n';
				retval_send = send(client_sock, buf, 16, 0);
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}
				break;
			}
			else {
				//해당하는 client 정보를 보내준다.
				retval_send = send(client_sock, (char*)&clientInfos[client_num], 26, 0);
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}

				break;
			}
			break;
		}
		default:
			printf("option error\n");
		}

		option_number = 0;

	}

	//closesocket() //client socket 를 종료
	closesocket(client_sock);
	printf("[TCP Server] connection terminated: client (%s:%d)\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	return 0;
}


int main(int argc, char* argv[])
{

	// 데이터 통신에 사용할 변수
	SOCKET listen_sock, client_sock;
	SOCKADDR_IN serveraddr, clientaddr;
	int		addrlen;
	char	buf[BUFSIZE + 1];
	int retval;
	HANDLE			hThread;
	DWORD			ThreadId;


	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() //listen socket 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"listen_sock()");

	// server address 
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind() //전체 interface 와의 bind
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

	// listen() 
	retval = listen(listen_sock, SOMAXCONN); //SOMAXCONN
	if (retval == SOCKET_ERROR) err_quit((char*)"listen()");

	//client 정보 저장할 구조체 배열 선언
	clientInfos = (clientInfo*)malloc(sizeof(clientInfo) * client_max_size);
	for (int i = 0; i < client_max_size; i++) {
		memset(&clientInfos[i], 0, 26);
	}


	//id와 password 데이터 저장
	for (int i = 0; i < client_max_size; i++) {
		client_identity[i] = new char* [2];
		client_identity[i][0] = new char[7];
		client_identity[i][1] = new char[13];
		memset(client_identity[i][0], 0, sizeof(char) * 7);
		memset(client_identity[i][1], 0, sizeof(char) * 13);
	}
	
	int client_identity_exist[client_max_size] = { 0, };

	for (int i = 0; i < client_max_size; i++) {

		strncat(client_identity[i][0], "client", 6);
		char number = (i + 1) + '0';
		client_identity[i][0][6] = number;
		client_identity[i][0][7] = '\0';

		strncat(client_identity[i][1], "easternmossy", 12);
		client_identity[i][1][12] = number;
		client_identity[i][1][13] = '\0';

	}

	while (1) {
		// accept() //client socket 생성
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display((char*)"accept()");
			continue;
		}
		//tcp conncetion accomplished
		printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		printf("\nAnd this is new socket value: %d\n", client_sock);

		//TCP client와 concurrent하게 정보 교환을 하게할 thread생성
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, &ThreadId);
		if (hThread == NULL)
			err_display((char*)"error: failure of thread creation!!!");
		else
			CloseHandle(hThread);

	}

	// closesocket() //listen socket 를 종료
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}