#ifndef HTTP_DF_H
#define HTTP_DF_H

#define CRLF \r\n
#define MAX_HEADER_BYTES 8192

#define IMAGE_PNG "image/png"
#define IMAGE_JPEG "image/jpeg"
#define IMAGE_GIF "image/gif"
#define TEXT_HTML "text/html"
#define TEXT_CSS "text/css"
#define APPLICATION_JS "application/javascript"
#define APPLICATION_UNKNOWN "application/octet-stream"

#define STR_EQUAL(A,B) (strcmp(A, B) == 0)

typedef struct http_request_ {
    char method[10];
    char res[MAX_HEADER_BYTES];// requested resource location
    char host[20]; 
    char http_version[4];
    char connection[20];
    
}http_request;

#endif
