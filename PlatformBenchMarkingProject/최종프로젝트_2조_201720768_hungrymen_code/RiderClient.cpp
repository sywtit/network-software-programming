// UDPEchoClient.c
// UDP echo client
//
// �����: ��Ʈ��ũ����Ʈ������
// ���ִ��б� ����Ʈ�����а�
//
#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>

//�� ��ĥ �� ������ ���� �߸� ��ģ�Ŵ�.
#define BUFSIZE 1024
#define PORT 8600

typedef struct Destination {
	int type;
	int number;
	SOCKADDR_IN addr;
}Destination;

/* ���ʷ� ����ü
* �� ����ü�� flag ���� ������ � ��ƾ�� ó���ؾ� �ϴ��� �� �� �ִ�.
* flag = 1; �α���
* flag = 2; ȸ�� ����
* flag = 3; �ֹ� ���� ��û
* flag = 4; ���� ���� ��û
* flag = 5; ��� ���� ���� ��û
* flag = 6; �ش� �ֹ� �¶�
* flag = 7; ���� ���� �¶�
* flag = 8; Ư�� �ֹ� ���� ��ȭ
*/
typedef struct flagprotocol {
	Destination start;
	Destination end;
	int flag;
	char buffer[BUFSIZE];
}FlagProtocol;

//ȸ�� ���� �� �α��� �� �ʿ��� ��������
typedef struct adminprotocol {
	Destination start;
	Destination end;
	int flag;
	char id[40];		//ȸ�� ���� �̸�
	char name[40];		//���̵�.   ���н� ������ ����� ����
	char password[40];	//��й�ȣ
	int result;		// ���� �� ����. (���ڸ� ok 0�̸� ����)
}AdminProtocol;

//����Ʈ �ȿ� ���� ����ü
typedef struct StoreInfo {
	int no;
	char clientaddress[40];
	char storeaddress[40];
	char storename[30];
	char menuname[30];
	char date[20];
}StoreInfo;

//����Ʈ ��û�� �ʿ��� ��������
typedef struct ListProtocol {
	Destination start;
	Destination end;
	int flag;
	int userid;
	int numofstore;
	StoreInfo info[5];
}ListProtocol;

//�ֹ� �¶��� �ʿ��� ��������
typedef struct AcceptProtocol {
	Destination start;
	Destination end;
	int flag;
	int itemid;
	int userid;
	int result;
}AcceptProtocol;

// ���� �Լ� ���� ��� �� ����
void err_quit(char* msg)
{
	printf("%s\n", msg);
}

// ���� �Լ� ���� ���
void err_display(char* msg)
{
	printf("%s\n", msg);
}

int identity;

int main(int argc, char* argv[])
{
	int retval;

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
	serveraddr.sin_port = htons(PORT);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;

	printf("\n��������������������������������������������������������������������\n");
	printf("��                                ��\n");
	printf("��      �ְ��� ��� �÷���        ��\n");
	printf("��     [ H U N G R Y M E N ]      ��\n");
	printf("��                                ��\n");
	printf("��������������������������������������������������������������������\n\n");

	// ������ ������ ���
	while (1) {
		int direction = 0;
		printf("  ********************************\n");
		printf("  *   ���� �� �۾��� �����ϼ���  *\n");
		printf("  *   1. �α���                  *\n");
		printf("  *   2. ȸ�� ����               *\n");
		printf("  ********************************\n");
		printf("���Ͻô� �۾� ��ȣ�� �Է��ϼ��� => ");
		scanf("%d", &direction);
		printf("\n");

		/*
		AdminProtocol�� id, password, name(ȸ�� ���� �� ���)�� �Է� �޴´�.
		�����
		request->start.type = 0;
		request->end.type = 1;
		request->end.number = 2;
		�� �κ��� �÷����� ���� ����ϱ� ���� ��ȣ�̴�.
		*/
		printf("\n---------------------------------\n");
		AdminProtocol* request = (AdminProtocol*)malloc(sizeof(AdminProtocol));
		request->start.type = 0;
		request->end.type = 1;
		request->end.number = 2;
		printf("  ���̵� �Է��ϼ��� => ");
		scanf("%s", request->id);
		request->id[strlen(request->id)] = '\0';
		printf("  ��� ��ȣ�� �Է��ϼ��� =>");
		scanf("%s", request->password);
		request->password[strlen(request->password)] = '\0';

		if (direction == 1) {
			request->flag = 1;
		}
		else {
			request->flag = 2;
			printf("  �̸��� �Է��ϼ���  => ");
			scanf("%s", request->name);
			request->name[strlen(request->name)] = '\0';
		}
		printf("---------------------------------\n");

		// ������ ������
		retval = sendto(sock, (char*)request, sizeof(AdminProtocol), 0,
			(SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			err_display((char*)"sendto()");
			continue;
		}
		free(request);

		// ������ �ޱ�
		AdminProtocol* response;
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display((char*)"recvfrom()");
			continue;
		}
		response = (AdminProtocol*)buf;

		if (response->result == 0) {
			printf("%s\n", response->name);
			continue;
		}
		else {
			if (response->flag == 1) {
				printf("�α��� �Ǿ����ϴ�. ���� ������� �Ѿ�ϴ�.\n");
				identity = response->result;
				break;
			}
			else {
				printf("ȸ�� ���� �Ǿ����ϴ�.\n");
			}
		}
		printf("---------------------------------\n\n");
	}

	while (1) {
		printf("\n");
		int whatdo;
		printf("  **********************************\n");
		printf("  *   �� ���� �����ϼ���~!         *\n");
		printf("  * 1. �ֹ� �����ϱ�               *\n");
		printf("  * 2. ���� ����ϱ�               *\n");
		printf("  * 3. ��� �Ϸ��ϱ�               *\n");
		printf("  * 4. �����ϱ�                    *\n");
		printf("  **********************************\n");
		printf("���Ͻô� �۾� ��ȣ�� �Է��ϼ��� => ");
		scanf("%d", &whatdo);

		if (whatdo == 4) {
			break;
		}

		/*
		ListProtocol�� �ڽ��� ���ϴ� list�� �Ӽ� (����, ��� �����ϱ�, ��� �Ϸ��ϱ�)�� flag�� ���� �����Ѵ�.
		���������� flag�� ���� flag�� ���� ó���Ѵ�.
		*/
		ListProtocol* request = (ListProtocol*)malloc(sizeof(ListProtocol));
		request->start.type = 0;
		request->end.type = 1;
		request->end.number = 2;
		switch (whatdo) {
		case 1:
			request->flag = 3;
			retval = sendto(sock, (char*)request, sizeof(ListProtocol), 0,
				(SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			free(request);
			break;
		case 2:
			request->flag = 4;
			retval = sendto(sock, (char*)request, sizeof(ListProtocol), 0,
				(SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			free(request);
			break;
		case 3:
			request->flag = 5;
			request->userid = identity;
			retval = sendto(sock, (char*)request, sizeof(ListProtocol), 0,
				(SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				err_display((char*)"sendto()");
				continue;
			}
			free(request);
			break;
		default:
			printf("�������� ���� Ȱ���Դϴ�. �ٽ� �����ϼ���\n\n");
			continue;
		}

		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);

		//������ ���� ����Ʈ�� �޾ƿͼ� �̸� �Ѹ���.
		ListProtocol* returnlist = (ListProtocol*)buf;

		printf("\n------- ���� ����Ʈ ---------------------------------\n");
		for (int i = 0; i < returnlist->numofstore; i++) {
			if (whatdo == 2) {
				printf("  %d =>  ���� �̸� : %s \n",
					i + 1, returnlist->info[i].storename);
			} //����� �׳� �������� �Ѹ��� �Ŷ� �ٸ��� ó���Ѵ�.
			else {
				printf("  %d => ���� �̸� : %s    ",
					i + 1, returnlist->info[i].storename);
				printf("  %d => �մ� �ּ� : %s    ",
					i + 1, returnlist->info[i].clientaddress);
				printf("  %d => ���� �ּ� : %s     ",
					i + 1, returnlist->info[i].storeaddress);
				printf("  %d => ��¥ : %s     ",
					i + 1, returnlist->info[i].date);
				printf("\n");
			}
		}
		printf(" !! �ǵ��ư���   => 0���� �Է��ϼ���\n");
		printf("------------------------------------------------------\n");

		//����Ʈ ���� �� ó��
		printf("\n ----- ���ϴ� ����Ʈ ��ȣ�� �����ϼ��� -----\n");
		int choicelist = 1;
		scanf("%d", &choicelist);
		if (choicelist == 0) {
			printf("ó������ �ǵ��ư��ϴ�. \n");
			continue;
		}

		if ((choicelist < 1) && (choicelist > returnlist->numofstore)) {
			printf("�������� ���� ����Ʈ�Դϴ�. ó������ ���ư��ϴ�.\n");
		}

		/*
		AcceptProtocol�� �ڽ��� ������ ordering �Ǵ� ������ ���¸� ��ȭ��Ű�� ���� ��û�Ѵ�.
		ordering�� _no �Ǵ� advertisement�� _no ���� AcceptProtocol�� ���� �����Ѵ�.
		*/
		AcceptProtocol* senddata = (AcceptProtocol*)malloc(sizeof(AcceptProtocol));
		senddata->userid = identity;
		senddata->start.type = 0;
		senddata->end.type = 1;
		senddata->end.number = 2;
		switch (whatdo) {
		case 1:
			senddata->flag = 6;
			senddata->itemid = (returnlist->info[choicelist - 1]).no;
			break;
		case 2:
			senddata->flag = 7;
			senddata->itemid = (returnlist->info[choicelist - 1]).no;
			break;
		case 3:
			senddata->flag = 8;
			senddata->itemid = (returnlist->info[choicelist - 1]).no;
			break;
		default:
			break;
		}

		//ordering list ���ε� �ϴ� �κ� --> ����� �߰� ������ ���� ��Ź
		retval = sendto(sock, (char*)senddata, sizeof(AcceptProtocol), 0,
			(SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			err_display((char*)"sendto()");
			continue;
		}
		free(senddata);

		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);

		AcceptProtocol* returnaccept = (AcceptProtocol*)buf;
		if (returnaccept->result == 1) {
			printf("���������� ó���Ǿ����ϴ�.\n");
			continue;
		}
		else {
			if (whatdo == 1) {
				printf("�̹� �������� �������׿�.\n");
			}
			else {
				printf("������ ������ �ֳ׿�...\n");
			}
			continue;
		}
	}
	printf("�����Ͽ����ϴ�. �����մϴ�.\n");
	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}