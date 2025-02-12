#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")


#define ICMP_ECHO 8

struct IcmpHeader
{
    unsigned char Type;
    unsigned char Code;
    unsigned short Checksum;
    unsigned short Identifier;
    unsigned short SequenceNumber;
};

struct s_ip_hdr // IPv4-пакет
{
#if BIGENDIAN
    unsigned char ip_version : 4; // 4 бита номер версии
    unsigned char hdr_len : 4; // 4 бита длина заголовка
#else
    unsigned char hdr_len : 4; // 4 бита длина заголовка
    unsigned char ip_version : 4;  // 4 бита номер версии
#endif
    unsigned char type_of_service : 8; // 8 бит тип сервиса
    unsigned short total_length : 16; // 16 общая длина
    unsigned short id : 16;  // 16 бит идентификатор  пакета
    unsigned char flags : 3; // 3 бита флаги
    unsigned short fragment_offset : 13;  // 13 бит смещение фрагмента
    unsigned char time_to_live : 8; // 8 бит время жизни
    unsigned char protocol : 8; // 8 бит протокол верхнего уровня
    unsigned short checksum : 16; // 16 бит контрольная сумма
    unsigned int source_ip : 32; // 32 бита адрес источника
    unsigned int dest_ip : 32; // 32 бита адрес назначения

};

unsigned short CalculateChecksum(unsigned short* pBuffer, int size)
{
    ULONG checksum = 0;
    for (int i = 0; i < size / 2; i++)
    {
        checksum += pBuffer[i];
    }
    if (size % 2)
    {
        checksum += ((BYTE*)pBuffer)[size - 1];
    }
    checksum = (checksum >> 16) + (checksum & 0xFFFF);
    return (unsigned short)(~checksum);
}

int ping(const char* ip_address)
{
    WORD wVersionRequested;
    WSADATA wsaData;

    struct sockaddr_in dest_addr_in;

    wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        printf("WSAStartup failed with error: %d\n", err);
        return 0;
    }

    unsigned int dest_ip;
    inet_pton(AF_INET, ip_address, &dest_ip); // создает структуру c сетевым адресом 

    unsigned short dest_port = 0;

    memset(&dest_addr_in, 0, sizeof(struct sockaddr_in));
    dest_addr_in.sin_addr.s_addr = dest_ip;

    dest_addr_in.sin_port = htons(dest_port);
    dest_addr_in.sin_family = AF_INET;
    struct sockaddr* dest_addr = (struct sockaddr*)&dest_addr_in;
    int local_addrlen = sizeof(struct sockaddr_in);

    // Создание raw-сокета
    int sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock_raw, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));


    printf("sock_raw: %d\n", sock_raw);
    printf("sock_create error: %d\n", WSAGetLastError());

    IcmpHeader icmp_hdr;

    // Настройка ICMP-заголовка
    icmp_hdr.Type = ICMP_ECHO;
    icmp_hdr.Code = 0;
    icmp_hdr.Checksum = 0;
    icmp_hdr.Identifier = 0;
    icmp_hdr.SequenceNumber = 1;

    // Вычисление контрольной суммы
    icmp_hdr.Checksum = CalculateChecksum((unsigned short*)&icmp_hdr, sizeof(icmp_hdr));
    
    
    // Отправка ICMP-пакета
    if (sendto(sock_raw, (char*)&icmp_hdr, sizeof(icmp_hdr), 0, dest_addr, local_addrlen) <= 0) {
        printf("ERR sendto");
        closesocket(sock_raw);
        WSACleanup();
        return 1;
    }

    char buffer[1024];
    int bytes_received = recvfrom(sock_raw, buffer, sizeof(buffer), 0, dest_addr, &local_addrlen);


    if (bytes_received < 0) {
        printf("ERR recvfrom");
        closesocket(sock_raw);
        WSACleanup();
        return 1;
    }


    struct s_ip_hdr* ip_header = (struct s_ip_hdr*)buffer;
    struct IcmpHeader* icmp_header = (struct IcmpHeader*)(buffer + (ip_header->hdr_len * 4));
    
 


    if (icmp_header->Type == 0) {
        printf("Received ICMP ECHO REPLY from %s\n", inet_ntoa(dest_addr_in.sin_addr));
    }


    printf("closesocket");
    closesocket(sock_raw);
    WSACleanup();
    return 0;
}

int main()
{

    for (int i = 1; i < 10; i++)
    {
        std::string s = "192.168.1.";
        s = s + std::to_string(i);
        const char* ip_address = s.c_str();
        ping(ip_address);
        Sleep(1000);
    }
    

    return 0;

}
