/* File Name: R02_201720768_UDP_server.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CL programming 에서 server가 client 와 통신하면서
 * client 의 packet loss를 이론적으로 가정하여 discard 하는 과정을 담은 프로그램
 * Last Changed: 2020-04-19
*/

#include<stdio.h>
#include<winsock.h>
#include<stdlib.h>
#include<iostream>
#include<random>
#define BUFSIZE 1024
#define INPUTSIZE 512
#define p 0.7

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

typedef struct {
	int sequence_num;
	char input[INPUTSIZE];
}fromMessage;

typedef struct {
	int sequence_num;
	char input[INPUTSIZE];
}toMessage;

int main(int argc, char* argv[]) {
	int retval;
	int sendAmount = 0;
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
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	fromMessage rcvMessage[100];
	toMessage sendMessage;
	int sNumber = 0;
	int discard_num = 1;

	while (1) {

		// 데이터 받기
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
			continue;
		}
		buf[retval] = '\0';

		//받은 데이터 server 내의 구조체에 복사
		memset(&rcvMessage[sNumber].input, 0, INPUTSIZE);
		memcpy(&rcvMessage[sNumber], buf, retval);
		// 받은 데이터 출력
		printf("\n[UDP/%s:%d]\n sequence number : %d\nInput Message :  %s\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), rcvMessage[sNumber].sequence_num, rcvMessage[sNumber].input);

		//0~1까지의 random 값 생성
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 9);

		float random = (float)dis(gen) / (float)10;

		//random 값이 p 보다 작거나 같으면 message 를 ack 보냄
		if (random <= p) {
			discard_num = 1;

			//데이터 보내기
			memset(sendMessage.input, 0, INPUTSIZE);
			strncpy(sendMessage.input, rcvMessage[sNumber].input, strlen(rcvMessage[sNumber].input));
			sendMessage.sequence_num = rcvMessage[sNumber].sequence_num + retval - 4;
			retval = sendto(sock, (char*)&sendMessage, retval, 0,
				(SOCKADDR*)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			sNumber++;
			sendAmount++;
			//client 가 재전송을 제외한 전송한 메세지 개수가 100개일때 server 종료
			if (sendAmount == 100)break;
		}
		//p보다 random 값이 큰경우 = client 에서 packet loss 가 일어난경우
		else {
			printf("server discard %d times!\n", discard_num++);
			continue;
		}

	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

