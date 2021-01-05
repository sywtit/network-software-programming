/* File Name: R01_201720768_COechoSvr
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CO socket programming 과정에서 server 가
 * 여러 형식의 데이터을 받고 echo message 를 보내는 프로그램
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
#define INPUTSIZE 1000 //client 에서 전송할 입력 문자열 의 크기 client 소스와 똑같이 지정

//client에서 받을 구조체 형식의 syntax
typedef struct sendedStructure {
	ULONG studentNum;
	USHORT countOfName;
	char name[14];
	char input[INPUTSIZE];
};

//server 에서 전송할 구조체 형식의 syntax
typedef struct sendStructure {
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
	// 데이터 통신에 사용할 변수
	SOCKET listen_sock;
	SOCKET client_sock;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	int		addrlen;
	char	buf[BUFSIZE + 1];
	int		retval, msglen;
	//문자열이 'QUIT'일때 echo message 를 보내고 socket 종료를 하는것을 도와줄 변수 선언
	int retval_send = -1;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket() //listen socket 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"Hello World");

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


	while (1) {
		// accept() //client socket 생성
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display((char*)"accept()");
			continue;
		}

		printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		printf("\nAnd this is new socket value: %d\n", client_sock);

		// 클라이언트와 데이터 통신
		while (1) {

			// 데이터 받기
			msglen = recv(client_sock, buf, BUFSIZE, 0);
			if (msglen == SOCKET_ERROR) {
				err_display((char*)"recv()");
				break;
			}
			else if (msglen == 0)
				break;

			buf[msglen] = '\0';

			//client 와 동일한 구조체 생성해
			//client 에서 받은 message buf를 저장
			sendedStructure recvMessage;
			memcpy(&recvMessage, buf, sizeof recvMessage);

			// 받은 데이터 구조체 parsing 후 출력            
			printf("[TCP/%s:%d] : 학번: %d, 이름: %s, 메세지: %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), ntohl(recvMessage.studentNum),
				recvMessage.name, recvMessage.input);


			//받은 데이터로 새로운 client로 보낼 구조체 생성
			sendStructure sendMessage;
			memset(sendMessage.input, 0, strlen(sendMessage.input));
			memcpy(sendMessage.input, recvMessage.input, strlen(recvMessage.input));
			sendMessage.inputNum = strlen(recvMessage.input);

			// client 로 echo message를 받은 문자열의 길이만큼 데이터 보내기
			retval_send = send(client_sock, (char*)&sendMessage, 2 + msglen - 19 + 1, 0);
			if (retval_send == SOCKET_ERROR) {
				err_display((char*)"send()");
				break;
			}

			// client 에서 받은 문자열이 QUIT 일때 echo message 를 보낸후 
			//client 와의 데이터 교환이 일어나는 loop 에서 break
			if ((strncmp(recvMessage.input, "QUIT", 4) == 0) && sendMessage.inputNum == 4 &&
				retval_send != -1) break;
		}

		//closesocket() //client socket 를 종료
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	// closesocket() //listen socket 를 종료
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}