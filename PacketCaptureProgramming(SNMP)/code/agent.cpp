/* File Name: agent
 * Author: Kim Soo Young
 * E-mail Address: youworthit17@ajou.ac.kr
 * Description:(�����۵�) packet capture �Ŀ� mib tree���� & TCP ������� �Ŵ������� mib ���� ���� device ����
 (�����۵�)UDP ������� �Ŵ������� TCP ��� ������ �ڽ��� ������ �����ϰ�, syn packet attck�� �߻��ߴٴ°��� �˸��� ����(Gu Gyo Hyean)
 * Last Changed: 2020-06-09
*/


#ifdef _MSC_VER
/*
 * we do not want the warnings about the old deprecated and unsecure CRT functions
 * since these examples can be compiled under *nix as well
 */
#define _CRT_SECURE_NO_WARNINGS
#endif
#pragma warning(disable:4996)
#include <pcap.h>
#include <pcap/pcap.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <ctime>
#include<string>
#include<cstring>
#include<malloc.h>
#include<Windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUFSIZE 1500
#define INPUTSIZE 1000 
#define ETHERTYPE_IP      0x0800
#define ETH_II_HSIZE      14      // frame size of ethernet v2
#define ETH_802_HSIZE      22      // frame size of IEEE 802.3 ethernet
#define IP_PROTO_IP          0      // IP
#define IP_PROTO_TCP      6      // TCP
#define IP_PROTO_UDP      17      // UDP
#define  RTPHDR_LEN         12      // Length of basic RTP header
#define CSRCID_LEN         4      // CSRC ID length
#define   EXTHDR_LEN         4      // Extension header length

#define ACK 0x10
#define SYN 0x02


int net_ip_count;
int net_etc_count;
int trans_tcp_count;
int trans_udp_count;
int trans_etc_count;
int if_tcp_syn;
int whole_packet_count;
int recvmsglen = 0;
int alert = 0;
int port = 0;


// Macros
// pntohs : to convert network-aligned 16bit word to host-aligned one
#define pntoh16(p)  ((unsigned short)                       \
                    ((unsigned short)*((unsigned char *)(p)+0)<<8|  \
                     (unsigned short)*((unsigned char *)(p)+1)<<0))

// pntohl : to convert network-aligned 32bit word to host-aligned one
#define pntoh32(p)  ((unsigned short)*((unsigned char *)(p)+0)<<24|  \
                    (unsigned short)*((unsigned char *)(p)+1)<<16|  \
                    (unsigned short)*((unsigned char *)(p)+2)<<8|   \
                    (unsigned short)*((unsigned char *)(p)+3)<<0)

char* iptos(u_long in);

//packet capture���Ŀ� mib tree�� �����ϱ� ���ؼ� �ش� ������ � �������� �Ľ��ϴ� �ܰ�

typedef struct ip_address
{
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;

}ip_address;

typedef struct ip_addresses {
    ip_address source_ip_address;
    ip_address dest_ip_address;
} ip_addresses;

ip_addresses* IP;

typedef struct udp_header
{
    u_short sport;   // Source port
    u_short dport;   // Destination port

}udp_header;

typedef struct tcp_header
{
    u_short sport;   // Source port
    u_short dport;   // Destination port
    u_int seqnum;   // Sequence Number
    u_int acknum;   // Acknowledgement number
    u_char offset;  // Header length
    u_char flags;   // packet flags
    u_short win;    // Window Size
    u_short crc;    // Header Checksum
    u_short urgptr; // Urgent pointer

}tcp_header;


// call back function //packet capture loopback
void    get_stat(u_char* user, const struct pcap_pkthdr* h, const u_char* p);

// UDP alert
DWORD WINAPI alert_by_udp(LPVOID arg);

int       numpkt_per_sec, numbyte_per_sec;   // packet and byte rates per second
int       tot_capured_pk_num = 0;              // total number of captured packets
long   crnt_sec, prev_sec;                  // time references in second
pcap_t* adhandle;                           // globally defined for callback fucntion

int whole_packet_number = 0;

typedef struct CodeAndValue {
    char code[6];
    float value;
}codeandvalue;

//snmp �������� ������ ����ü syntax
//��ȯ�� pdu type
typedef struct mibStructure {
    //pdu type response ������ �����̱� ������ type �� 2�� ����
    int pdu_type = 2;
    codeandvalue array[5];
};

//========================================================

//mib tree�� �� ����
//"1.1.1""1.2.1"����� ��Ÿ���� ���� degree�� �ȿ� ���� value�� data�� ����
typedef struct mibtreenode {
    int degree;
    float data;
    struct mibtreenode* child;
}Node;

typedef struct mibtree {
    Node* root;
}mibTree;

//tree�� ���ο� ��带 ����� �κ�
Node* newNode(int data) {
    Node* returnNode = (Node*)malloc(sizeof(Node));
    returnNode->data = data;
    returnNode->degree = 0;
    returnNode->child = NULL;
    return returnNode;
}

//mib tree�� ���鶧 ��� --> ���� snmp ������ mib tree�� �������� �������� 
//mib tree�� packet capture�� �ϸ鼭 ���� mib tree�� ���� ������� Ȱ��
//manager���� response�� ������ mib tree�ȿ� �ִ� ���� reset
mibTree* newTree() {
    mibTree* returnTree = (mibTree*)malloc(sizeof(mibTree));
    returnTree->root = NULL;
    return returnTree;
}

mibTree* tree_for_mib;

typedef struct UDP_PROTO {
    int type;
    int port;
}UDP_PROTO;

//mib tree���� �� �ڵ忡 �ش��ϴ� value�� �־��ٶ� ����ϴ� �Լ�
//parent node pointer�� �ȿ� ���� ���� data ���� ����.
int  ParentInsertChild(Node* p, int data) {
    p->child = (Node*)realloc(p->child, sizeof(Node) * (p->degree + 1));
    p->child[p->degree].data = data;
    p->child[p->degree].degree = 0;
    p->child[p->degree].child = NULL;
    p->degree++;

    return 1;
}
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


//tcp connection�� �α� ���� tcp client���� tcp server�� agent ������ �˷��ִ� thread�Լ�
//������ port�� �� �κ��� �Ϲ� udp ��� �κ����� syn flooding ���� �˷��ֱ� ���ؼ� type�� �߰�
//input : UDP_PROTO ����ü ���� 
DWORD WINAPI UDP_call(LPVOID arg) {
    UDP_PROTO* params = (UDP_PROTO*)arg;
    int retval;
    SOCKADDR_IN peeraddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        err_quit((char*)"socket()"); return 0;
    }

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8000);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    retval = sendto(sock, (char*)params, sizeof(UDP_PROTO), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) {
        err_display((char*)"sendto()");
        return 0;
    }

    addrlen = sizeof(peeraddr);
    retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peeraddr, &addrlen);
    if (retval == SOCKET_ERROR) {
        err_display((char*)"recvfrom()");
        return 0;
    }

    if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
        printf("[����] �߸��� �������Դϴ�!\n");
        return 0;
    }

    printf("connection complete");
    closesocket(sock);
    return 0;
}

//capture �� syn flooding �� �߻������� ����Ǵ� thread�Լ�
//capture thread���� tcp syn ������ 1500�� �̻��� �ɶ� �� thread �Լ��� ����
//���� ������� input : type �� port �� ���� �̶� type�� 0���� ����
DWORD WINAPI UDP_alert(LPVOID arg) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8000);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    SOCKADDR_IN peeraddr;
    int addrlen;
    char buf[BUFSIZE + 1];
    int len;
    int retval;

    UDP_PROTO* params = (UDP_PROTO*)arg;

    retval = sendto(sock, (char*)params, sizeof(UDP_PROTO), 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) {
        err_display((char*)"sendto()");
        return 0;
    }
    free(params);

    addrlen = sizeof(peeraddr);
    retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peeraddr, &addrlen);
    if (retval == SOCKET_ERROR) {
        err_display((char*)"recvfrom()");
        return 0;
    }

    if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
        printf("[����] �߸��� �������Դϴ�!\n");
        return 0;
    }

    printf("alert message is transimit");
    closesocket(sock);
    return 0;
}

//paket capture�� �����ϴ� thread�Լ�
//get_stat callback�� �θ���
DWORD WINAPI Capture(LPVOID arg) {

    pcap_if_t* alldevs;
    pcap_if_t* d;

    char                errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr* pkt_hdr;    // captured packet header
    const u_char* pkt_data;   // caputred packet data
    time_t              local_tv_sec;
    struct tm* ltime;
    char                timestr[16];

    int      i, ret;         // for general use
    int      ndNum = 0;   // number of network devices
    int      devNum;      // device Id used for online packet capture


  /* Retrieve the device list */
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    /* Print the list */
    printf("\n");
    pcap_addr_t* a;
    for (d = alldevs; d; d = d->next)
    {
        // device name
        printf(" [%d] %s", ++ndNum, d->name);

        // description
        if (d->description)
            printf(" (%s) ", d->description);

        for (a = d->addresses; a; a = a->next) {
            if (a->addr->sa_family == AF_INET) {
                if (a->addr)
                    printf("[%s]", iptos(((struct sockaddr_in*)a->addr)->sin_addr.s_addr));
         
                break;
            }
        }
        printf(" flag=%d\n", (int)d->flags);
    }
    printf("\n");

    if (ndNum == 0)
    {
        printf("\nNo interfaces found! Make sure Npcap is installed.\n");
        return -1;
    }

    /* select device for online packet capture application */
    printf(" Enter the interface number (1-%d):", ndNum);
    scanf("%d", &devNum);

    /* select error ? */
    if (devNum < 1 || devNum > ndNum)
    {
        printf("\nInterface number out of range.\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return -1;
    }

    /* Jump to the selected adapter */
    for (d = alldevs, i = 0; i < devNum - 1; d = d->next, i++);

    /* Open the adapter */
    if ((adhandle = pcap_open_live(d->name, // name of the device
        65536,     // portion of the packet to capture. 
               // 65536 grants that the whole packet will be captured on all the MACs.
        1,         // promiscuous mode
        1000,      // read timeout
        errbuf)     // error buffer
        ) == NULL)
    {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", d->name);
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("\n Selected device %s is available\n\n", d->description);
    pcap_freealldevs(alldevs);

    //���Ⱑ udp ��û ������.
    HANDLE         hThread;
    DWORD         ThreadId;

    UDP_PROTO params;
    params.port = port;
    params.type = 1;

    //���� �Ϲ� udp ����� �ϴ� thread����
    //manager���� �ڽ��� ������ ����
    hThread = CreateThread(NULL, 0, UDP_call, (LPVOID)&params, 0, &ThreadId);
    if (hThread == NULL)
        err_display((char*)"error: failure of thread creation!!!");
    else
        CloseHandle(hThread);

    // start the capture
    numpkt_per_sec = numbyte_per_sec = net_ip_count = trans_tcp_count = trans_udp_count = if_tcp_syn = 0;
    pcap_loop(adhandle,    // capture device handler
        -1,          // forever
        get_stat,   // callback function
        NULL);      // no arguments

    return 0;
}

int main(int argc, char* argv[])
{
    // ������ ��ſ� ����� ����
    SOCKET listen_sock;
    SOCKET client_sock;
    SOCKADDR_IN serveraddr;
    SOCKADDR_IN clientaddr;
    int      addrlen;
    char   buf[BUFSIZE + 1];
    int      retval, msglen;
    int retval_send = -1;
    HANDLE         hThread;
    DWORD         ThreadId;

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    // socket() //listen socket ����
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit((char*)"fail to make listen socket");

    printf("port ��ȣ�� �Է��ϼ��� ~ ");
    scanf("%d", &port);

    // server address 
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind() //��ü interface ���� bind
    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

    // listen() 
    retval = listen(listen_sock, SOMAXCONN); //SOMAXCONN
    if (retval == SOCKET_ERROR) err_quit((char*)"listen()");

    //online capture start
    // thread for capture
    hThread = CreateThread(NULL, 0, Capture, NULL, 0, &ThreadId);
    if (hThread == NULL)
        err_display((char*)"error: failure of thread creation!!!");
    else
        CloseHandle(hThread);

    //parsing ���� ������ ������ tree ����
    //���ο� tree ����
    tree_for_mib = newTree();
    tree_for_mib->root = newNode(0);

    //IPnode ����
    ParentInsertChild(tree_for_mib->root, 0);
    //�̷��� root �� degree�� �ϳ� �ö󰡰� �ȴ�.
    //TCP node ����
    ParentInsertChild(tree_for_mib->root, 0);
    //UDP node ����
    ParentInsertChild(tree_for_mib->root, 0);

    //��ü mib tree �ʱ�ȭ
    //�ʱ�ȭ �ϴ� ���� : tcp client ���� �ٷ� ������ ���ڸ��� request �� ������ response�� ��� ������
    //ó���� packet capture ���� �ʴ� ���¿��� �ƿ� ����ִ� mib tree ����

    tree_for_mib->root->child[0].degree = 0;
    ParentInsertChild((Node*)&tree_for_mib->root->child[0], (float)net_ip_count);
    //tcp packet ���� tcp �� ��ƾ��Ҷ�
    tree_for_mib->root->child[1].degree = 0;
    ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)trans_tcp_count);
    //tcp syn packet ���� tcp�� ��ƾ� �Ҷ�
    ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)if_tcp_syn);
    //��Ʈ�� ����ؼ� �ִ� ���
    //manager ���� request 5�ʿ� �ѹ�
    //��ü ��Ŷ�� / �ð� ���� //tot_capured_pk_num
    ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)tot_capured_pk_num / 5);
    tree_for_mib->root->child[2].degree = 0;
    //udp packet ���� �ִ°�
    ParentInsertChild((Node*)&tree_for_mib->root->child[2], (float)trans_udp_count);


    int firsttime = 1;
    while (1) {

        // accept() //manager accetp 
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display((char*)"accept()");
            continue;
        }

        printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        while (1) {
            // request ���� �ޱ�
            msglen = 0;
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if (msglen == SOCKET_ERROR) {
                err_display((char*)"recv()");
                break;
            }
            //request ������ ���� �ʾ�����
            else if (msglen == 0)
                break;

            buf[msglen] = '\0';
            recvmsglen = 1;
            //tree�� �����Ȱ� ��ȯ�� pdu type structrue �� �ֱ�

            mibStructure sendMibStructure;

            sendMibStructure.pdu_type = 2;

            memcpy(sendMibStructure.array[0].code, "1.1.1", 6);
            sendMibStructure.array[0].value = tree_for_mib->root->child[0].child[0].data;
            memcpy(sendMibStructure.array[1].code, "1.2.1", 6);
            sendMibStructure.array[1].value = tree_for_mib->root->child[1].child[0].data;
            memcpy(sendMibStructure.array[2].code, "1.2.2", 6);
            sendMibStructure.array[2].value = tree_for_mib->root->child[1].child[1].data;
            memcpy(sendMibStructure.array[3].code, "1.2.3", 6);
            sendMibStructure.array[3].value = tree_for_mib->root->child[1].child[2].data;
            memcpy(sendMibStructure.array[4].code, "1.3.1", 6);
            sendMibStructure.array[4].value = tree_for_mib->root->child[2].child[0].data;

            //response message
            retval = send(client_sock, (char*)&sendMibStructure, sizeof(sendMibStructure), 0);

            recvmsglen = 0;

            if (retval == SOCKET_ERROR) {
                err_display((char*)"send()");
                break;
            }

            //�ٽ� mib�� ���� ����ڷ� reset
            net_ip_count = 0;
            trans_tcp_count = 0;
            if_tcp_syn = 0;
            tot_capured_pk_num = 0;
            trans_udp_count = 0;
            alert = 0;

        }

        // closesocket()
        closesocket(client_sock);
        printf("[TCP Server] connection terminated: client (%s:%d)\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    }
    // closesocket() //listen socket �� ����
    closesocket(listen_sock);
    pcap_close(adhandle);

    // ���� ����
    WSACleanup();
    return 0;
}



/* From tcptraceroute, convert a numeric IP address to a string : source Npcap SDK */
#define IPTOSBUFFERS   12
char* iptos(u_long in)
{
    static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
    static short which;
    u_char* p;

    p = (u_char*)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}

//packet capture ���������� ����
void get_stat(u_char* user, const struct pcap_pkthdr* h, const u_char* p)
{
    HANDLE         hThread;
    DWORD         ThreadId;

    if (recvmsglen == 0) {

        struct tm* ltime;
        char timestr[16];
        unsigned short type;
        time_t local_tv_sec;


        // convert the timestamp to readable format
        local_tv_sec = h->ts.tv_sec;
        ltime = localtime(&local_tv_sec);
        strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);
        USHORT tlen;


        // check time difference in second
        //<No> <Time> <Source IP> <Dest. IP> <Type> <Protocol> <Length> <Info>
        crnt_sec = h->ts.tv_sec;
        if (tot_capured_pk_num == 0) prev_sec = crnt_sec;
        if (crnt_sec > prev_sec) {

            whole_packet_number++;
            printf("%d ", whole_packet_number);

            IP = (ip_addresses*)(&p[14 + 12]);
            printf("%d.%d.%d.%d ",
                IP->source_ip_address.byte1, IP->source_ip_address.byte2, IP->source_ip_address.byte3
                , IP->source_ip_address.byte4);
            printf("%d.%d.%d.%d ",
                IP->dest_ip_address.byte1, IP->dest_ip_address.byte2, IP->dest_ip_address.byte3
                , IP->dest_ip_address.byte4);
            printf("%04x ", pntoh16(&p[12]));


            if (p[23] == IP_PROTO_UDP) {
                printf("UDP ");
                tlen = (USHORT)&p[16];
                printf("%d ", tlen);
                udp_header* udp = (udp_header*)&p[14 + p[14] & 0x0f * 4];
                printf("%d -> ", ntohs(udp->sport));
                printf("%d ", ntohs(udp->dport));

            }
            else if (p[23] == IP_PROTO_TCP) {
                printf("TCP ");
                tlen = (USHORT)p[16];
                printf("%d ", tlen);
                tcp_header* tcp = (tcp_header*)&p[14 + p[14] & 0x0f * 4];
                printf("%d -> ", ntohs(tcp->sport));
                printf("%d ", ntohs(tcp->dport));

            }
            else {
                printf("not TCP/UDP ");
                tlen = p[16];
                printf("%d ", tlen);
                printf("not TCP/UDP");
            }
            printf("\n");

            //numpkt_per_sec = numbyte_per_sec = net_ip_count = trans_tcp_count = trans_udp_count = 0;

        }
        prev_sec = crnt_sec;
        numpkt_per_sec++;
        numbyte_per_sec += h->len;

        //<No> <Time> <Source IP> <Dest. IP> <Type> <Protocol> <Length> <Info>

        //protocol �� ip�� ���
        if ((type = pntoh16(&p[12])) == 0x0800) {

            net_ip_count++;
            if (p[23] == IP_PROTO_UDP) {
                trans_udp_count++;
            }
            else if (p[23] == IP_PROTO_TCP) {
                trans_tcp_count++;
                tcp_header* tcp = (tcp_header*)&p[14 + (p[14] & 0x0f) * 4];
                if (tcp->flags == SYN) {

                    if (ntohs(tcp->dport) == port) { if_tcp_syn++; }
                    //Syn flooding check �ܼ��ϰ� �׳� ������ ���� //1500�� �̻� syn flooding attack��� ����
                    //UDP_PROTO �������� ������ �Է��� ��Ʈ ��ȣ�� ���, type�� 0���� ����
                    if (if_tcp_syn >= 1500 && alert == 0) {
                        UDP_PROTO* params = (UDP_PROTO*)malloc(sizeof(UDP_PROTO));
                        params->port = port;
                        params->type = 0;
                        hThread = CreateThread(NULL, 0, UDP_alert, (LPVOID)params, 0, &ThreadId);
                        if (hThread == NULL)
                            err_display((char*)"error: failure of thread creation!!!");
                        else
                            CloseHandle(hThread);

                        alert = 1;
                    }
                }
            }
            else
                trans_etc_count++;

        }

        tot_capured_pk_num++;

        //mib tree �ʱ�ȭ & ����
        tree_for_mib->root->child[0].degree = 0;
        ParentInsertChild((Node*)&tree_for_mib->root->child[0], (float)net_ip_count);
        //tcp packet ���� tcp �� ��ƾ��Ҷ�
        tree_for_mib->root->child[1].degree = 0;
        ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)trans_tcp_count);
        //tcp syn packet ���� tcp�� ��ƾ� �Ҷ�
        ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)if_tcp_syn);
        //��Ʈ�� ����ؼ� �ִ� ���
        //manager ���� request 5�ʿ� �ѹ�
        //��ü ��Ŷ�� / �ð� ���� //tot_capured_pk_num
        ParentInsertChild((Node*)&tree_for_mib->root->child[1], (float)tot_capured_pk_num / 5);
        tree_for_mib->root->child[2].degree = 0;
        //udp packet ���� �ִ°�
        ParentInsertChild((Node*)&tree_for_mib->root->child[2], (float)trans_udp_count);

     
    }
}