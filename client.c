#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "config.h"
#include "world.h"
#include "server.h"
#include "player.h"
int main() { // client application

    // create a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);  // socket is created

    // create address structure, specify address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // casting to a proper structure sockaddr
    int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    if (connection_status == -1) {
        printf("Error connecting to a remote socket\n\n");
        return 0;
    }

    // receive data from the server
    char server_response[256];
    recv(network_socket, &server_response, sizeof(server_response), 0);

    // print out server's response
    printf("%s\n", server_response);
    char buffer[1024] = "MOV|1";

    // send data to server
    send(network_socket, buffer, sizeof(buffer), 0);

    usleep(20000000);

    close(network_socket);
    return 0;
}