/*
 * response.c - picking the status, finding the file (or error page), and sending it with its header
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>  /* needed for open */
#include <errno.h>  /* needed to get specific errors */
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "server.h"

/* opens <doc_root><filename> for reading */
static int open_in_root(const char *filename) {
  char target[BUFSIZE]; /* full path on disk */

  snprintf(target, BUFSIZE, "%s%s", doc_root, filename);
  printf("File passed through: %s\n", target);
  return open(target, O_RDONLY);
}

/* opens the file to send back and sets status to the matching status line
 * on an error, the error's page (e.g. <doc_root>/404.html) is opened instead and filename is changed to match
 */
static int open_response_file(char filename[BUFSIZE], int malformed, const char **status) {
  int fd;

  if (malformed) {
    *status = "400 Malformed Request\r\n";
  } else if (strstr(filename, "..") != NULL || strstr(filename, "./") != NULL) {
    /* trying to access above root or use relative file paths (which is bad in HTTP) */
    *status = "403 Permission Denied\r\n";
  } else {
    /* if it's / we want index */
    if (!strcmp(filename, "/")) {
      strcpy(filename, "/index.html");
    }
    fd = open_in_root(filename);
    if (fd >= 0) {
      *status = "200 OK \r\n";
      return fd;
    }
    /* 404 covers not found and any other error of unknown origin */
    *status = errno == EACCES ? "403 Permission Denied\r\n" : "404 not found\r\n";
  }

  /* every error status starts with its 3-digit code, which names its page */
  snprintf(filename, BUFSIZE, "/%.3s.html", *status);
  return open_in_root(filename);
}

/* maps a file's extension (whatever follows the last '.') to its content type */
static const char *content_type(const char *filename) {
  const char *ext = strrchr(filename, '.');

  if (ext == NULL || strchr(ext, '/') != NULL) return "unknown"; /* no extension */
  if (!strcmp(ext, ".html")) return "text/html";
  if (!strcmp(ext, ".txt")) return "text/txt";
  if (!strcmp(ext, ".gif")) return "image/gif";
  if (!strcmp(ext, ".jpg")) return "image/jpg";
  return "unknown";
}

/* send: sends the header and then the requested file (or the error page) to the client */
void send_response(request *req, int connfd) {
  char buf[BUFSIZE];   /* message buffer */
  char date[64];       /* the Date header's value */
  const char *status;  /* status line, e.g. "200 OK" */
  int fd;              /* the file we're sending */
  int num_read;        /* num bytes read from the file */
  long length = 0;     /* file size, for Content-Length */
  struct stat st;
  time_t now;
  struct tm tm;

  fd = open_response_file(req->filename, req->malformed, &status);
  if (fd >= 0 && fstat(fd, &st) == 0) {
    length = st.st_size;
  }

  /* gmtime_r writes into our own tm, so the date doesn't leak between threads */
  time(&now);
  gmtime_r(&now, &tm);
  strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &tm);

  snprintf(buf, BUFSIZE, "%s%sDate: %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
           req->keep_alive ? "HTTP/1.1 " : "HTTP/1.0 ", status,
           date, content_type(req->filename), length);

  /* send header before sending information */
  send(connfd, buf, strlen(buf), 0);

  if (fd < 0) {
    return; /* the error page itself is missing: header only */
  }
  while ((num_read = read(fd, buf, sizeof(buf))) > 0) {
    send(connfd, buf, num_read, 0);
  }
  close(fd);
}