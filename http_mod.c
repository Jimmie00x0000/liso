#include "http_mod.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "http_response.h"
#include "string.h"

#define BUFFER_SIZE 4096
#define FORK_CHILD_PID 0

http_mod* http_init(uint16_t port) {
    http_mod* m = (http_mod*) malloc(sizeof(http_mod*));
    m->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m->sockfd == -1) {
       fprintf(stderr, "Create socket failed.");
       free(m);
       return NULL;
    }

    m->addr.sin_family = AF_INET;
    m->addr.sin_port = htons(port);
    m->addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_ret = bind(m->sockfd, (struct sockaddr*) &(m->addr), sizeof(m->addr));
    if (bind_ret != 0) {
        fprintf(stderr, "bind to %d failed!", port);
        if(close(m->sockfd)) {
            fprintf(stderr, "Close socked failed!\n");
        }
        free(m);
        return NULL;
    }

    int listen_ret = listen(m->sockfd, 5);
    if (listen_ret != 0) {
        fprintf(stderr, "Failed to listen!\n");
        if(close(m->sockfd)) {
            fprintf(stderr, "Close socked failed!\n");
        }
        free(m);
        return NULL;
    }
    fprintf(stdout, "start http mod successfully! \n"); 

    return m;
}


void handle_request_loop(int sock_fd) {
    int read_ret = 0;
    uint8_t buffer[BUFFER_SIZE];
    http_response *hr = create_temp_hr();
    char databuffer[200];
    strcpy(databuffer, "This is a test. \n");
    while (read_ret = recv(sock_fd, buffer, BUFFER_SIZE, 0) >= 1) {
        fprintf(stdout, "%s", buffer);
        fprintf(stdout, "###############################\n"); 
        // write(sock_fd, databuffer, 17);
        write_http_response(sock_fd, hr);
    }
}

int start_receive_conn(http_mod *http) {
    struct sockaddr_in client_sock_addr;
    socklen_t cs_size = sizeof(client_sock_addr);
    fprintf(stdout, "Waiting for a connection, localport: %d\n", ntohs(http->addr.sin_port));

    while (1) {
        int new_sock = accept(http->sockfd, (struct sockaddr*)(&client_sock_addr), &cs_size);
        if (new_sock == -1) {
            fprintf(stderr, "Accept connections failed!\n");
            if (close(http->sockfd)) {
                fprintf(stderr, "Close socket failed!\n");
            }
            return -1;
        } else {
            fprintf(stdout, "Receive connection from %s\n", inet_ntoa(client_sock_addr.sin_addr));
        }
        if (fork() == FORK_CHILD_PID) {
            handle_request_loop(new_sock); 
            if (close(new_sock)) {
                fprintf(stderr, "Close real sock failed! \n'");
            }
            fprintf(stdout, "Close connection from %s", inet_ntoa(client_sock_addr.sin_addr));
            exit(0);
        }

        if (close(new_sock)) {
            fprintf(stderr, "Close real sock failed! \n'");
            return -1;
        }

    }

    if (close(http->sockfd)) {
        fprintf(stderr, "Close sock failed! \n'");
    }
    fprintf(stdout, "Liso has stopped.\n");
    return 0;
}