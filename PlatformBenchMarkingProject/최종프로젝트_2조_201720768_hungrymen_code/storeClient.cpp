/* File Name: storeClient
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description:Store client, Platform server�� udp ����ϴ� client
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

//client���� server���� �����ϰ�, rider���� store���� �����ϴ� ����ü
//Store Client : type=0, number=1
typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;

//��� syntax�� flag���� �з��� �� �ִ�
//������ ���� ����ü
/*
* flag = 1; �α���
* flag = 2; ȸ�� ����
* flag = 3; �ֹ� ���� ��û
* flag = 4: ���� ��� ��û
* flag = 5: �ֺ� ���� ���� ��û
*/

typedef struct flagprotocol
{
	Destination start;
	Destination end;
	int flag;
	char buffer[BUFSIZE];
}FlagProtocol;

//������� syntax
typedef struct advertisementprotocol
{
	Destination start;
	Destination end;
	int flag;
	int store_index;
	int result;
}AdvertisementProtocol;

//�α���||ȸ������ ���� syntax
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

//�ֹ� list���� ����ü
typedef struct orderingInfo
{
	char foodname[30];
	char address[40];
	char status[20];
	char ridername[30];
	char storename[30];
	char timestamp[30];
}OrderingInfo;

//orderingInfo ����ü �����ϴ� syntax 
typedef struct orderingInfoList
{
	Destination start;
	Destination end;
	int flag;
	int store_index;
	int whole_row;
	OrderingInfo Info[5];
}OrderingInfoList;

//orderingInfo store ���� update syntax
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
��ü �ֹ� list�� print���ִ� �Լ�
input : ��ü �ֹ� list�� ��� �ִ� list
*/
void displayWholeOrderingList(OrderingInfoList* list)
{
	for (int i = 0; i < list->whole_row; i++)
	{
		printf("-----------------------------------------\n");
		printf("�ֹ� list %d\n", i + 1);
		printf("%s\n", list->Info[i].timestamp);
		printf("�޴� : %s\n", list->Info[i].foodname);
		printf("�մ� �ּ� : %s\n", list->Info[i].address); //customer address
		if (strlen(list->Info[i].ridername) != 0)
		{
			printf("���̴� �̸� : %s\n", list->Info[i].ridername);

		}
		printf("%s\n", list->Info[i].status);
		printf("-----------------------------------------\n\n");
	}

}

/*
���� �Ϸ� ��� ������Ʈ�� �ߵǾ����� print���ִ� �Լ�
*/
void printUpdateCheckMessage(AcceptProtocol* message)
{
	if (message->result == 1)
	{
		printf("\n������Ʈ �Ϸ�!!!\n\n");
	}
	else
	{
		printf("\n������Ʈ ����!!\n\n");
	}
}

/*
���� �� ��� �Ǿ����� print���ִ� �Լ�
*/
void printInsertCheckMessage(AdvertisementProtocol* message)
{
	if (message->result == 1)
	{
		printf("\n���� ��� �Ϸ�!!!\n\n");
	}
	else
	{
		printf("\n���� ��� ����!!\n\n");
	}
}

/*
platform ������ udp ����ϴ� main�Լ�
*/
int main() {

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
	serveraddr.sin_port = htons(8600); //platform server port number
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int inputNum = 0;
	char buffer[BUFFERSIZE + 1];
	int result_store_index = 0;


	// ������ ������ ���
	while (1) {

		printf("\n��������������������������������������������������������������������\n");
		printf("��                                ��\n");
		printf("��      �ְ��� ��� �÷���        ��\n");
		printf("��     [ H U N G R Y M E N ]      ��\n");
		printf("��                                ��\n");
		printf("��������������������������������������������������������������������\n\n");



		char buf[BUFSIZE + 1];
		int loginOrRegister;
		int retval;
		int checkloginstate = 0;

		while (1) {

			printf("\n�α����� ���Ͻó���? ȸ�������� ���Ͻó���?(�α���:1), (ȸ������:2) :");
			scanf("%d", &loginOrRegister);

			if (loginOrRegister == 1) printf("LOGIN\n\n");
			else printf("REGISTER\n\n");

			//�α��� ȸ�������� ����
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

				printf("\nfood category 1.�ѽ� 2.��� 3.�߽� : ");
				int category = 0;
				scanf("%d", &category);
				if (category == 1)
				{
					strncpy(request->category, "�ѽ�", strlen("�ѽ�"));
					request->category[strlen("�ѽ�")] = '\0';
				}
				else if (category == 2)
				{
					strncpy(request->category, "���", strlen("���"));
					request->category[strlen("���")] = '\0';
				}
				else
				{
					strncpy(request->category, "�߽�", strlen("�߽�"));
					request->category[strlen("�߽�")] = '\0';
				}


			}

			//server ���� ������ ������
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

			//�α���
			if (response->flag == 1)
			{
				if (response->result == 0)
				{
					printf("�α��� ������ Ʋ�Ƚ��ϴ�.\nó��ȭ������ ���ư��ϴ�\n\n");
					continue;
				}
				else
				{
					result_store_index = response->store_index;
					break;
				}
			}
			//ȸ������
			else
			{
				if (response->result == 0)
				{
					printf("�ߺ��� ���̵� ����Դϴ�.\nó�� ȭ������ ���ư��ϴ�\n\n");
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
			printf("  *   �� ���� �����ϼ���~!         *\n");
			printf("  * 1. ���� �Ϸ��ϱ�               *\n");
			//printf("  * 2. �Ⱦ� �Ϸ��ϱ�               *\n");//�Ⱦ� �Ϸ�� ��޿��� status�� �ٲ۴�.
			printf("  * 2. ���� ����ϱ�               *\n");
			printf("  * 3. �α׾ƿ��ϱ�                    *\n");
			printf("  **********************************\n");
			printf("���Ͻô� �۾� ��ȣ�� �Է��ϼ��� => ");
			scanf("%d", &command);

			OrderingInfoList* orderList = (OrderingInfoList*)malloc(sizeof(OrderingInfoList));
			AcceptProtocol* statuscommand = (AcceptProtocol*)malloc(sizeof(AcceptProtocol));
			AdvertisementProtocol* advertisementRegister = (AdvertisementProtocol*)malloc(sizeof(AdvertisementProtocol));

			if (command == 4) break;


			switch (command)
			{
			case 1:
				//�ֹ� ����Ʈ �䱸
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
				//���� ��� �䱸
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
					printf("\n���̴��� ����� ���� �Ϸ��� �޴��� ����Ʈ���� �������ּ��� : ");
					int listnum;
					scanf("%d", &listnum);
					statuscommand->flag = 5;
					statuscommand->itemid = listnum - 1;
					statuscommand->store_index = result_store_index;
					statuscommand->statusid = 1;
					statuscommand->start.type = 0;
					statuscommand->end.type = 1;
					statuscommand->end.number = 1;
					
					//���� �Ϸ� ��û
					retval = sendto(sock, (char*)statuscommand, sizeof(AcceptProtocol), 0,
						(SOCKADDR*)&serveraddr, sizeof(serveraddr));
					if (retval == SOCKET_ERROR)
					{
						err_display((char*)"sendto()");
						continue;
					}

					//������Ʈ Ȯ�� �޽���
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

	// ���� ����
	WSACleanup();
	return 0;

}