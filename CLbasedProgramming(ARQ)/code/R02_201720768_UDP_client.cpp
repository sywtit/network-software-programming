/* File Name: R02_201720768_UDP_client.cpp
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description: CL programming ���� client �� udp ������� server�� ����ϸ鼭,
 stop-and-wait ARQ protocol �� �̿��� ���α׷�
 * �Է� ������ : �ִ� ���̰� 20�� random �� ���ڿ�
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

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int inputNum = 0;
	char buffer[BUFFERSIZE + 1];

	// ������ ������ ���
	while (1) {
		// ������ �Է�
		// ������ �Է½� random �ϰ� �����ϴ°� ����
		printf("\n[���� ������] ");
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(MINRANDSIZE, MAXRANDSIZE);

		randLength = dis(gen);
		memset(buf, 0, BUFSIZE);
		//���ڴ� �ҹ��ڵ�θ����� �̷���� �ִٰ� �����Ѵ�.
		for (int i = 0; i < randLength; i++) {
			std::uniform_int_distribution<int> dis(97, 122);
			char num = (char)(dis(gen));
			buf[i] = num;
		}

		// '\n' ���� ����
		if (buf[randLength] == '\n')
			buf[randLength] = '\0';
		printf("%s\n", buf);

		//�������� ����ü�ȿ� ���� �ִ´�.
		strncpy(sendMessage[inputNum].input, buf, randLength);
		sendMessage[inputNum].sequence_num = sNumber;

		//����ü ������ ������
		retval = sendto(sock, (char*)&sendMessage[inputNum], randLength + 4,
			0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

		if (retval == SOCKET_ERROR) {
			err_display((char*)"sendto()");
			continue;
		}

		//1�ʸ� time out���� �ð����� ����
		int tout = 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tout, sizeof(tout));

		printf("\n���� ���ڿ� �� : [UDP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n\n", randLength);

		//sequence number����
		sNumber += randLength;

		// ������ �ޱ�
		addrlen = sizeof(peeraddr);
		retval = -1;
		int time_out = 1;
		while (1) {
			retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				//���� error�� timeout �� ���
				if (WSAGetLastError() == WSAETIMEDOUT) {
					printf("time out %d times!\n", time_out++);
					setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tout, sizeof(tout));
					//������
					sendto(sock, (char*)&sendMessage[inputNum], randLength + 4,
						0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
					continue;
				}
			}
			break;
		}
		// �۽����� IP �ּ� üũ
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[����] �߸��� �������Դϴ�!\n");
			continue;
		}

		// ���� ������ ���
		buffer[retval] = '\0';
		memset(&rcvMessage.input, 0, BUFSIZE);
		memcpy(&rcvMessage, buffer, retval);

		int a = -1;
		printf("���� ���ڿ� �� : [UDP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval - 4);

		a = printf("echo : sequence number : %d,  Input Message :  %s\n\n",
			rcvMessage.sequence_num, rcvMessage.input);

		//QUIT �Է½� �ߴ�
		if (strncmp(rcvMessage.input, "QUIT", 4) == 0 && retval - 4 == 4 && a != -1)break;

		inputNum++;
		sendAmount++;
		//100���� ������ �޼��� ������ ����
		if (sendAmount == 100 && a != -1) break;

	}

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}


