# C++ HTTP Server

A modern HTTP server built from scratch in C++ to deeply understand networking, HTTP, concurrency, and systems programming.

The goal of this project is to progressively implement the core components of a production-grade web server without relying on existing HTTP frameworks.

---

## Features Implemented

- ✅ TCP socket server
- ✅ HTTP request parsing (Request Line)
- ✅ Request buffering with support for partial TCP reads
- ✅ Per-connection leftover buffer for request reassembly
- ✅ HTTP request routing
- ✅ Thread pool for concurrent client handling
- ✅ Configurable logging
- ✅ Basic HTTP response generation

---

## Current Architecture

```
            TCP Socket
                 │
             recv()
                 │
      Request Buffering
       (TCP Reassembly)
                 │
          HTTP Parser
                 │
             Router
                 │
        Request Handler
                 │
        HTTP Response
                 │
              send()
```

---

## Technologies

- C++17
- POSIX Sockets
- CMake
- STL
- Multithreading (`std::thread`)
- Mutexes & Condition Variables

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

The server listens on the configured port (default: **9090**).

Example request:

```bash
curl http://localhost:9090/
```

---

## Roadmap

- [x] TCP socket server
- [x] Thread pool
- [x] Request routing
- [x] HTTP request parser
- [x] TCP request buffering
- [ ] HTTP header parsing
- [ ] Query parameter parsing
- [ ] POST/PUT request body parsing
- [ ] MIME type support
- [ ] Static file serving
- [ ] Persistent (Keep-Alive) connections
- [ ] Chunked Transfer-Encoding
- [ ] HTTP/1.1 compliance improvements
- [ ] Unit tests
- [ ] Benchmarking & profiling

---

## Learning Objectives

This project is focused on understanding:

- TCP/IP networking
- HTTP protocol internals
- Socket programming
- Request parsing
- Concurrent server design
- Memory management
- Modern C++ design
- Systems programming concepts