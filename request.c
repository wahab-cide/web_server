/*
 * request.c - reading a request off the socket, splitting it up, and checking it's valid
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "server.h"

/* has the client finished its request? it ends with a double-carriage return (mac or windows) */
static int has_blank_line(char buf[BUFSIZE]) {
  return strstr(buf, "\r\n\r\n") != NULL || strstr(buf, "\r\r") != NULL || strstr(buf, "\n\n") != NULL;
}

/* recv: reads from the client into buf (which must start zeroed) until the request is complete
 * returns 0 once it is, -1 on a read error, a disconnect, or a request too big for buf
 */
int read_request(int connfd, char buf[BUFSIZE]) {
  int total_read = 0; /* bytes in buf so far */
  int num_read;       /* num bytes read by this recv */

  while (!has_blank_line(buf)) {
    /* leave the last byte as the string terminator */
    if (total_read >= BUFSIZE - 1) {
      printf("ERROR request too large\n");
      return -1;
    }
    num_read = recv(connfd, buf + total_read, BUFSIZE - 1 - total_read, 0);
    if (num_read < 0) {
      printf("ERROR reading from socket\n");
      return -1;
    }
    if (num_read == 0) {
      /* client closed the connection */
      return -1;
    }
    total_read += num_read;
    printf("server received %d bytes: %s\n", num_read, buf);
  }
  return 0;
}

/* splits the request into its parts by a space delimiter, then decides whether it's
 * HTTP1.1 (keep_alive) and whether it's malformed
 * HTTP1.1 has four arguments (it needs the host argument sent by firefox) and HTTP1.0 has only three
 * note: strtok modifies buf
 */
void parse_request(char buf[BUFSIZE], request *req) {
  char *token;

  /* strtok() needs NULL on repeated calls to return the next substring, and returns NULL once it's done */
  for (token = strtok(buf, " \r\n"); token != NULL; token = strtok(NULL, " \r\n")) {
    switch (req->nargs) {
      case 0: strcpy(req->method, token); break;
      case 1: strcpy(req->filename, token); break;
      case 2: strcpy(req->protocol, token); break;
      case 3: strcpy(req->host, token); break;
    }
    req->nargs++;
  }

  req->keep_alive = !strcmp(req->protocol, "HTTP/1.1");
  if (req->keep_alive) {
    req->malformed = req->nargs < 4 || strcmp(req->method, "GET") || strcmp(req->host, "Host:");
  } else {
    req->malformed = req->nargs < 3 || strcmp(req->method, "GET") || strcmp(req->protocol, "HTTP/1.0");
  }
}