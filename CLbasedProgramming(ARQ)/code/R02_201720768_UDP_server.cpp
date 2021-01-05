/* File Name: R02_201720768_UDP_server.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CL programming ���� server�� client �� ����ϸ鼭
 * client �� packet loss�� �̷������� �����Ͽ� discard �ϴ� ������ ���� ���α׷�
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

// ���� �Լ� ���� ���
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
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	fromMessage rcvMessage[100];
	toMessage sendMessage;
	int sNumber = 0;
	int discard_num = 1;

	while (1) {

		// ������ �ޱ�
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
			continue;
		}
		buf[retval] = '\0';

		//���� ������ server ���� ����ü�� ����
		memset(&rcvMessage[sNumber].input, 0, INPUTSIZE);
		memcpy(&rcvMessage[sNumber], buf, retval);
		// ���� ������ ���
		printf("\n[UDP/%s:%d]\n sequence number : %d\nInput Message :  %s\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), rcvMessage[sNumber].sequence_num, rcvMessage[sNumber].input);

		//0~1������ random �� ����
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 9);

		float random = (float)dis(gen) / (float)10;

		//random ���� p ���� �۰ų� ������ message �� ack ����
		if (random <= p) {
			discard_num = 1;

			//������ ������
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
			//client �� �������� ������ ������ �޼��� ������ 100���϶� server ����
			if (sendAmount == 100)break;
		}
		//p���� random ���� ū��� = client ���� packet loss �� �Ͼ���
		else {
			printf("server discard %d times!\n", discard_num++);
			continue;
		}

	}

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}

