/* File Name: storeClient
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description:Store client, Platform server와 udp 통신하는 client
 * Last Changed: 2020-06-29
*/
#include <winsock.h>
#include <stdio.h>
#include<stdlib.h>
#include<iostream>
#define BUFSIZE 1024
#define BUFFERSIZE 1024
#define addressBUFSIZE 128
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable:4996)


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

//client인지 server인지 구별하고, rider인지 store인지 구별하는 구조체
//Store Client : type=0, number=1
typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;

//모든 syntax를 flag별로 분류할 수 있는
//역할을 가진 구조체
/*
* flag = 1; 로그인
* flag = 2; 회원 가입
* flag = 3; 주문 정보 요청
* flag = 4: 광고 등록 요청
* flag = 5: 주분 정보 변경 요청
*/

typedef struct flagprotocol
{
	Destination start;
	Destination end;
	int flag;
	char buffer[BUFSIZE];
}FlagProtocol;

//광고관련 syntax
typedef struct advertisementprotocol
{
	Destination start;
	Destination end;
	int flag;
	int store_index;
	int result;
}AdvertisementProtocol;

//로그인||회원가입 관련 syntax
typedef struct adminprotocol
{
	Destination start;
	Destination end;
	int flag;
	char id[40];
	char password[40];
	char storename[40];
	char address[40];
	char category[10];
	int store_index;
	int result;
}AdminProtocol;

//주문 list관련 구조체
typedef struct orderingInfo
{
	char foodname[30];
	char address[40];
	char status[20];
	char ridername[30];
	char storename[30];
	char timestamp[30];
}OrderingInfo;

//orderingInfo 구조체 포함하는 syntax 
typedef struct orderingInfoList
{
	Destination start;
	Destination end;
	int flag;
	int store_index;
	int whole_row;
	OrderingInfo Info[5];
}OrderingInfoList;

//orderingInfo store 관련 update syntax
typedef struct acceptProtocol
{
	Destination start;
	Destination end;
	int flag;
	int itemid;
	int store_index;
	int statusid;
	int result;
}AcceptProtocol;

/*
전체 주문 list를 print해주는 함수
input : 전체 주문 list를 담고 있는 list
*/
void displayWholeOrderingList(OrderingInfoList* list)
{
	for (int i = 0; i < list->whole_row; i++)
	{
		printf("-----------------------------------------\n");
		printf("주문 list %d\n", i + 1);
		printf("%s\n", list->Info[i].timestamp);
		printf("메뉴 : %s\n", list->Info[i].foodname);
		printf("손님 주소 : %s\n", list->Info[i].address); //customer address
		if (strlen(list->Info[i].ridername) != 0)
		{
			printf("라이더 이름 : %s\n", list->Info[i].ridername);

		}
		printf("%s\n", list->Info[i].status);
		printf("-----------------------------------------\n\n");
	}

}

/*
조리 완료 명령 업데이트가 잘되었는지 print해주는 함수
*/
void printUpdateCheckMessage(AcceptProtocol* message)
{
	if (message->result == 1)
	{
		printf("\n업데이트 완료!!!\n\n");
	}
	else
	{
		printf("\n업데이트 실패!!\n\n");
	}
}

/*
광고가 잘 등록 되었는지 print해주는 함수
*/
void printInsertCheckMessage(AdvertisementProtocol* message)
{
	if (message->result == 1)
	{
		printf("\n광고 등록 완료!!!\n\n");
	}
	else
	{
		printf("\n광고 등록 실패!!\n\n");
	}
}

/*
platform 서버와 udp 통신하는 main함수
*/
int main() {

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
	serveraddr.sin_port = htons(8600); //platform server port number
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int inputNum = 0;
	char buffer[BUFFERSIZE + 1];
	int result_store_index = 0;


	// 서버와 데이터 통신
	while (1) {

		printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("┃                                ┃\n");
		printf("┃      최고의 배달 플랫폼        ┃\n");
		printf("┃     [ H U N G R Y M E N ]      ┃\n");
		printf("┃                                ┃\n");
		printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");



		char buf[BUFSIZE + 1];
		int loginOrRegister;
		int retval;
		int checkloginstate = 0;

		while (1) {

			printf("\n로그인을 원하시나요? 회원가입을 원하시나요?(로그인:1), (회원가입:2) :");
			scanf("%d", &loginOrRegister);

			if (loginOrRegister == 1) printf("LOGIN\n\n");
			else printf("REGISTER\n\n");

			//로그인 회원가입의 시작
			AdminProtocol* request = (AdminProtocol*)malloc(sizeof(AdminProtocol));
			printf("id :  ");
			scanf("%s", request->id);
			request->id[strlen(request->id)] = '\0';
			printf("password : ");
			scanf("%s", request->password);
			request->password[strlen(request->password)] = '\0';
			request->start.type = 0;
			request->end.type = 1;
			request->end.number = 1;

			getchar();
			if (loginOrRegister == 1) request->flag = 1;
			else
			{
				request->flag = 2;
				printf("\nstorename : ");
				scanf("%[^\n]", request->storename);
				request->storename[strlen(request->storename)] = '\0';

				getchar();
				printf("\naddress : ");
				scanf("%[^\n]", request->address);
				request->address[strlen(request->address)] = '\0';

				printf("\nfood category 1.한식 2.양식 3.중식 : ");
				int category = 0;
				scanf("%d", &category);
				if (category == 1)
				{
					strncpy(request->category, "한식", strlen("한식"));
					request->category[strlen("한식")] = '\0';
				}
				else if (category == 2)
				{
					strncpy(request->category, "양식", strlen("양식"));
					request->category[strlen("양식")] = '\0';
				}
				else
				{
					strncpy(request->category, "중식", strlen("중식"));
					request->category[strlen("중식")] = '\0';
				}


			}

			//server 에게 데이터 보내기
			retval = sendto(sock, (char*)request, sizeof(AdminProtocol), 0,
				(SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR)
			{
				err_display((char*)"sendto()");
				continue;
			}

			free(request);

			//from server 
			AdminProtocol* response;
			addrlen = sizeof(peeraddr);
			retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display((char*)"recvfrom()");
				continue;
			}
			response = (AdminProtocol*)buffer;

			//로그인
			if (response->flag == 1)
			{
				if (response->result == 0)
				{
					printf("로그인 정보가 틀렸습니다.\n처음화면으로 돌아갑니다\n\n");
					continue;
				}
				else
				{
					result_store_index = response->store_index;
					break;
				}
			}
			//회원가입
			else
			{
				if (response->result == 0)
				{
					printf("중복된 아이디 사용입니다.\n처음 화면으로 돌아갑니다\n\n");
					continue;
				}
				else
				{
					result_store_index = response->store_index;
					break;
				}
			}
		}
		printf(" \n\n **********************************\n");
		printf("\n\nWELCOME TO HUNGRYMAN\n");
		printf(" \n\n **********************************\n");

		int workstatus = 0;
		while (workstatus == 0)
		{
			int command = 0;
			printf("  **********************************\n");
			printf("  *   할 일을 선택하세요~!         *\n");
			printf("  * 1. 조리 완료하기               *\n");
			//printf("  * 2. 픽업 완료하기               *\n");//픽업 완료는 배달원이 status를 바꾼다.
			printf("  * 2. 광고 등록하기               *\n");
			printf("  * 3. 로그아웃하기                    *\n");
			printf("  **********************************\n");
			printf("원하시는 작업 번호를 입력하세요 => ");
			scanf("%d", &command);

			OrderingInfoList* orderList = (OrderingInfoList*)malloc(sizeof(OrderingInfoList));
			AcceptProtocol* statuscommand = (AcceptProtocol*)malloc(sizeof(AcceptProtocol));
			AdvertisementProtocol* advertisementRegister = (AdvertisementProtocol*)malloc(sizeof(AdvertisementProtocol));

			if (command == 4) break;


			switch (command)
			{
			case 1:
				//주문 리스트 요구
				orderList->flag = 3;
				orderList->store_index = result_store_index;
				orderList->start.type = 0;
				orderList->end.type = 1;
				orderList->end.number = 1;

				retval = sendto(sock, (char*)orderList, sizeof(OrderingInfoList), 0,
					(SOCKADDR*)&serveraddr, sizeof(serveraddr));
				if (retval == SOCKET_ERROR)
				{
					err_display((char*)"sendto()");
					continue;
				}
				break;

			case 2:
				//광고 등록 요구
				advertisementRegister->flag = 4;
				advertisementRegister->store_index = result_store_index;
				advertisementRegister->start.type = 0;
				advertisementRegister->end.type = 1;
				advertisementRegister->end.number = 1;
				retval = sendto(sock, (char*)advertisementRegister, sizeof(AdvertisementProtocol), 0,
					(SOCKADDR*)&serveraddr, sizeof(serveraddr));
				if (retval == SOCKET_ERROR)
				{
					err_display((char*)"sendto()");
					continue;
				}

				retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
					(SOCKADDR*)&peeraddr, &addrlen);
				if (retval == SOCKET_ERROR)
				{
					err_display((char*)"recvfrom()");
					continue;
				}

				printInsertCheckMessage((AdvertisementProtocol*)buffer);

				break;

			case 3:
				workstatus = 1;
				break;
			default:
				break;
			}

			if (command == 1) {

				retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
					(SOCKADDR*)&peeraddr, &addrlen);
				if (retval == SOCKET_ERROR)
				{
					err_display((char*)"recvfrom()");
					continue;
				}

				FlagProtocol* commandFromServer = (FlagProtocol*)buffer;

				switch (commandFromServer->flag)
				{
				case 1:
					displayWholeOrderingList((OrderingInfoList*)buffer);
					printf("\n라이더가 연결된 조리 완료할 메뉴를 리스트에서 선택해주세요 : ");
					int listnum;
					scanf("%d", &listnum);
					statuscommand->flag = 5;
					statuscommand->itemid = listnum - 1;
					statuscommand->store_index = result_store_index;
					statuscommand->statusid = 1;
					statuscommand->start.type = 0;
					statuscommand->end.type = 1;
					statuscommand->end.number = 1;
					
					//조리 완료 요청
					retval = sendto(sock, (char*)statuscommand, sizeof(AcceptProtocol), 0,
						(SOCKADDR*)&serveraddr, sizeof(serveraddr));
					if (retval == SOCKET_ERROR)
					{
						err_display((char*)"sendto()");
						continue;
					}

					//업데이트 확인 메시지
					retval = recvfrom(sock, buffer, BUFFERSIZE, 0,
						(SOCKADDR*)&peeraddr, &addrlen);
					if (retval == SOCKET_ERROR)
					{
						err_display((char*)"recvfrom()");
						continue;
					}
					printUpdateCheckMessage((AcceptProtocol*)buffer);

					break;

				default:
					break;
				}


			}
		}


	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;

}