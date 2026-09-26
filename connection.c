/*
 * connection.c - what each client's thread does, from its first request until it disconnects
 * HTTP1.0 gets one request; HTTP1.1 loops for more
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/socket.h>

#include "server.h"

/* number of open connections, used to scale the HTTP1.1 timeout */
static atomic_int conncnt = 0;

/* waits up to 1 minute (divided by the number of connections) for the client's next request
 * on a timeout, tells the client and returns 0; otherwise returns 1
 */
static int wait_for_request(int connfd) {
  struct pollfd pfd = { .fd = connfd, .events = POLLIN }; /* POLLIN: data is ready to read */
  int n = conncnt;

  if (poll(&pfd, 1, 60000 / (n > 0 ? n : 1)) == 0) {
    send(connfd, "408 Request Timeout\r\n", 21, 0);
    return 0;
  }
  return 1;
}

/* reads, checks and answers requests until the client is done, it times out, or a read fails */
static void handle_connection(int connfd) {
  char buf[BUFSIZE]; /* request buffer */
  request req;       /* the current request, split into its parts */
  int first = 1;     /* is this the first request on the connection? */

  do {
    /* the timeout only applies while waiting for the second request onwards */
    if (!first && !wait_for_request(connfd)) {
      return;
    }
    first = 0;

    /* eliminate what's left from the previous request */
    bzero(buf, BUFSIZE);
    bzero(&req, sizeof(req));

    if (read_request(connfd, buf) < 0) {
      return;
    }
    parse_request(buf, &req);
    send_response(&req, connfd);
  } while (req.keep_alive);
}

/* new threads will start execution in this function */
void *run_thread(void *vargp) {
  int connfd = *((int *)vargp); /* nasty pointer casting. this is our client fd */

  /* detach this thread from parent thread */
  if (pthread_detach(pthread_self()) != 0) {
    printf("error detaching\n");
  }

  /* free heap space for vargp since we have connfd */
  free(vargp);

  conncnt++;
  handle_connection(connfd);

  /* close client */
  shutdown(connfd, SHUT_RD);
  close(connfd);
  conncnt--;
  return NULL;
}