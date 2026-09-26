/*
 * server.h - shared definitions for the web server
 */

#ifndef SERVER_H
#define SERVER_H

#define BUFSIZE 2048

/* parts of a req: "GET /index.html HTTP/1.1 Host: ..." */
typedef struct request {
  char method[BUFSIZE];
  char filename[BUFSIZE];
  char protocol[BUFSIZE];
  char host[BUFSIZE];


  int nargs;      /* how many args the client sent */
  int keep_alive; /* http.1: keep reading requests on this connection */
  int malformed;  /* serve the 400s */
} request;

/* directory files are served from */
extern const char *doc_root;

/* connection.c: new threads start here, one per client */
void *run_thread(void *vargp);

/* request.c: reading and checking what the client sent */
int read_request(int connfd, char buf[BUFSIZE]);
void parse_request(char buf[BUFSIZE], request *req);

/* response.c: building and sending resp */
void send_response(request *req, int connfd);

#endif