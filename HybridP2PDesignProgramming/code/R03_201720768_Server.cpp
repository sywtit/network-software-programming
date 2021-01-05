/* File Name: R03_201720768_Server
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: client�� ���̵�� ��й�ȣ ����, 
 client�� IP, port number�� �����ؼ� �̸� client�� TCP ����� �̿��ؼ� 
 ������ ��ȯ�� �ϴ� ���α׷�
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
//concurrent server�� ������ n���� client 
//client�ڿ� �ٴ� ���ڴ� �ִ� ���ڸ� �� �� 9���� �����ϱ� ������ max�� 9�� �����Ѵ�.
#define client_max_size 9


//client �� ����� id, IP,  portnumber ����ü �迭����
typedef struct clientInfo {
	char id[19];
	ULONG IP;
	USHORT portnumber;
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

clientInfo* clientInfos;
//client�� id�� ��й�ȣ�� ������ ������ ���̽�
char*** client_identity = new char** [client_max_size];

//TCPserver�� concurrent�� ������� �����ϰ� ���ִ� thread�Լ�
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET				client_sock = (SOCKET)arg;
	char				buf[BUFSIZE + 1];
	SOCKADDR_IN			clientaddr;
	int					addrlen;
	int					retval;
	int		            msglen;
	int retval_send = -1;
	// 2�� option���� ã������ client�� �ش��ϴ� client������ Ư�� ����
	int option_number = 0;


	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

	int id = 0;
	//ID Ȥ�� password �� Ʋ���� �ٽ� ������ while�� ����
	while (1) {

		//ID�� ����� syntax ������
		strcpy(buf, "Hi! Insert your ID : ");
		buf[22] = '\n';
		retval_send = send(client_sock, (char*)buf, 22, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		// ID �ޱ�
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';

		//ID�� ��ġ�ϴ°��� �ִ��� Ȯ�� + ��ġ���� ������ �ٽ� ID ���
		if (strncmp(buf, "client", 6) == 0 && (int)buf[6] - '0' >= 1
			&& (int)(buf[6] - '0') <= client_max_size
			&& strncmp(&buf[7], "@ajou.ac.kr", 11) == 0
			&& msglen == 19) break;
		else continue;

	}

	//���� option���� ������� ������ �����͸� ������ �迭�� ��ġ�� ����Ű�� ����
	id = (int)buf[6] - (int)'0';

	//password �� ����� syntax ������
	while (1) {

		strcpy(buf, "ID confirmed, Insert your PASSWORD : ");
		buf[38] = '\n';
		retval_send = send(client_sock, (char*)buf, 38, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}
		// password �ޱ�
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';

		//password�� ��ġ�ϴ��� Ȯ�� + ��ġ���� ������ �ٽ� password ���
		if (strncmp(buf, "easternmossy", 12) == 0 && (int)buf[12] - '0' == id) break;
		else continue;
	}

	//�� id �� password �� Ȯ���ϴ� ������ ��ġ��
	//���� �ش��ϴ� option number �� ����� �ܰ�
	//1. registration
	//2. query client
	//3. quit
	while (option_number != 3) {
		//option number �� ������ ����
		strcpy(buf, "(1=registration, 2=query client 3=quit) option number? : ");
		buf[58] = '\n';
		retval_send = send(client_sock, (char*)buf, 58, 0);
		if (retval_send == SOCKET_ERROR) {
			err_display((char*)"send()");
			break;
		}

		//option number �� �޾ƿ�
		msglen = recv(client_sock, buf, BUFSIZE, 0);
		if (msglen == SOCKET_ERROR) {
			err_display((char*)"recv()");
			break;
		}
		else if (msglen == 0)
			break;

		buf[msglen] = '\0';
		option_number = (int)buf[0] - (int)'0';

		//1,2,3�� �ش����� �ʴ� ���ڸ� client�� �Է��ϸ� �ٽ� option number�� ���
		//3���� �����ϸ� while�� �� ����
		if (option_number != 1 && option_number != 2 && option_number != 3) continue;
		if (option_number == 3)break;

		//option number�� 1,2�� �����ϴ� ���
		switch (option_number) {
		case 1:
		{
			while (1) {
				//����� client�� ������ �����.
				strcpy(buf, "insert your id, IP, portnumber please ");
				buf[39] = '\n';
				retval_send = send(client_sock, (char*)buf, 39, 0);
				if (retval_send == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}
				//id, IP, portnumber �޾ƿ���
				msglen = recv(client_sock, buf, BUFSIZE, 0);
				if (msglen == SOCKET_ERROR) {
					err_display((char*)"recv()");
					break;
				}
				else if (msglen == 0)
					break;

				buf[msglen] = '\0';

				//���߿� �ٸ� client�� query �� ��� �����ֱ� ���ؼ� ����ü �迭�� ����
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
				//confirm �� ���� message �� yes �̸� �ٽ� option number ����°�����
				//confirm �� ���� message �� no �̸� �ٽ� client ��� ������ ����� send �� �̵�
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
			//ã������ client�� ID�� ����� �޽��� ����
			strcpy(buf, "tell me the client ID that you are finding");
			buf[43] = '\n';
			retval_send = send(client_sock, (char*)buf, 43, 0);
			if (retval_send == SOCKET_ERROR) {
				err_display((char*)"send()");
				break;
			}
			//client ID�� �޴� �κ�
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
				//�ش��ϴ� clientID�� �����Ƿ� �ٽ� optionnumber �� �Ѿ��.
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
				//�ش��ϴ� client ������ �����ش�.
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

	//closesocket() //client socket �� ����
	closesocket(client_sock);
	printf("[TCP Server] connection terminated: client (%s:%d)\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	return 0;
}


int main(int argc, char* argv[])
{

	// ������ ��ſ� ����� ����
	SOCKET listen_sock, client_sock;
	SOCKADDR_IN serveraddr, clientaddr;
	int		addrlen;
	char	buf[BUFSIZE + 1];
	int retval;
	HANDLE			hThread;
	DWORD			ThreadId;


	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() //listen socket ����
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"listen_sock()");

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

	//client ���� ������ ����ü �迭 ����
	clientInfos = (clientInfo*)malloc(sizeof(clientInfo) * client_max_size);
	for (int i = 0; i < client_max_size; i++) {
		memset(&clientInfos[i], 0, 26);
	}


	//id�� password ������ ����
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
		// accept() //client socket ����
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

		//TCP client�� concurrent�ϰ� ���� ��ȯ�� �ϰ��� thread����
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, &ThreadId);
		if (hThread == NULL)
			err_display((char*)"error: failure of thread creation!!!");
		else
			CloseHandle(hThread);

	}

	// closesocket() //listen socket �� ����
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}