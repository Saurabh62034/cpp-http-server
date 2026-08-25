# C++ HTTP Server

A modern HTTP server built from scratch in C++ to gain a deep understanding of TCP/IP networking, the HTTP protocol, concurrency, and systems programming.

Rather than relying on existing web frameworks, this project incrementally implements the core components of an HTTP server to explore how HTTP servers work internally.

---

## Features Implemented

### Networking

- ✅ TCP socket server using POSIX sockets
- ✅ Client connection handling using `accept()`
- ✅ Request reception using `recv()`
- ✅ Reliable HTTP response transmission using `sendAll()`
- ✅ Configurable listening port

### HTTP Processing

- ✅ HTTP Request-Line parsing
- ✅ HTTP header parsing
- ✅ HTTP header value trimming
- ✅ URL decoding
- ✅ Query parameter parsing
- ✅ POST request body extraction using `Content-Length`
- ✅ Parsing of `application/x-www-form-urlencoded` request bodies
- ✅ HTTP request routing
- ✅ Basic HTTP response generation
- ✅ Support for custom response headers
- ✅ Basic Post/Redirect/Get (PRG) flow using **303 See Other**
- ✅ MIME type detection
- ✅ Static file serving
- ✅ Nested static file serving
- ✅ Static file path traversal protection
- ✅ HTTP/1.1 persistent connections
- ✅ HTTP/1.0 Keep-Alive support
- ✅ `Connection: close` handling
- ✅ `Connection: keep-alive` response handling

### HTTP Parser

HTTP request parsing has been separated from the server's connection-handling logic using a dedicated `HttpParser`.

The parser currently handles:

- ✅ Request-Line parsing
- ✅ HTTP method extraction
- ✅ Request target extraction
- ✅ HTTP version extraction
- ✅ Query string extraction
- ✅ URL path decoding
- ✅ Query parameter parsing
- ✅ Query parameter URL decoding
- ✅ HTTP header parsing
- ✅ Header whitespace trimming
- ✅ `Content-Length` parsing
- ✅ Request body extraction
- ✅ TCP-buffer-aware body reading

The parser produces a structured `HttpRequest` object that is consumed by the router and application layer.

### TCP Stream Handling

- ✅ Incremental request buffering
- ✅ Support for partial TCP reads
- ✅ Per-connection leftover buffer for request reassembly
- ✅ Proper handling of TCP's byte-stream nature
- ✅ Separation of HTTP headers from request body
- ✅ Body extraction while preserving bytes belonging to subsequent requests
- ✅ Multiple HTTP requests over a single TCP connection
- ✅ Persistent connections using HTTP/1.1 Keep-Alive
- ✅ Correct handling of `Connection: close`

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

MIME types are automatically detected based on the file extension.

Examples:

.html  → text/html
.css   → text/css
.js    → application/javascript
.json  → application/json
.png   → image/png
.jpg   → image/jpeg

Static file resolution also protects against path traversal attempts such as:

/../../etc/passwd
/%2e%2e/%2e%2e/etc/passwd

The requested URL is decoded and validated before resolving the corresponding filesystem path.

Configuration
✅ Configurable server port
✅ Configurable thread pool size
✅ External configuration file (server.conf)
Concurrency
✅ Fixed-size thread pool
✅ Task queue using mutexes and condition variables
✅ Concurrent client connection handling
✅ Multiple requests handled sequentially on a persistent connection
Utilities
✅ Configurable logger
✅ URL decoder
✅ MIME type detection utility
✅ Static file loader
✅ Form data parser (application/x-www-form-urlencoded)
✅ Query parameter parser
Architecture
                              Client
                                │
                            TCP Socket
                                │
                             accept()
                                │
                                ▼
                       Thread Pool / Worker
                                │
                                ▼
                              recv()
                                │
                                ▼
                    TCP Stream Reassembly
                  (Buffer + Leftover Bytes)
                                │
                                ▼
                           HttpParser
                                │
             ┌──────────────────┼──────────────────┐
             │                  │                  │
             ▼                  ▼                  ▼
        Request-Line       HTTP Headers       Query String
        Method/Target/     Key-Value Map       Extraction
        Version                 │                  │
             │                  │                  ▼
             │                  ▼             URL Decoding
             │            Content-Length      Query Params
             │                  │
             │                  ▼
             │            Request Body
             │                  │
             ▼                  ▼
                       HttpRequest
                             │
                             ▼
                           Router
                             │
                ┌────────────┴────────────┐
                │                         │
                ▼                         ▼
        Application Routes          Static Files
                │                         │
         /saveData, etc.          File Resolution
                                          │
                                   Path Validation
                                          │
                                    MIME Detection
                │                         │
                └────────────┬────────────┘
                             ▼
                       HttpResponse
                             │
                             ▼
                          sendAll()
                             │
                             ▼
                    Keep-Alive Decision
                             │
                  ┌──────────┴──────────┐
                  │                     │
               Keep Alive            Close
                  │                     │
                  ▼                     ▼
             Next Request          close()
Tech Stack
C++17
POSIX Sockets
STL
CMake
std::thread
std::mutex
std::condition_variable
std::unordered_map
std::filesystem
Build
mkdir build
cd build
cmake ..
make
Run
./http_server

The server listens on the configured port (9090 by default).

Example:

curl http://localhost:9090/

Static file example:

curl http://localhost:9090/style.css

Query parameter example:

curl "http://localhost:9090/about?name=Saurabh"

POST request example:

curl -X POST \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "name=Saurabh&email=test@example.com" \
     http://localhost:9090/saveData
Persistent Connection Example

The server supports multiple HTTP requests over the same TCP connection.

For example:

GET /about.html
GET /css/style.css
Connection: close

can be sent over the same connection.

Example using netcat:

printf 'GET /about.html HTTP/1.1\r\nHost: localhost:9090\r\n\r\nGET /css/style.css HTTP/1.1\r\nHost: localhost:9090\r\nConnection: close\r\n\r\n' | nc 127.0.0.1 9090

The server processes both requests from the same TCP connection and closes the connection after the second response.

Current Request Processing Pipeline

The server currently processes an HTTP request through the following stages:

Accepts a TCP connection using accept().
Assigns the connection to a thread-pool worker.
Receives bytes from the TCP socket using recv().
Reassembles the TCP byte stream using a per-connection leftover buffer.
Detects the HTTP header boundary using \r\n\r\n.
Passes the HTTP message to HttpParser.
Parses the HTTP Request-Line (METHOD PATH VERSION).
Separates the request path from the query string.
URL-decodes the request path.
Parses and URL-decodes query parameters.
Parses HTTP headers into a key-value map.
Trims whitespace from HTTP header names and values.
Detects Content-Length.
Reads additional TCP data when the request body has not been completely received.
Extracts exactly Content-Length bytes as the request body.
Preserves any remaining bytes belonging to subsequent requests.
Parses application/x-www-form-urlencoded request bodies using FormParser.
Determines whether the connection should remain persistent.
Routes the request through Router.
Resolves and validates static file paths when required.
Determines the appropriate MIME type for static files.
Builds an HttpResponse.
Sends the complete response using sendAll().
Continues processing additional requests when Keep-Alive is enabled.
Closes the client socket when the connection is no longer persistent.
What I Learned

This project helped me understand:

TCP and Networking
TCP is a byte-stream protocol rather than a message protocol.
Why a single recv() call cannot be assumed to contain one complete HTTP request.
Why multiple recv() calls may be required to receive a complete request body.
Why a single recv() may contain multiple HTTP requests.
How application-level TCP stream reassembly works.
How leftover bytes from one request can belong to the next request.
Why response transmission may also require multiple send() calls.
How sendAll() ensures the complete response is transmitted.
HTTP
The difference between the Request-Line, Headers, and Body.
How Content-Length determines request body boundaries.
How HTTP/1.1 persistent connections work.
How Connection: close changes connection behavior.
How HTTP/1.0 Keep-Alive differs from HTTP/1.1.
How response headers communicate connection state.
How HTTP status codes are represented in responses.
How browsers implement the Post/Redirect/Get (PRG) workflow.
How MIME types describe HTTP response content.
URL and Form Encoding
How URL encoding works in request paths and query parameters.
The difference between %2B and + when decoding query parameters.
How query strings are separated from request paths.
How browsers encode HTML form data using application/x-www-form-urlencoded.
Why query parameter decoding and form decoding have different semantics.
Static File Serving
How HTTP servers map URLs to filesystem paths.
How std::filesystem can be used for filesystem operations.
How MIME types are determined from file extensions.
Why static file servers must protect against path traversal vulnerabilities.
Why URL decoding must be considered before validating filesystem paths.
Concurrency
Thread pool design using mutexes and condition variables.
How worker threads consume tasks from a shared queue.
How multiple client connections can be processed concurrently.
The difference between connection-level concurrency and request-level processing.
Software Architecture
Separating TCP connection handling from HTTP parsing.
Separating protocol parsing from application-level form parsing.
Designing a dedicated HttpParser.
Keeping URL decoding and form parsing as independent utilities.
Breaking a growing systems project into single-responsibility components.
Maintaining TCP buffering state independently for each client connection.
Keeping routing and HTTP parsing separate.
Roadmap
HTTP
- [x] HTTP Request-Line parsing
- [x] HTTP header parsing
- [x] HTTP header value trimming
- [x] POST request body extraction
- [x] Form URL-encoded body parsing
- [x] Post/Redirect/Get (PRG) flow using 303 redirects
- [x] URL decoding
- [x] Query parameter parsing
- [x] MIME type detection
- [x] Static file serving
- [x] Persistent (Keep-Alive) connections
- [ ] JSON request body parsing
- [ ] Multipart form-data parsing
- [ ] Cookie parsing
- [ ] Directory listing
- [ ] Chunked Transfer-Encoding
- [ ] Range requests
- [ ] Conditional requests (ETag, If-Modified-Since)
- [ ] Proper handling of malformed HTTP requests
- [ ] Improved HTTP status code handling
- [ ] Full HTTP/1.1 compliance

Performance
- [ ] Connection timeout handling
- [ ] Graceful server shutdown
- [x] Benchmarking with ApacheBench
- [x] Concurrent load testing
- [ ] Profiling
- [ ] Memory optimization
- [ ] Static file caching
- [ ] Response buffering optimization
Systems
- [x] Refactor HTTP parsing into a dedicated `HttpParser`
- [ ] Separate TCP connection management from `HttpServer`
- [ ] Connection abstraction
- [ ] Non-blocking sockets
- [ ] Event-driven I/O (`epoll`)
- [ ] Connection manager
- [ ] Thread-safe response cache
- [ ] Event-driven architecture
- [ ] Zero-copy / efficient file transmission
- [ ] Graceful worker-thread shutdown
Testing
- [ ] Unit tests
- [ ] HTTP parser unit tests
- [ ] Query parser tests
- [ ] URL decoder tests
- [ ] Integration tests
- [x] Static file security tests
- [x] Keep-Alive tests
- [x] Multiple-request connection tests
- [ ] Malformed request tests
- [ ] Stress testing
- [ ] Automated regression tests

Testing Performed

The server has been manually tested with:

Static Files
/about.html
/style.css
/css/style.css
/script.js
Missing Files
/does-not-exist.css
/style.html

Correctly returns:

404 Not Found
Path Traversal

Tested against:

/../../etc/passwd
/%2e%2e/%2e%2e/etc/passwd

Requests are prevented from escaping the configured public/ directory.

URL Decoding

Tested with encoded paths such as:

/hello%20world

which is decoded to:

/hello world
Query Parameters

Tested with:

/about?name=Saurabh

including multiple parameters and URL-encoded values.

POST Requests

Tested with:

Content-Type: application/x-www-form-urlencoded

and verified:

Content-Length based body extraction
Form parsing
Query-independent request body handling
PRG redirect flow
Persistent Connections

Tested multiple HTTP requests over the same TCP connection:

GET /about.html
GET /css/style.css
Connection: close

The server correctly:

Processes the first request.
Keeps the TCP connection alive.
Reads the second request from the same connection.
Sends the second response.
Closes the connection because of Connection: close.
Project Structure

The project is organized into focused components:

http-server/
│
├── src/
│   ├── HttpServer.cpp
│   ├── HttpParser.cpp
│   ├── HttpResponse.cpp
│   ├── Router.cpp
│   ├── FormParser.cpp
│   ├── UrlDecoder.cpp
│   ├── MimeTypes.cpp
│   └── ThreadPool.cpp
│
├── include/
│   ├── HttpServer.h
│   ├── HttpParser.h
│   ├── HttpRequest.h
│   ├── HttpResponse.h
│   ├── Router.h
│   ├── FormParser.h
│   ├── UrlDecoder.h
│   ├── MimeTypes.h
│   └── ThreadPool.h
│
├── public/
│   ├── index.html
│   ├── about.html
│   ├── style.css
│   ├── script.js
│   └── css/
│
├── tests/
├── build/
├── CMakeLists.txt
├── server.conf
└── README.md
Future Goals

The long-term objective is to evolve this project into a lightweight production-style HTTP server by implementing more of the HTTP/1.1 specification, improving performance, and exploring lower-level systems concepts such as efficient I/O multiplexing, scalable connection handling, and robust request parsing.

The next major goals are:

Improve HTTP parser robustness and validation.
Add automated unit and integration tests.
Separate TCP connection management from HttpServer.
Introduce connection timeouts.
Implement graceful server shutdown.
Add non-blocking sockets.
Introduce Linux epoll.
Move toward an event-driven architecture.
Benchmark the server under concurrent load.
Profile CPU and memory usage.
Optimize static file serving.
Explore efficient file transmission and zero-copy techniques.
Implement more of the HTTP/1.1 specification.

The project is ultimately intended to demonstrate practical understanding of:

TCP/IP networking
Socket programming
HTTP/1.1
TCP stream reassembly
HTTP request parsing
HTTP response generation
Persistent connections
Concurrency
Thread pools
Filesystem operations
Secure static file serving
Non-blocking I/O
Event-driven server architecture
High-performance server design