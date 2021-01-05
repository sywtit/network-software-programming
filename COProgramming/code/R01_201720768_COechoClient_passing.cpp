/* File Name: R01_201720768_COechoClient_passing.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CO socket programming 과정에서 client 가
 * 여러 형식의 데이터를 보내고 echo message 를 받는 프로그램
 * 입력 데이터 :network byte order에 따른 학번(201720768),이름 문자 개수(13)두개의 값과
 * 이름: kim soo young 문자열: 입력한 값
 * Last Changed: 2020-04-01
*/
#include<WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define BUFSIZE 1500
#define INPUTSIZE 1000 //임의로 지정한 입력 문자열의 크기

//client 가 전송하는 구조체 형식의 syntax
typedef struct sendStructure {
	ULONG studentNum;
	USHORT countOfName;
	char name[14];
	char input[INPUTSIZE];
};

//server 에서 받을 구조체 형식의 syntax
typedef struct sendedStructure {
	int inputNum;
	char input[INPUTSIZE];
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


int main(int argc, char* argv[])
{
	int retval;
	SOCKET sock;
	SOCKADDR_IN serveraddr;
	char buf[BUFSIZE + 1];
	int len;
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() socket 생성
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// server address
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// connect() server 와의 connect 
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"connect()");

	// 서버와 데이터 통신
	while (1) {

		// 데이터 입력
		//message 이름의 client가 전송할 구조체 선언
		//학번 //4byte 크기의 network byte order 로 저장
		//이름 길이 //2byte 크기의 network byte order 로 저장
		//이름
		struct sendStructure message;
		memset(&message, 0, sizeof message);
		message.studentNum = htonl(201720768);
		message.countOfName = htons(13);
		strncpy(message.name, "kim soo young", 13);
		int message_len = 20;


		//보낼 문자열 데이터
		ZeroMemory(buf, sizeof(buf));
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0) {
			continue;
		}

		//전체 보낼 byte 크기
		message_len += strlen(buf);

		//보낸 데이터 값 생성한 구조체에 넣기
		memset(message.input, 0, strlen(buf));
		memcpy(message.input, buf, strlen(buf));

		// 전체 보낼 byte 만큼 데이터 보내기
		retval = send(sock, (char*)&message, message_len, 0);

		if (retval == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		printf("[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);

		ZeroMemory(buf, sizeof(buf));
		// echo message 데이터 받기
		retval = recv(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (retval == 0)
			break;

		buf[retval] = '\0';
		// server 에서 받은 데이터를 저장할 구조체 선언
		sendedStructure recvMessage;
		memcpy(&recvMessage, buf, sizeof(recvMessage));

		// 받은 데이터 출력
		int a = 0; //client 에서 echo message 출력후 quit 시 socket 종료를 도와주는 변수
		a = printf("[TCP 클라이언트] %d bytes echoed, (%s)\n", recvMessage.inputNum, recvMessage.input);

		//보낸 문자열이 quit 일때 echo message 출력후 client server 데이터 교환 loop에서 break
		if (strncmp(recvMessage.input, "QUIT", 4) == 0 && recvMessage.inputNum == 4 && a != 0) break;

	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}