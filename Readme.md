# C++ HTTP Server

A modern HTTP server built from scratch in C++ to gain a deep understanding of TCP/IP networking, the HTTP protocol, concurrency, and systems programming.

Rather than relying on existing web frameworks, this project incrementally implements the core components of an HTTP server to explore how HTTP servers work internally.

---

## Features Implemented

### Networking

* ✅ TCP socket server using POSIX sockets
* ✅ Client connection handling using `accept()`
* ✅ Request reception using `recv()`
* ✅ HTTP response transmission using `send()`

### HTTP Processing

* ✅ HTTP Request-Line parsing
* ✅ HTTP header parsing
* ✅ HTTP header value trimming
* ✅ URL decoding
* ✅ Query parameter parsing
* ✅ POST request body extraction using `Content-Length`
* ✅ Parsing of `application/x-www-form-urlencoded` request bodies
* ✅ HTTP request routing
* ✅ Basic HTTP response generation
* ✅ Support for custom response headers
* ✅ Basic Post/Redirect/Get (PRG) flow using **303 See Other**
* ✅ MIME type detection
* ✅ Static file serving
* ✅ Nested static file serving
* ✅ Static file path traversal protection

### TCP Stream Handling

* ✅ Incremental request buffering
* ✅ Support for partial TCP reads
* ✅ Per-connection leftover buffer for request reassembly
* ✅ Proper handling of TCP's byte-stream nature
* ✅ Separation of HTTP headers from request body
* ✅ Body extraction while preserving bytes belonging to subsequent requests

### Static File Serving

The server can serve files directly from the `public/` directory.

Examples:

```text
GET /about.html
        ↓
public/about.html

GET /style.css
        ↓
public/style.css

GET /css/style.css
        ↓
public/css/style.css

GET /script.js
        ↓
public/script.js
```

MIME types are automatically detected based on the file extension.

Examples:

```text
.html  → text/html
.css   → text/css
.js    → application/javascript
.json  → application/json
.png   → image/png
.jpg   → image/jpeg
```

Static file resolution also protects against path traversal attempts such as:

```text
/../../etc/passwd
/%2e%2e/%2e%2e/etc/passwd
```

### Configuration

* ✅ Configurable server port
* ✅ Configurable thread pool size
* ✅ External configuration file (`server.conf`)

### Concurrency

* ✅ Fixed-size thread pool
* ✅ Task queue using mutexes and condition variables
* ✅ Concurrent client request handling

### Utilities

* ✅ Configurable logger
* ✅ URL decoder
* ✅ MIME type detection utility
* ✅ Static file loader
* ✅ Form data parser (`application/x-www-form-urlencoded`)
* ✅ Query parameter parser

---

## Architecture

```text
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
                           ▼
                    HTTP Request Parser
                           │
              ┌────────────┴────────────┐
              │                         │
        Request-Line              Header Parser
      (Method, Target, Version)   (Key-Value Headers)
              │                         │
              │                         ▼
              │                  Content-Length
              │                       Parser
              │                         │
              │                         ▼
              │                  Request Body Reader
              │                         │
              ▼                         ▼
        URL / Query              Form URL-Encoding
         Processing                   Parser
              │                         │
              └────────────┬────────────┘
                           ▼
                      HttpRequest
                           │
                           ▼
                         Router
                           │
              ┌────────────┴────────────┐
              │                         │
        Application Routes         Static Files
              │                         │
       /saveData, etc.          File Resolution
              │                         │
              │                  Path Validation
              │                         │
              │                   MIME Detection
              │                         │
              └────────────┬────────────┘
                           ▼
                    HttpResponse
                           │
                           ▼
                         send()
```

---

## Tech Stack

* C++17
* POSIX Sockets
* STL
* CMake
* `std::thread`
* `std::mutex`
* `std::condition_variable`
* `std::unordered_map`
* `std::filesystem`

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

Static file example:

```bash
curl http://localhost:9090/style.css
```

---

## Current Request Processing Pipeline

The server currently processes an HTTP request through the following stages:

1. Reassembles TCP byte streams into complete HTTP headers.
2. Parses the HTTP Request-Line (`METHOD PATH VERSION`).
3. Separates the request path from the query string.
4. URL-decodes the request path.
5. Parses and URL-decodes query parameters.
6. Parses HTTP headers into a key-value map.
7. Trims whitespace from HTTP header names and values.
8. Detects the end of headers using `\r\n\r\n`.
9. Reads the request body using the `Content-Length` header.
10. Preserves any remaining bytes belonging to subsequent requests.
11. Parses `application/x-www-form-urlencoded` request bodies into key-value pairs.
12. Produces a structured `HttpRequest` object consumed by the router.
13. Routes application-specific requests.
14. Resolves static files safely from the `public/` directory.
15. Determines the appropriate MIME type for static files.
16. Builds and transmits the HTTP response.

---

## What I Learned

This project helped me understand:

* TCP is a byte-stream protocol rather than a message protocol.
* Why a single `recv()` call cannot be assumed to contain one complete HTTP request.
* Why multiple `recv()` calls may be required to receive a complete request body.
* Why one `recv()` may also contain multiple HTTP requests.
* How production servers reconstruct HTTP requests from TCP streams.
* The difference between the Request-Line, Headers, and Body in HTTP.
* How `Content-Length` determines request body boundaries.
* How URL encoding works in request paths and query parameters.
* The difference between `%2B` and `+` when decoding query parameters.
* How query strings are separated from request paths.
* How browsers encode HTML form data using `application/x-www-form-urlencoded`.
* Why protocol parsing and application-level parsing should remain separate.
* How browsers implement the Post/Redirect/Get (PRG) workflow.
* MIME types and their role in HTTP responses.
* How static files can be resolved from a filesystem and served over HTTP.
* Why static file servers must protect against path traversal vulnerabilities.
* Thread pool design using mutexes and condition variables.
* Organizing a growing systems project into modular, single-responsibility components.

---

## Roadmap

### HTTP

* [x] HTTP Request-Line parsing
* [x] HTTP header parsing
* [x] POST request body extraction
* [x] Form URL-encoded body parsing
* [x] Post/Redirect/Get (PRG) flow using 303 redirects
* [x] URL decoding
* [x] Query parameter parsing
* [ ] JSON request body parsing
* [ ] Multipart form-data parsing
* [ ] Cookie parsing
* [x] MIME type detection
* [x] Static file serving
* [ ] Directory listing
* [ ] Persistent (Keep-Alive) connections
* [ ] Chunked Transfer-Encoding
* [ ] Full HTTP/1.1 compliance

### Performance

* [ ] Connection timeout handling
* [ ] Graceful server shutdown
* [ ] Benchmarking
* [ ] Profiling
* [ ] Memory optimization

### Systems

* [ ] Refactor HTTP parsing into a dedicated `HttpParser`
* [ ] Event-driven I/O (`epoll`)
* [ ] Non-blocking sockets
* [ ] Connection manager
* [ ] Thread-safe response cache

### Testing

* [ ] Unit tests
* [ ] Integration tests
* [ ] Stress testing

---

## Future Goals

The long-term objective is to evolve this project into a lightweight production-style HTTP server by implementing more of the HTTP/1.1 specification, improving performance, and exploring lower-level systems concepts such as efficient I/O multiplexing, scalable connection handling, and robust request parsing.

As the project evolves, I also plan to deepen my understanding of systems programming concepts such as event-driven architectures, non-blocking I/O, efficient memory management, and high-performance server design.
