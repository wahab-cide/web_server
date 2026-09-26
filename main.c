/*
 * main.c - A simple HTTP web server
 * every client that connects gets its own thread running run_thread (connection.c)
 * usage: server -document_root <filepath> -port <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "server.h"

const char *doc_root;

/* check_flags: returns 1 if the usage is correct for ./server, 0 if not */
static int check_flags(char **argv) {
  return !strcmp(argv[1], "-document_root") &&
         !strcmp(argv[3], "-port") &&
         atoi(argv[4]) > 0;
}

int main(int argc, char **argv) {
  int listenfd;        /* listening socket */
  int *connfd;         /* connection socket */
  int portno;          /* port to listen on */
  socklen_t clientlen; /* byte size of client's address */
  pthread_t tid;       /* thread id */
  int optval;

  struct sockaddr_in myaddr;  /* my ip address info */
  struct sockaddr clientaddr; /* client's info */

  /* check command line args (the executable counts as one) */
  if (argc != 5 || !check_flags(argv)) {
    fprintf(stderr, "usage: %s -document_root <filepath> -port <port>\n", argv[0]);
    exit(1);
  }
  doc_root = argv[2];
  portno = atoi(argv[4]);

  /* first, set necessary fields in myaddr struct */
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_port = htons(portno);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* make a socket for listening */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    printf("ERROR opening socket\n");
    exit(1);
  }

  /* setsockopt: lets us rerun the server on the same port right after we kill it.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

  /* bind: associate the listening socket (listenfd) with myaddr */
  if (bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
    printf("ERROR on binding\n");
    exit(1);
  }

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 10) < 0) {
    printf("ERROR on listen\n");
    exit(1);
  }

  /* main loop: wait for a connection request, then hand it to a new thread */
  while (1) {

    /* accept: wait for a connection request */
    clientlen = sizeof(clientaddr);

    /* reserve space for connfd on heap, so each thread gets its own copy */
    connfd = malloc(sizeof(int));
    *connfd = accept(listenfd, &clientaddr, &clientlen);

    /* one bad accept shouldn't take the whole server down */
    if (*connfd < 0) {
      printf("ERROR on accept\n");
      free(connfd);
      continue;
    }

    printf("Client connected!\n");

    /* try to create thread */
    /* if successful, new thread will run function "run_thread" and free connfd */
    if (pthread_create(&tid, NULL, run_thread, connfd) != 0) {
      printf("error creating thread\n");
      close(*connfd);
      free(connfd);
    }
  }
  return 0;
}
