#ifndef SERVER_H
#define SERVER_H

#define BUFSIZE 2048


/* GET /index.html HTTP/1.1 Host..*/
typedef struct request
{
    char method[BUFSIZE];
    char filename[BUFSIZE];
    char protocol[BUFSIZE];
    char host[BUFSIZE];

    int keep_alive; /* HTTP/1.1 vs HTTP/1.0*/
    int malformed; /* serve 400s */

} request;


/* request */



#endif