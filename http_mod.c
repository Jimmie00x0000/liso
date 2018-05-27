#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "http_mod.h"
#include "http_response.h"
#include "http_request.h"
#include "http_df.h"

#define BUFFER_SIZE 4096
#define FORK_CHILD_PID 0

const char *PATH_404 = "./html/404.html";
const char *PATH_500 = "./html/500.html";
extern http_response hr_200;
extern http_response hr_404;
extern http_response hr_500;

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

static void recognize_content_type(const char *file_name, http_response *hr) {
    int end = strlen(file_name);
    int point_pos = -1;
    if (end < 1 || end >= MAX_HEADER_BYTES) {
        fprintf(stderr, "Something wrong when recognizing content type.\n");
        return;
    }
    for (int i = end - 1; i >= 0; i --) {
        if (file_name[i] == '.') {
            point_pos = i;
            break;
        }
    }
    if (point_pos == -1) {
        return; // cannot guess file type
    }
    static char suffix[10]; 
    strcpy(suffix, file_name + point_pos + 1);
    if (STR_EQUAL(suffix, "png")) {
        hr->header.content_type = IMAGE_PNG;
    } else if (STR_EQUAL(suffix, "jpg") 
        || STR_EQUAL(suffix, "jpeg")) {
        hr->header.content_type = IMAGE_JPEG;
    } else if (STR_EQUAL(suffix, "gif")) {
        hr->header.content_type = IMAGE_GIF;
    } else if (STR_EQUAL(suffix, "html") 
        || STR_EQUAL(suffix, "htm")) {
        hr->header.content_type = TEXT_HTML;
    } else if (STR_EQUAL(suffix, "css")) {
        hr->header.content_type = TEXT_CSS;
    } else if (STR_EQUAL(suffix, "js")) {
        hr->header.content_type = APPLICATION_JS;
    } else {
        hr->header.content_type = APPLICATION_UNKNOWN;
    }
}

http_request *parse_request(char *str) {
    http_request *req = (http_request *) malloc(1 * sizeof(http_request));
    /** 
     * When I have not used flex, use a stupid method to find the resources
     * that browser is requesting.
     **/
    int start = 0, end = 0; // start is '/'s localtion, end is first ' 's location
    for (start = 0; start < BUFFER_SIZE; start ++) {
        if (str[start] == '/') {
            break;
        }
    } 
    if (start == BUFFER_SIZE) {
        fprintf(stderr, "Cannot find resource.\n");
        return req;
    }
    for (end = start; end < BUFFER_SIZE; end ++) {
        if (str[end] == ' ') {
            break;
        } 
    }
    memcpy(req->res, str + start, (end - start));
    req->res[end - start] = 0; 
    return req;
}

http_response *gen_hr(http_request* hrq) {
    http_response *result = (http_response *) malloc(1 * sizeof(http_response));
    char filename[MAX_HEADER_BYTES];
    struct stat file_stat;
    strcpy(filename, "/var/www/html");
    strcat(filename, hrq->res);
    //TODO: support multiple index file
    //TODO: suport config from file
    if (filename[strlen(filename) - 1] == '/') {strcat(filename, "index.html");}

    fprintf(stderr, "Res name: %s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        memcpy(result, &hr_404, sizeof(http_response));
        fprintf(stderr, "status code: %d\n", result->header.status_code);
        fp = fopen(PATH_404, "r");
    } else {
        memcpy(result, &hr_200, sizeof(http_response));
    }

    result->body.res_fd = fileno(fp); 

    fstat(fileno(fp), &file_stat);
    strcpy(result->header.date, get_current_time());
    recognize_content_type(filename, result);
    result->header.content_length = file_stat.st_size;

    return result;
}

void handle_request_loop(int sock_fd) {
    int read_ret = 0;
    uint8_t buffer[BUFFER_SIZE];
    http_response *hr = create_temp_hr();
    while (read_ret = recv(sock_fd, buffer, BUFFER_SIZE, 0) >= 1) {
        fprintf(stdout, "%s", buffer);
        fprintf(stdout, "###############################\n"); 
        http_request *rq = parse_request(buffer);
        http_response * res = gen_hr(rq);
        // write_http_response(sock_fd, hr);
        write_http_response(sock_fd, res);
        free(rq);
        free(res);
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