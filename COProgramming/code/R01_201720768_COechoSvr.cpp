/* File Name: R01_201720768_COechoSvr
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CO socket programming �������� server ��
 * ���� ������ �������� �ް� echo message �� ������ ���α׷�
 * Last Changed: 2020-04-01
*/

#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include<string>
#include<cstring>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUFSIZE 1500
#define INPUTSIZE 1000 //client ���� ������ �Է� ���ڿ� �� ũ�� client �ҽ��� �Ȱ��� ����

//client���� ���� ����ü ������ syntax
typedef struct sendedStructure {
	ULONG studentNum;
	USHORT countOfName;
	char name[14];
	char input[INPUTSIZE];
};

//server ���� ������ ����ü ������ syntax
typedef struct sendStructure {
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
	// ������ ��ſ� ����� ����
	SOCKET listen_sock;
	SOCKET client_sock;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	int		addrlen;
	char	buf[BUFSIZE + 1];
	int		retval, msglen;
	//���ڿ��� 'QUIT'�϶� echo message �� ������ socket ���Ḧ �ϴ°��� ������ ���� ����
	int retval_send = -1;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() //listen socket ����
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"Hello World");

	// server address 
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind() //��ü interface ���� bind
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

	// listen() 
	retval = listen(listen_sock, SOMAXCONN); //SOMAXCONN
	if (retval == SOCKET_ERROR) err_quit((char*)"listen()");


	while (1) {
		// accept() //client socket ����
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display((char*)"accept()");
			continue;
		}

		printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		printf("\nAnd this is new socket value: %d\n", client_sock);

		// Ŭ���̾�Ʈ�� ������ ���
		while (1) {

			// ������ �ޱ�
			msglen = recv(client_sock, buf, BUFSIZE, 0);
			if (msglen == SOCKET_ERROR) {
				err_display((char*)"recv()");
				break;
			}
			else if (msglen == 0)
				break;

			buf[msglen] = '\0';

			//client �� ������ ����ü ������
			//client ���� ���� message buf�� ����
			sendedStructure recvMessage;
			memcpy(&recvMessage, buf, sizeof recvMessage);

			// ���� ������ ����ü parsing �� ���            
			printf("[TCP/%s:%d] : �й�: %d, �̸�: %s, �޼���: %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), ntohl(recvMessage.studentNum),
				recvMessage.name, recvMessage.input);


			//���� �����ͷ� ���ο� client�� ���� ����ü ����
			sendStructure sendMessage;
			memset(sendMessage.input, 0, strlen(sendMessage.input));
			memcpy(sendMessage.input, recvMessage.input, strlen(recvMessage.input));
			sendMessage.inputNum = strlen(recvMessage.input);

			// client �� echo message�� ���� ���ڿ��� ���̸�ŭ ������ ������
			retval_send = send(client_sock, (char*)&sendMessage, 2 + msglen - 19 + 1, 0);
			if (retval_send == SOCKET_ERROR) {
				err_display((char*)"send()");
				break;
			}

			// client ���� ���� ���ڿ��� QUIT �϶� echo message �� ������ 
			//client ���� ������ ��ȯ�� �Ͼ�� loop ���� break
			if ((strncmp(recvMessage.input, "QUIT", 4) == 0) && sendMessage.inputNum == 4 &&
				retval_send != -1) break;
		}

		//closesocket() //client socket �� ����
		closesocket(client_sock);
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	// closesocket() //listen socket �� ����
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}