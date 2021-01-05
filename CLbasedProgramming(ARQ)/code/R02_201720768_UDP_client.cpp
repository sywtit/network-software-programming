/* File Name: R02_201720768_UDP_client.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CL programming 에서 client 가 udp 방법으로 server와 통신하면서,
 stop-and-wait ARQ protocol 를 이용한 프로그램
 * 입력 데이터 : 최대 길이가 20인 random 한 문자열
 * Last Changed: 2020-04-19
*/

#include<winsock.h>
#include<stdlib.h>
#include<stdio.h>
#include<random>
#define BUFSIZE 512
#define BUFFERSIZE 1024
#define MINRANDSIZE 1
#define MAXRANDSIZE 20

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
	char input[BUFSIZE];
}toMessage;

typedef struct {
	int sequence_num;
	char input[BUFSIZE];
}fromMessage;

int main(int argc, char* argv[]) {
	int retval;
	int sNumber = 0, randLength = 0, sendAmount = 0;
	toMessage sendMessage[100];
	fromMessage rcvMessage;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// 소켓 주소 구조체 초기화
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int inputNum = 0;
	char buffer[BUFFERSIZE + 1];

	// 서버와 데이터 통신
	while (1) {
		// 데이터 입력
		// 데이터 입력시 random 하게 생성하는게 목적
		printf("\n[보낼 데이터] ");
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(MINRANDSIZE, MAXRANDSIZE);

		randLength = dis(gen);
		memset(buf, 0, BUFSIZE);
		//문자는 소문자들로만으로 이루어져 있다고 생각한다.
		for (int i = 0; i < randLength; i++) {
			std::uniform_int_distribution<int> dis(97, 122);
			char num = (char)(dis(gen));
			buf[i] = num;
		}

		// '\n' 문자 제거
		if (buf[randLength] == '\n')
			buf[randLength] = '\0';
		printf("%s\n", buf);

		//보내려는 구조체안에 값을 넣는다.
		strncpy(sendMessage[inputNum].input, buf, randLength);
		sendMessage[inputNum].sequence_num = sNumber;

		//구조체 데이터 보내기
		retval = sendto(sock, (char*)&sendMessage[inputNum], randLength + 4,
			0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

		if (retval == SOCKET_ERROR) {
			err_display((char*)"sendto()");
			continue;
		}

		//1초를 time out기준 시간으로 설정
		int tout = 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tout, sizeof(tout));

		printf("\n전송 문자열 수 : [UDP 클라이언트] %d바이트를 보냈습니다.\n\n", randLength);

		//sequence number갱신
		sNumber += randLength;

		// 데이터 받기
		addrlen = sizeof(peeraddr);
		retval = -1;
		int time_out = 1;
		while (1) {
			retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				//만약 error가 timeout 일 경우
				if (WSAGetLastError() == WSAETIMEDOUT) {
					printf("time out %d times!\n", time_out++);
					setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tout, sizeof(tout));
					//재전송
					sendto(sock, (char*)&sendMessage[inputNum], randLength + 4,
						0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
					continue;
				}
			}
			break;
		}
		// 송신자의 IP 주소 체크
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[오류] 잘못된 데이터입니다!\n");
			continue;
		}

		// 받은 데이터 출력
		buffer[retval] = '\0';
		memset(&rcvMessage.input, 0, BUFSIZE);
		memcpy(&rcvMessage, buffer, retval);

		int a = -1;
		printf("받은 문자열 수 : [UDP 클라이언트] %d바이트를 받았습니다.\n", retval - 4);

		a = printf("echo : sequence number : %d,  Input Message :  %s\n\n",
			rcvMessage.sequence_num, rcvMessage.input);

		//QUIT 입력시 중단
		if (strncmp(rcvMessage.input, "QUIT", 4) == 0 && retval - 4 == 4 && a != -1)break;

		inputNum++;
		sendAmount++;
		//100개의 랜덤한 메세지 전송후 종료
		if (sendAmount == 100 && a != -1) break;

	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}


