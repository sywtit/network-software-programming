#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 512

struct infotoSend {
	char id[20];
	char pw[20];
	char storename[20];
	char order[20];
	char selectnum[100];
	char logininfo[100];
	char menu[100];
	char address[100];
};
struct infotoRecv {
	char store[200];
	char choosemenu[200];
	char inputadd[100];
	int storenumb;
};
// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	//MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
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

int main(int argc, char* argv[])
{
	int retval;
	int answer;
	int logincount = 0;

	//로그인할건지 회원가입할 것인지에 대해 물어보는 int 
	infotoRecv recvinfo;
	infotoSend sendinfo;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 소켓 주소 구조체 초기화
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(8000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;

	// 서버와 데이터 통신
	while (1) {
		printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("┃                                ┃\n");
		printf("┃      최고의 배달 플랫폼        ┃\n");
		printf("┃     [ H U N G R Y M E N ]      ┃\n");
		printf("┃                                ┃\n");
		printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");


		/*1. 회원가입 혹은 로그인 통신*/
		while (logincount == 0) {
			addrlen = sizeof(peeraddr);

			printf("  ===============================\n\n");
			printf("     진행 할 작업을 선택하세요. \n\n");
			printf("     1. 로그인\n");
			printf("     2. 회원가입\n\n");
			printf("  ===============================\n");
			printf("   입력 : ");
			scanf("%d", &answer);

			printf("\n   ID 입력 :");
			scanf("%s", sendinfo.id);
			len = strlen(sendinfo.id);
			sendinfo.id[len] = '\0';
			printf("   PW 입력 :");
			scanf("%s", sendinfo.pw);
			len = strlen(sendinfo.pw);
			sendinfo.pw[len] = '\0';


			if (answer == 1)//로그인하기
				sprintf(sendinfo.logininfo, "1 %s %s", sendinfo.id, sendinfo.pw);
			else //회원가입하기
				sprintf(sendinfo.logininfo, "2 %s %s", sendinfo.id, sendinfo.pw);


			retval = sendto(sock, sendinfo.logininfo, sizeof(sendinfo.logininfo), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				printf("sendto()");
				continue;
			}
			/*회원가입 혹은 로그인 정보 받기*/
			retval = recvfrom(sock, buf, BUFSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			buf[retval] = '\0';
			printf("\n  %s", buf);

			if (strcmp(buf, "회원가입 완료\n") == 0 || strcmp(buf, "로그인 완료 !\n") == 0) {
				logincount = 1;
				//printf("끝!\n");
				break;
			}
		}

		/*2. 카테고리 입력받고, 원하는 카테고리 전송하기
		만약, 카테고리 내에서 상점이 없다면 다시 물어본다.*/

		int ordercount = 0;
		while (ordercount == 0) {
			retval = recvfrom(sock, buf, BUFSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			printf("\n%s", buf);
			printf("\n  원하는 카테고리 번호 : ");
			scanf("%d", &answer);
			printf("\n");
			sprintf(sendinfo.selectnum, "%d", answer);
			retval = sendto(sock, sendinfo.selectnum, sizeof(sendinfo.selectnum), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				printf("sendto()");
				continue;
			}
			retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			buf[retval] = '\0';
			recvinfo.storenumb = atoi(buf);
			/*상점 row가 몇개인지 체크
			0개 있으면 오류 메세지 출력후 continue
			1이상이면 그만큼 출력하고 아웃*/
			if (recvinfo.storenumb == 0) {
				retval = recvfrom(sock, recvinfo.store, sizeof(recvinfo.store), 0, (SOCKADDR*)&peeraddr, &addrlen);
				if (retval == SOCKET_ERROR) {
					err_display("recvfrom()");
					continue;
				}
				recvinfo.store[retval] = '\0';
				printf("%s\n", recvinfo.store);
				continue;
			}
			else {
				for (int i = 0; i < recvinfo.storenumb; i++) {
					retval = recvfrom(sock, recvinfo.store, sizeof(recvinfo.store), 0, (SOCKADDR*)&peeraddr, &addrlen);
					if (retval == SOCKET_ERROR) {
						err_display("recvfrom()");
						continue;
					}
					recvinfo.store[retval] = '\0';
					printf("%s", recvinfo.store);
				}
				break;
			}
		}


		/*3. 상점이름 전송 받고, 상점 이름과 메뉴이름 전송하기*/
		while (1) {
			printf("\n  상점 이름 : ");
			scanf("%s", sendinfo.storename);
			retval = sendto(sock, sendinfo.storename, sizeof(sendinfo.storename), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				printf("sendto()");
				continue;
			}
			/*상점 정보 잘 입력했는지 체크*/
			retval = recvfrom(sock, recvinfo.choosemenu, sizeof(recvinfo.choosemenu), 0, (SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			recvinfo.choosemenu[retval] = '\0';
			if (strcmp(recvinfo.choosemenu, "0") == 0) {
				printf("입력하신 상점이 없습니다!\n");
				continue;
			}
			else {
				printf("%s", recvinfo.choosemenu);
				break;
			}
		}
		/*메뉴정보에 대한 정보 전송*/
		scanf("%s%*c", sendinfo.menu);

		retval = sendto(sock, sendinfo.menu, sizeof(sendinfo.menu), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			printf("sendto()");
			continue;
		}

		/*주소 입력받기*/
		retval = recvfrom(sock, recvinfo.inputadd, sizeof(recvinfo.inputadd), 0, (SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			continue;
		}
		recvinfo.inputadd[retval] = '\0';
		char str[100];
		printf("%s", recvinfo.inputadd);
		//gets_s(sendinfo.address,sizeof(sendinfo.address));

		gets_s(sendinfo.address, sizeof(sendinfo.address));
		//printf("%s", sendinfo.address);

		retval = sendto(sock, sendinfo.address, sizeof(sendinfo.address), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			printf("sendto()");
			continue;
		}

		//order에 대한 정보 받기/
		retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			continue;
		}
		buf[retval] = '\0';
		printf("%s", buf);
		break;



	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}