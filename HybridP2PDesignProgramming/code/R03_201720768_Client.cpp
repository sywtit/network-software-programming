/* File Name: R03_201720768_Client.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: TCPserver와 통신을 해서 원하는 정보를 등록하거나 얻어오고
 다른 client끼리 UDP 통신을 하는 프로그램
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
//thread에서 받아올 udpServerPort
USHORT udpServerPort = 0;
int quit = 0;

//client 가 등록할 id, IP,  portnumber 구조체 배열선언
typedef struct clientInfo {
	char id[19];
	ULONG IP;
	USHORT portnumber;
};

//server에서 confirm 으로 받아올 structure
typedef struct clientInfoRecv {
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

//UDPserver thread
DWORD WINAPI ProcessServer(LPVOID arg)
{
	char				buf[BUFSIZE + 1];
	SOCKADDR_IN			clientaddr;
	int					addrlen;
	int					retval;


	//UDP server socket 생성
	SOCKET UDPserversock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDPserversock == INVALID_SOCKET) err_quit((char*)"socket()");

	//UDP server bind
	SOCKADDR_IN UDPserveraddr;
	ZeroMemory(&UDPserveraddr, sizeof(UDPserveraddr));
	UDPserveraddr.sin_family = AF_INET;
	//==========================
	//client에 해당하는 port number
	//===========================
	UDPserveraddr.sin_port = htons(1000);
	udpServerPort = htons(1000);
	UDPserveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(UDPserversock, (SOCKADDR*)&UDPserveraddr, sizeof(UDPserveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");


	//데이터 받기
	while (quit != 1) {

		addrlen = sizeof(clientaddr);
		retval = recvfrom(UDPserversock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
		}
		buf[retval] = '\0';

		// 받은 데이터 출력
		printf("\n[UDP/%s:%d]\nInput Message :  %s\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), buf);

		//echo message 데이터 전송
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
	//client 정보가 맞는지 확인하는 부분
	SOCKADDR_IN checkClientAddr;
	int optnum = 0;
	char buf[BUFSIZE + 1];
	int len;
	//UDP 데이터 통신에 필요한 변수
	SOCKADDR_IN clientaddr;
	SOCKADDR_IN peeraddr;
	HANDLE			hThread;
	DWORD			ThreadId;
	SOCKET			server_sock;
	int addrlen;
	//TCP를 통해서 다른 client정보를 얻었을때
	int getInfo = 0;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	//udp client socket
	SOCKET UDPclientsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDPclientsock == INVALID_SOCKET) err_quit((char*)"socket()");


	// TCP socket() socket 생성
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// server address
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	//====================
	//나의 aws IP address
	//====================
	serveraddr.sin_addr.s_addr = inet_addr("15.164.232.220");

	// peer server address 소켓 주소 구조체 초기화
	SOCKADDR_IN peerServeraddr;
	ZeroMemory(&peerServeraddr, sizeof(peerServeraddr));
	peerServeraddr.sin_family = AF_INET;


	// connect() server 와의 connect 
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"connect()");

	clientInfo client_Info;
	clientInfoRecv client_Info_Recv;

	//udp 다른 client와의 통신 (server관점)
	// 데이터 받기
	// thread for client
	//여기서 매개변수 넘겨줄 필요 없음
	hThread = CreateThread(NULL, 0, ProcessServer, NULL, 0, &ThreadId);
	if (hThread == NULL)
		err_display((char*)"error: failure of thread creation!!!");
	else
		CloseHandle(hThread);

	while (quit != 1) {

		//udp 데이터 server로 전송하는 부분 (client 관점에서)
		if (getInfo == 1) {
			while (1) {
				printf("\nQUIT를 적으면 전체 종료 -> [보낼 데이터] ");
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;
				// '\n' 문자 제거
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

				//echo message받는 부분
				addrlen = sizeof(peeraddr);

				retval = recvfrom(UDPclientsock, buf, len, 0,
					(SOCKADDR*)&peeraddr, &addrlen);

				printf("echo : Input Message :  %s\n\n", buf);

				getInfo = 0;
			}
		}
		// 서버와 데이터 통신
		//server와 tcp connect를 맺어서 client 정보를 얻어올 구간
		while (optnum != 3) {

			msglen = recv(sock, buf, BUFSIZE, 0);
			printf("\n%s", buf);

			//id 와 password확인하는 부분
			if (strncmp(buf, "Hi! Insert your ID : ", 22) == 0) {
				ZeroMemory(buf, sizeof(buf));
				if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
					break;

				// '\n' 문자 제거
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

			//buf에 option number를 물어보는것이 담김
			else if (strncmp(buf, "(1=registration, 2=query client 3=quit) option number? : ", 58) == 0) {
				std::cin >> optnum;

				buf[0] = (char)('0' + optnum);
				printf("option number: %c\n", buf[0]);
				std::cin.ignore(1501, '\n');
			
				retval = send(sock, (char*)buf, 1, 0);

				if (optnum == 3)break;
			}

			//option number 를 1로 입력했을때
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
				//현재 내 wifi 의 IP address
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

				//confirm 할건지 말건지 결정하는곳
				//buf의 값은 clientInfo정보와
				//confirm 할지 말지 물어보는 정보가 담겨 있음
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

			//찾으려는 client 정보 검색하는 곳
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
				//id do not exist 혹은 정보들 가져옴
				msglen = recv(sock, buf, BUFSIZE, 0);

				if (strncmp(buf, "id do not exist", 16) == 0)
					printf("%s\n", buf);

				else {
					memset(&client_Info_Recv, 0, 26);
					memcpy(&client_Info_Recv, buf, sizeof client_Info_Recv);
					checkClientAddr.sin_addr.s_addr = client_Info_Recv.IP;
					printf("That you are finding is : client Id : %s\nIP address : %s\nportnumber : %d\n",
						client_Info_Recv.id, inet_ntoa(checkClientAddr.sin_addr), ntohs(client_Info_Recv.portnumber));
					//나중에 udp 소통할 변수에 값들 저장
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


	// 윈속 종료
	WSACleanup();
	return 0;
}