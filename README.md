# Simple HTTP Web Server

A multithreaded HTTP/1.0 and HTTP/1.1 web server in C. Serves static files with `GET`.

## Usage

```sh
make
./server -document_root <file dir> -port <port>
```

Then open `http://localhost:8080/`.

## Features

- One detached thread per client
- HTTP/1.0: one request per connection
- HTTP/1.1: keep-alive, closed after an idle timeout of `60s / open connections`
- Status codes: `200`, `400`, `403`, `404`, `408`
- Content types: `.html`, `.txt`, `.gif`, `.jpg`

The document root needs `index.html` (served for `/`) and the error pages `400.html`, `403.html` and `404.html`.

## Files

| File           | Purpose                                     |
|----------------|---------------------------------------------|
| `main.c`       | Parses args, accepts connections, spawns threads |
| `connection.c` | Per-client loop and HTTP/1.1 timeout        |
| `request.c`    | Reads and parses requests                   |
| `response.c`   | Picks the status and sends the file         |
| `server.h`     | Shared definitions                          |
| `DOC_ROOT/`    | Sample document root                        |

## Limitations

- HTTP/1.1 requests must send `Host:` as the first header
- Requests must fit in 2048 bytes
