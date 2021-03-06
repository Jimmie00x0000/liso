#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "stdio.h"


typedef struct http_response_header{

    uint32_t status_code;
    char date[35];
    uint32_t content_length;
    char *reason_phrase;
    char *content_type;
    char *content_encoding;
    char *server;
    char *connection;
    char *transfer_encoding;
} http_response_header;

typedef struct http_response_body{
    int res_fd;
    //char data[100];
} http_response_body;

typedef struct http_response{
    http_response_header header;
    http_response_body body;

} http_response;

http_response *create_temp_hr(); 

 size_t write_http_header(FILE *fp, http_response_header *header);

 size_t write_http_body(FILE *fp, http_response_body *header);

int write_http_response(int fd, http_response *header);

const char *get_current_time(); 

#endif
