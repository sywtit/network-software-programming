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
// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

	//�α����Ұ��� ȸ�������� �������� ���� ����� int 
	infotoRecv recvinfo;
	infotoSend sendinfo;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(8000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;

	// ������ ������ ���
	while (1) {
		printf("\n��������������������������������������������������������������������\n");
		printf("��                                ��\n");
		printf("��      �ְ��� ��� �÷���        ��\n");
		printf("��     [ H U N G R Y M E N ]      ��\n");
		printf("��                                ��\n");
		printf("��������������������������������������������������������������������\n\n");


		/*1. ȸ������ Ȥ�� �α��� ���*/
		while (logincount == 0) {
			addrlen = sizeof(peeraddr);

			printf("  ===============================\n\n");
			printf("     ���� �� �۾��� �����ϼ���. \n\n");
			printf("     1. �α���\n");
			printf("     2. ȸ������\n\n");
			printf("  ===============================\n");
			printf("   �Է� : ");
			scanf("%d", &answer);

			printf("\n   ID �Է� :");
			scanf("%s", sendinfo.id);
			len = strlen(sendinfo.id);
			sendinfo.id[len] = '\0';
			printf("   PW �Է� :");
			scanf("%s", sendinfo.pw);
			len = strlen(sendinfo.pw);
			sendinfo.pw[len] = '\0';


			if (answer == 1)//�α����ϱ�
				sprintf(sendinfo.logininfo, "1 %s %s", sendinfo.id, sendinfo.pw);
			else //ȸ�������ϱ�
				sprintf(sendinfo.logininfo, "2 %s %s", sendinfo.id, sendinfo.pw);


			retval = sendto(sock, sendinfo.logininfo, sizeof(sendinfo.logininfo), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				printf("sendto()");
				continue;
			}
			/*ȸ������ Ȥ�� �α��� ���� �ޱ�*/
			retval = recvfrom(sock, buf, BUFSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			buf[retval] = '\0';
			printf("\n  %s", buf);

			if (strcmp(buf, "ȸ������ �Ϸ�\n") == 0 || strcmp(buf, "�α��� �Ϸ� !\n") == 0) {
				logincount = 1;
				//printf("��!\n");
				break;
			}
		}

		/*2. ī�װ� �Է¹ް�, ���ϴ� ī�װ� �����ϱ�
		����, ī�װ� ������ ������ ���ٸ� �ٽ� �����.*/

		int ordercount = 0;
		while (ordercount == 0) {
			retval = recvfrom(sock, buf, BUFSIZE, 0,
				(SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			printf("\n%s", buf);
			printf("\n  ���ϴ� ī�װ� ��ȣ : ");
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
			/*���� row�� ����� üũ
			0�� ������ ���� �޼��� ����� continue
			1�̻��̸� �׸�ŭ ����ϰ� �ƿ�*/
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


		/*3. �����̸� ���� �ް�, ���� �̸��� �޴��̸� �����ϱ�*/
		while (1) {
			printf("\n  ���� �̸� : ");
			scanf("%s", sendinfo.storename);
			retval = sendto(sock, sendinfo.storename, sizeof(sendinfo.storename), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				printf("sendto()");
				continue;
			}
			/*���� ���� �� �Է��ߴ��� üũ*/
			retval = recvfrom(sock, recvinfo.choosemenu, sizeof(recvinfo.choosemenu), 0, (SOCKADDR*)&peeraddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				err_display("recvfrom()");
				continue;
			}
			recvinfo.choosemenu[retval] = '\0';
			if (strcmp(recvinfo.choosemenu, "0") == 0) {
				printf("�Է��Ͻ� ������ �����ϴ�!\n");
				continue;
			}
			else {
				printf("%s", recvinfo.choosemenu);
				break;
			}
		}
		/*�޴������� ���� ���� ����*/
		scanf("%s%*c", sendinfo.menu);

		retval = sendto(sock, sendinfo.menu, sizeof(sendinfo.menu), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			printf("sendto()");
			continue;
		}

		/*�ּ� �Է¹ޱ�*/
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

		//order�� ���� ���� �ޱ�/
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

	// ���� ����
	WSACleanup();
	return 0;
}