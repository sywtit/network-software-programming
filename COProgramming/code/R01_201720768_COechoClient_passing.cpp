/* File Name: R01_201720768_COechoClient_passing.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CO socket programming �������� client ��
 * ���� ������ �����͸� ������ echo message �� �޴� ���α׷�
 * �Է� ������ :network byte order�� ���� �й�(201720768),�̸� ���� ����(13)�ΰ��� ����
 * �̸�: kim soo young ���ڿ�: �Է��� ��
 * Last Changed: 2020-04-01
*/
#include<WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define BUFSIZE 1500
#define INPUTSIZE 1000 //���Ƿ� ������ �Է� ���ڿ��� ũ��

//client �� �����ϴ� ����ü ������ syntax
typedef struct sendStructure {
	ULONG studentNum;
	USHORT countOfName;
	char name[14];
	char input[INPUTSIZE];
};

//server ���� ���� ����ü ������ syntax
typedef struct sendedStructure {
	int inputNum;
	char input[INPUTSIZE];
};
// ���� �Լ� ���� ��� �� ����
void err_quit(char* msg)
{
	printf("Error [%s] ... program terminated \n", msg);
	exit(-1);
}

// ���� �Լ� ���� ���
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
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() socket ����
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// server address
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// connect() server ���� connect 
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"connect()");

	// ������ ������ ���
	while (1) {

		// ������ �Է�
		//message �̸��� client�� ������ ����ü ����
		//�й� //4byte ũ���� network byte order �� ����
		//�̸� ���� //2byte ũ���� network byte order �� ����
		//�̸�
		struct sendStructure message;
		memset(&message, 0, sizeof message);
		message.studentNum = htonl(201720768);
		message.countOfName = htons(13);
		strncpy(message.name, "kim soo young", 13);
		int message_len = 20;


		//���� ���ڿ� ������
		ZeroMemory(buf, sizeof(buf));
		printf("\n[���� ������] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' ���� ����
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0) {
			continue;
		}

		//��ü ���� byte ũ��
		message_len += strlen(buf);

		//���� ������ �� ������ ����ü�� �ֱ�
		memset(message.input, 0, strlen(buf));
		memcpy(message.input, buf, strlen(buf));

		// ��ü ���� byte ��ŭ ������ ������
		retval = send(sock, (char*)&message, message_len, 0);

		if (retval == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", retval);

		ZeroMemory(buf, sizeof(buf));
		// echo message ������ �ޱ�
		retval = recv(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (retval == 0)
			break;

		buf[retval] = '\0';
		// server ���� ���� �����͸� ������ ����ü ����
		sendedStructure recvMessage;
		memcpy(&recvMessage, buf, sizeof(recvMessage));

		// ���� ������ ���
		int a = 0; //client ���� echo message ����� quit �� socket ���Ḧ �����ִ� ����
		a = printf("[TCP Ŭ���̾�Ʈ] %d bytes echoed, (%s)\n", recvMessage.inputNum, recvMessage.input);

		//���� ���ڿ��� quit �϶� echo message ����� client server ������ ��ȯ loop���� break
		if (strncmp(recvMessage.input, "QUIT", 4) == 0 && recvMessage.inputNum == 4 && a != 0) break;

	}

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}