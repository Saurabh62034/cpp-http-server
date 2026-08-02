# C++ HTTP Server

A modern HTTP server built from scratch in C++ to gain a deep understanding of TCP/IP networking, the HTTP protocol, concurrency, and systems programming.

Rather than relying on existing web frameworks, this project incrementally implements the core components of a production-grade web server to explore how HTTP servers work internally.

---

## Features Implemented

### Networking
- ✅ TCP socket server using POSIX sockets
- ✅ Client connection handling using `accept()`
- ✅ Request reception using `recv()`
- ✅ HTTP response transmission using `send()`

### HTTP Processing
- ✅ HTTP Request-Line parsing
- ✅ HTTP header parsing
- ✅ POST request body extraction using `Content-Length`
- ✅ HTTP request routing
- ✅ Basic HTTP response generation
- ✅ Support for custom response headers
- ✅ Basic Post/Redirect/Get (PRG) flow using **303 See Other**

### TCP Stream Handling
- ✅ Incremental request buffering
- ✅ Support for partial TCP reads
- ✅ Per-connection leftover buffer for request reassembly
- ✅ Proper handling of TCP's byte-stream nature
- ✅ Separation of HTTP headers from request body
- ✅ Body extraction while preserving bytes belonging to the next request

### Configuration
- ✅ Configurable server port
- ✅ Configurable thread pool size
- ✅ External configuration file (`server.conf`)

### Concurrency
- ✅ Fixed-size thread pool
- ✅ Task queue using mutexes and condition variables
- ✅ Concurrent client request handling

### Utilities
- ✅ Configurable logger
- ✅ Static HTML file loading

---

## Architecture

```
                   Client
                      │
                  TCP Socket
                      │
                   accept()
                      │
                    recv()
                      │
        TCP Stream Reassembly
      (Buffer + Leftover Bytes)
                      │
          HTTP Request Parser
      ┌───────────────┴───────────────┐
      │                               │
 Request-Line                  Header Parser
(Method, Path, Version)      (Key-Value Map)
                                      │
                                      ▼
                           Content-Length Parser
                                      │
                                      ▼
                             Body Extraction
                                      │
                                      ▼
                                  Router
                                      │
                                      ▼
                              Request Handler
                                      │
                                      ▼
                           HTTP Response Builder
                                      │
                                      ▼
                                   send()
```

---

## Tech Stack

- C++17
- POSIX Sockets
- STL
- CMake
- `std::thread`
- `std::mutex`
- `std::condition_variable`
- `std::unordered_map`

---

## Build

```bash
mkdir build
cd build
cmake ..
make
```

---

## Run

```bash
./http_server
```

The server listens on the configured port (**9090** by default).

Example:

```bash
curl http://localhost:9090/
```

---

## Current HTTP Parser

The server currently performs the following parsing pipeline:

1. Reassembles TCP byte streams into complete HTTP requests.
2. Parses the HTTP Request-Line (`METHOD PATH VERSION`).
3. Parses HTTP headers into a key-value map.
4. Detects the end of headers (`\r\n\r\n`).
5. Uses the `Content-Length` header to determine body size.
6. Extracts the request body while preserving any bytes that belong to the next HTTP request.
7. Produces a structured `HttpRequest` object consumed by the router.

---

## What I Learned

This project helped me understand:

- TCP is a byte-stream protocol rather than a message protocol.
- Why multiple `recv()` calls may be required to receive one HTTP request.
- Why one `recv()` may also contain multiple HTTP requests.
- How production servers reconstruct HTTP requests from TCP streams.
- The difference between the Request-Line, Headers, and Body in HTTP.
- How `Content-Length` determines request body boundaries.
- Why HTTP parsing must distinguish protocol framing from application data.
- How browsers implement the Post/Redirect/Get (PRG) workflow.
- Thread pool design using mutexes and condition variables.
- Request routing without using existing web frameworks.
- Organizing a growing systems project into modular components.

---

## Roadmap

### HTTP

- [x] HTTP Request-Line parsing
- [x] HTTP header parsing
- [x] POST request body extraction
- [ ] URL decoding (`application/x-www-form-urlencoded`)
- [ ] Query parameter parsing
- [ ] JSON request body parsing
- [ ] Multipart form-data parsing
- [ ] Cookie parsing
- [ ] HTTP status code improvements
- [ ] MIME type detection
- [ ] Static file serving
- [ ] Directory listing
- [ ] Persistent (Keep-Alive) connections
- [ ] Chunked Transfer-Encoding
- [ ] HTTP/1.1 compliance improvements

### Performance

- [ ] Connection timeout handling
- [ ] Graceful server shutdown
- [ ] Benchmarking
- [ ] Profiling
- [ ] Memory optimization

### Systems

- [ ] HTTP parser refactoring into a dedicated `HttpParser`
- [ ] Event-driven I/O (`epoll`)
- [ ] Non-blocking sockets
- [ ] Connection manager
- [ ] Thread-safe response cache

### Testing

- [ ] Unit tests
- [ ] Integration tests
- [ ] Stress testing

---

## Future Goals

The long-term objective is to evolve this project into a lightweight production-style HTTP server by implementing more of the HTTP/1.1 specification, improving performance, and exploring lower-level systems concepts such as efficient I/O multiplexing, scalable connection handling, and robust request parsing.