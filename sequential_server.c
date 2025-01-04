#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

typedef enum { WAIT_FOR_CLIENT, WAIT_FOR_MSG, IN_MSG } ProcessingState;
ProcessingState STATE;
int portnumber;

void serve_connection(int sockfd){
    int bytes_sent = send(sockfd, "*", 1, 0);
    if(bytes_sent != 1){
        perror("Error sending");
        EXIT_FAILURE;
    }
    STATE = WAIT_FOR_MSG;
    while(1){
        int8_t buf[1024];
        int len = recv(sockfd, buf, sizeof buf, 0);
        if(len < 0) {
            perror("Error receiving");
            EXIT_FAILURE;
        }else if(len == 0){
            break;
        }
        for(int i = 0; i < len; i++){
            if(STATE == WAIT_FOR_MSG){
                if(buf[i] == '^'){
                    STATE = IN_MSG;
                }
            }else if(STATE == IN_MSG){
                if(buf[i] == '$'){
                    STATE = WAIT_FOR_MSG;
                }else{
                    int8_t incremented_byte = buf[i] + 1;
                    int bytes_sent = send(sockfd, &incremented_byte,1,0);
                    if(bytes_sent != 1){
                        perror("Error sending");
                        closesocket(sockfd);
                        EXIT_FAILURE;
                    }
                }
                break;
            }
        }
        closesocket(sockfd);
    }
}

SOCKET listen_inet_socket(int portnumber) {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        perror("ERROR creating socket");
        EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on any network interface
    server_addr.sin_port = htons(portnumber);     // Set the port number

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("ERROR on bind");
        EXIT_FAILURE;
    }

    if (listen(sockfd, SOMAXCONN) == SOCKET_ERROR) {
        perror("ERROR on listen");
        EXIT_FAILURE;
    }

    printf("Server listening on port %d\n", portnumber);
    return sockfd;
}

void report_peer_connected(struct sockaddr_in* peer_addr, int addr_len) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr->sin_addr, ip, sizeof(ip));
    printf("Peer connected from %s:%d\n", ip, ntohs(peer_addr->sin_port));
}



int main(int argc, char** argv) {
    if (argc >= 2) {
        portnumber = atoi(argv[1]);
    }else{
        portnumber = 0;
    }

    SOCKET sockfd = listen_inet_socket(portnumber);
    while(1){
        struct sockaddr_in peer_addr;
        int addr_len = sizeof(peer_addr);
        SOCKET newsockfd = accept(sockfd, (struct sockaddr*)&peer_addr, &addr_len);
        if (newsockfd == INVALID_SOCKET) {
            perror("ERROR on accept");
            break;
        }
        STATE = WAIT_FOR_CLIENT;

        report_peer_connected(&peer_addr, addr_len);
        serve_connection(newsockfd);
        printf("Peer done\n");

        closesocket(newsockfd);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}

