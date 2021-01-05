/* File Name: R03_201720768_Client.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: TCPserver�� ����� �ؼ� ���ϴ� ������ ����ϰų� ������
 �ٸ� client���� UDP ����� �ϴ� ���α׷�
 * Last Changed: 2020-05-11
*/
#include<WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <string>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define BUFSIZE 1500
//thread���� �޾ƿ� udpServerPort
USHORT udpServerPort = 0;
int quit = 0;

//client �� ����� id, IP,  portnumber ����ü �迭����
typedef struct clientInfo {
	char id[19];
	ULONG IP;
	USHORT portnumber;
};

//server���� confirm ���� �޾ƿ� structure
typedef struct clientInfoRecv {
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

//UDPserver thread
DWORD WINAPI ProcessServer(LPVOID arg)
{
	char				buf[BUFSIZE + 1];
	SOCKADDR_IN			clientaddr;
	int					addrlen;
	int					retval;


	//UDP server socket ����
	SOCKET UDPserversock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDPserversock == INVALID_SOCKET) err_quit((char*)"socket()");

	//UDP server bind
	SOCKADDR_IN UDPserveraddr;
	ZeroMemory(&UDPserveraddr, sizeof(UDPserveraddr));
	UDPserveraddr.sin_family = AF_INET;
	//==========================
	//client�� �ش��ϴ� port number
	//===========================
	UDPserveraddr.sin_port = htons(1000);
	udpServerPort = htons(1000);
	UDPserveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(UDPserversock, (SOCKADDR*)&UDPserveraddr, sizeof(UDPserveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");


	//������ �ޱ�
	while (quit != 1) {

		addrlen = sizeof(clientaddr);
		retval = recvfrom(UDPserversock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
		}
		buf[retval] = '\0';

		// ���� ������ ���
		printf("\n[UDP/%s:%d]\nInput Message :  %s\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), buf);

		//echo message ������ ����
		retval = sendto(UDPserversock, buf, retval, 0,
			(SOCKADDR*)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display((char*)"sendto()");
			continue;
		}
		//else break;
	}
	//closeUDPsocket()
	closesocket(UDPserversock);
}

int main(int argc, char* argv[])
{
	int retval, msglen;
	SOCKET sock;
	SOCKADDR_IN serveraddr;
	//client ������ �´��� Ȯ���ϴ� �κ�
	SOCKADDR_IN checkClientAddr;
	int optnum = 0;
	char buf[BUFSIZE + 1];
	int len;
	//UDP ������ ��ſ� �ʿ��� ����
	SOCKADDR_IN clientaddr;
	SOCKADDR_IN peeraddr;
	HANDLE			hThread;
	DWORD			ThreadId;
	SOCKET			server_sock;
	int addrlen;
	//TCP�� ���ؼ� �ٸ� client������ �������
	int getInfo = 0;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	//udp client socket
	SOCKET UDPclientsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDPclientsock == INVALID_SOCKET) err_quit((char*)"socket()");


	// TCP socket() socket ����
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// server address
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	//====================
	//���� aws IP address
	//====================
	serveraddr.sin_addr.s_addr = inet_addr("15.164.232.220");

	// peer server address ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN peerServeraddr;
	ZeroMemory(&peerServeraddr, sizeof(peerServeraddr));
	peerServeraddr.sin_family = AF_INET;


	// connect() server ���� connect 
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"connect()");

	clientInfo client_Info;
	clientInfoRecv client_Info_Recv;

	//udp �ٸ� client���� ��� (server����)
	// ������ �ޱ�
	// thread for client
	//���⼭ �Ű����� �Ѱ��� �ʿ� ����
	hThread = CreateThread(NULL, 0, ProcessServer, NULL, 0, &ThreadId);
	if (hThread == NULL)
		err_display((char*)"error: failure of thread creation!!!");
	else
		CloseHandle(hThread);

	while (quit != 1) {

		//udp ������ server�� �����ϴ� �κ� (client ��������)
		if (getInfo == 1) {
			while (1) {
				printf("\nQUIT�� ������ ��ü ���� -> [���� ������] ");
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
				// '\n' ���� ����
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}
				if (strncmp(buf, "QUIT", 4) == 0) {
					quit = 1; break;
				}
				retval = sendto(UDPclientsock, buf, len,
					0, (SOCKADDR*)&peerServeraddr, sizeof(peerServeraddr));

				if (retval == SOCKET_ERROR) {
					err_display((char*)"sendto()");
					continue;
				}

				//echo message�޴� �κ�
				addrlen = sizeof(peeraddr);

				retval = recvfrom(UDPclientsock, buf, len, 0,
					(SOCKADDR*)&peeraddr, &addrlen);

				printf("echo : Input Message :  %s\n\n", buf);

				getInfo = 0;
			}
		}
		// ������ ������ ���
		//server�� tcp connect�� �ξ client ������ ���� ����
		while (optnum != 3) {

			msglen = recv(sock, buf, BUFSIZE, 0);
			printf("\n%s", buf);

			//id �� passwordȮ���ϴ� �κ�
			if (strncmp(buf, "Hi! Insert your ID : ", 22) == 0) {
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;

				// '\n' ���� ����
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}

				retval = send(sock, (char*)buf, len, 0);

			}
			//ID confirmed, Insert your Password
			else if (strncmp(buf, "ID confirmed, Insert your PASSWORD : ", 38) == 0) {
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}

				retval = send(sock, (char*)buf, len, 0);

			}

			//buf�� option number�� ����°��� ���
			else if (strncmp(buf, "(1=registration, 2=query client 3=quit) option number? : ", 58) == 0) {
				std::cin >> optnum;

				buf[0] = (char)('0' + optnum);
				printf("option number: %c\n", buf[0]);
				std::cin.ignore(1501, '\n');
			
				retval = send(sock, (char*)buf, 1, 0);

				if (optnum == 3)break;
			}

			//option number �� 1�� �Է�������
			else if (strncmp(buf, "insert your id, IP, portnumber please ", 39) == 0) {
				printf("\nyour id : ");
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
			
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}
				memset(&client_Info, 0, 26);
				strncpy(client_Info.id, buf, len);
				//==========================
				//���� �� wifi �� IP address
				//==========================
				printf("\nyour IP address is %s", "192.168.0.6");
				client_Info.IP = inet_addr("192.168.0.6");
				printf("\nyour port number is %d\n", ntohs(udpServerPort));
				client_Info.portnumber = udpServerPort;

				retval = send(sock, (char*)&client_Info, 26, 0);
				if (retval == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}

				//confirm �Ұ��� ������ �����ϴ°�
				//buf�� ���� clientInfo������
				//confirm ���� ���� ����� ������ ��� ����
				msglen = recv(sock, buf, BUFSIZE, 0);
				memset(&client_Info_Recv, 0, 26);
				memcpy(&client_Info_Recv, buf, sizeof client_Info_Recv);

				checkClientAddr.sin_addr.s_addr = client_Info_Recv.IP;

				printf("\necho : client Id : %s\nIP address : %s\nportnumber : %d\n",
					client_Info_Recv.id, inet_ntoa(checkClientAddr.sin_addr), ntohs(client_Info_Recv.portnumber));

				msglen = recv(sock, buf, BUFSIZE, 0);
				printf("%s ", buf);

				//message input yes or no
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
		
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}
				retval = send(sock, (char*)buf, len, 0);
				if (retval == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}


			}

			//ã������ client ���� �˻��ϴ� ��
			else if (strncmp(buf, "tell me the client ID that you are finding", 43) == 0) {
				printf("\nclientID : ");
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
			
				len = strlen(buf);
				if (buf[len - 1] == '\n')
					buf[len - 1] = '\0';
				if (strlen(buf) == 0) {
					continue;
				}
				retval = send(sock, (char*)buf, len, 0);
				if (retval == SOCKET_ERROR) {
					err_display((char*)"send()");
					break;
				}
				//id do not exist Ȥ�� ������ ������
				msglen = recv(sock, buf, BUFSIZE, 0);

				if (strncmp(buf, "id do not exist", 16) == 0)
					printf("%s\n", buf);

				else {
					memset(&client_Info_Recv, 0, 26);
					memcpy(&client_Info_Recv, buf, sizeof client_Info_Recv);
					checkClientAddr.sin_addr.s_addr = client_Info_Recv.IP;
					printf("That you are finding is : client Id : %s\nIP address : %s\nportnumber : %d\n",
						client_Info_Recv.id, inet_ntoa(checkClientAddr.sin_addr), ntohs(client_Info_Recv.portnumber));
					//���߿� udp ������ ������ ���� ����
					peerServeraddr.sin_port = client_Info_Recv.portnumber;
					peerServeraddr.sin_addr.s_addr = client_Info_Recv.IP;
					getInfo = 1;
				}


			}

			optnum = 0;

		}
		// closeTCPsocket()
		closesocket(sock);
	}
	// closesocket()
	closesocket(UDPclientsock);


	// ���� ����
	WSACleanup();
	return 0;
}