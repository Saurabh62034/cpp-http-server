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
- ✅ Parsing of `application/x-www-form-urlencoded` request bodies
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
- ✅ Body extraction while preserving bytes belonging to subsequent requests

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
- ✅ Form data parser (`application/x-www-form-urlencoded`)

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
      ┌───────────────┴────────────────┐
      │                                │
 Request-Line                   Header Parser
(Method, Path, Version)      (Key-Value Headers)
                                       │
                                       ▼
                            Content-Length Parser
                                       │
                                       ▼
                              Request Body Reader
                                       │
                                       ▼
                        Form URL-Encoded Parser
                     (key=value&key=value...)
                                       │
                                       ▼
                                HttpRequest
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

## Current Request Processing Pipeline

The server currently processes an HTTP request in the following stages:

1. Reassembles TCP byte streams into complete HTTP headers.
2. Parses the HTTP Request-Line (`METHOD PATH VERSION`).
3. Parses HTTP headers into a key-value map.
4. Detects the end of headers using `\r\n\r\n`.
5. Reads the request body using the `Content-Length` header.
6. Preserves any remaining bytes belonging to subsequent requests.
7. Parses `application/x-www-form-urlencoded` request bodies into key-value pairs.
8. Produces a structured `HttpRequest` object that is consumed by the router.

---

## What I Learned

This project helped me understand:

- TCP is a byte-stream protocol rather than a message protocol.
- Why a single `recv()` call cannot be assumed to contain one complete HTTP request.
- Why multiple `recv()` calls may be required to receive a complete request body.
- Why one `recv()` may also contain multiple HTTP requests.
- How production servers reconstruct HTTP requests from TCP streams.
- The difference between the Request-Line, Headers, and Body in HTTP.
- How `Content-Length` determines request body boundaries.
- How browsers encode HTML form data using `application/x-www-form-urlencoded`.
- Why protocol parsing and application-level parsing should remain separate.
- How browsers implement the Post/Redirect/Get (PRG) workflow.
- Thread pool design using mutexes and condition variables.
- Organizing a growing systems project into modular, single-responsibility components.

---

## Roadmap

### HTTP

- [x] HTTP Request-Line parsing
- [x] HTTP header parsing
- [x] POST request body extraction
- [x] Form URL-encoded body parsing
- [x] Post/Redirect/Get (PRG) flow using 303 redirects
- [ ] URL decoding (`%20`, `%40`, `+`, etc.)
- [ ] Query parameter parsing
- [ ] JSON request body parsing
- [ ] Multipart form-data parsing
- [ ] Cookie parsing
- [ ] MIME type detection
- [ ] Static file serving
- [ ] Directory listing
- [ ] Persistent (Keep-Alive) connections
- [ ] Chunked Transfer-Encoding
- [ ] Full HTTP/1.1 compliance

### Performance

- [ ] Connection timeout handling
- [ ] Graceful server shutdown
- [ ] Benchmarking
- [ ] Profiling
- [ ] Memory optimization

### Systems

- [ ] Refactor HTTP parsing into a dedicated `HttpParser`
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

As the project evolves, I also plan to deepen my understanding of systems programming concepts such as event-driven architectures, non-blocking I/O, efficient memory management, and high-performance server design.