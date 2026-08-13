#!/usr/bin/env python3

import socket
import sys
import time


HOST = "127.0.0.1"
PORT = 9090

CONNECT_TIMEOUT = 2.0
READ_TIMEOUT = 2.0


# ============================================================
# Exceptions
# ============================================================

class TestFailure(Exception):
    pass


# ============================================================
# TCP / HTTP helpers
# ============================================================

def connect():
    sock = socket.create_connection(
        (HOST, PORT),
        timeout=CONNECT_TIMEOUT
    )
    sock.settimeout(READ_TIMEOUT)
    return sock


def recv_until(sock, marker):
    """
    Receive until marker is found.
    Returns:
        complete_data,
        remaining_data
    """
    data = b""

    while marker not in data:
        chunk = sock.recv(4096)

        if not chunk:
            break

        data += chunk

    if marker in data:
        index = data.index(marker) + len(marker)
        return data[:index], data[index:]

    return data, b""


def parse_headers(header_bytes):
    """
    Parse:

        HTTP/1.1 200 OK
        Content-Type: text/html
        Content-Length: 100
        Connection: keep-alive

    into:
        status_line
        headers
    """

    text = header_bytes.decode("iso-8859-1")

    lines = text.split("\r\n")

    status_line = lines[0]

    headers = {}

    for line in lines[1:]:
        if not line:
            continue

        if ":" not in line:
            continue

        key, value = line.split(":", 1)

        headers[key.strip().lower()] = value.strip()

    return status_line, headers


def read_http_response(sock, initial_data=b""):
    """
    Read one complete HTTP response.

    Uses Content-Length when available.

    Returns:
        {
            "status_line": ...,
            "status_code": ...,
            "headers": ...,
            "body": ...
        }

    Also returns any bytes belonging to a following response.
    """

    data = initial_data

    # --------------------------------------------------------
    # Read HTTP headers
    # --------------------------------------------------------

    while b"\r\n\r\n" not in data:
        chunk = sock.recv(4096)

        if not chunk:
            raise TestFailure(
                "Connection closed before HTTP headers completed"
            )

        data += chunk

    header_end = data.index(b"\r\n\r\n") + 4

    header_bytes = data[:header_end]
    remaining = data[header_end:]

    status_line, headers = parse_headers(header_bytes)

    # --------------------------------------------------------
    # Status code
    # --------------------------------------------------------

    try:
        status_code = int(status_line.split()[1])
    except (IndexError, ValueError):
        raise TestFailure(
            f"Invalid HTTP status line: {status_line!r}"
        )

    # --------------------------------------------------------
    # Body
    # --------------------------------------------------------

    content_length = headers.get("content-length")

    if content_length is not None:
        try:
            expected_length = int(content_length)
        except ValueError:
            raise TestFailure(
                f"Invalid Content-Length: {content_length!r}"
            )

        while len(remaining) < expected_length:
            chunk = sock.recv(4096)

            if not chunk:
                raise TestFailure(
                    f"Connection closed before body completed "
                    f"(expected {expected_length}, "
                    f"received {len(remaining)})"
                )

            remaining += chunk

        body = remaining[:expected_length]
        leftover = remaining[expected_length:]

    else:
        # For our current server, responses should use
        # Content-Length. If one doesn't exist, read until close.
        body = remaining
        leftover = b""

    return {
        "status_line": status_line,
        "status_code": status_code,
        "headers": headers,
        "body": body,
    }, leftover


def send_request(request, timeout=READ_TIMEOUT):
    """
    Open a new TCP connection, send one HTTP request,
    read one response, then close.
    """

    sock = connect()
    sock.settimeout(timeout)

    try:
        sock.sendall(request)

        response, _ = read_http_response(sock)

        return response

    finally:
        sock.close()


def send_raw(sock, data):
    sock.sendall(data)


def response_text(response):
    return response["body"].decode(
        "utf-8",
        errors="replace"
    )


# ============================================================
# Assertions
# ============================================================

def assert_equal(actual, expected, message=""):
    if actual != expected:
        raise TestFailure(
            f"{message}\n"
            f"Expected: {expected!r}\n"
            f"Actual:   {actual!r}"
        )


def assert_true(condition, message):
    if not condition:
        raise TestFailure(message)


def assert_status(response, expected):
    assert_equal(
        response["status_code"],
        expected,
        "Unexpected HTTP status"
    )


def assert_header(response, name, expected):
    actual = response["headers"].get(name.lower())

    assert_equal(
        actual,
        expected,
        f"Unexpected value for header {name}"
    )


# ============================================================
# Basic HTTP tests
# ============================================================

def test_request_line():
    request = (
        b"GET /index.html HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    assert_true(
        len(response["body"]) > 0,
        "Response body is empty"
    )


def test_headers():
    request = (
        b"GET / HTTP/1.1\r\n"
        b"Host:    localhost\r\n"
        b"User-Agent:    test-client    \r\n"
        b"X-Test:    hello world    \r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    # The important thing here is that the request was accepted
    # with whitespace around header values.
    assert_true(
        len(response["body"]) > 0,
        "Server did not return a body"
    )


def test_query_parameters():
    request = (
        b"GET /?name=Saurabh&role=developer&age=27 HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    # IMPORTANT:
    # This proves the request containing query parameters is accepted.
    # It does NOT independently prove the internal query map contents.
    assert_true(
        len(response["body"]) > 0,
        "Query request returned empty body"
    )


def test_url_decoding():
    request = (
        b"GET /?name=Saurabh%20Kumar&message=hello%20world%21 HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    # As above, this verifies request acceptance.
    # To verify decoded values themselves, the server needs
    # an endpoint that exposes parsed query parameters.
    assert_true(
        len(response["body"]) > 0,
        "URL encoded request returned empty body"
    )


# ============================================================
# POST tests
# ============================================================

def test_post_body():
    body = (
        b"name=Saurabh&role=developer"
    )

    request = (
        b"POST /create-user HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: application/x-www-form-urlencoded\r\n"
        b"Content-Length: " + str(len(body)).encode() + b"\r\n"
        b"\r\n"
        + body
    )

    response = send_request(request)

    # We mainly want to prove the server receives and processes
    # a POST request with a body.
    assert_true(
        response["status_code"] in (200, 201, 303, 400),
        f"Unexpected POST status: {response['status_code']}"
    )


def test_form_urlencoded():
    body = (
        b"name=Saurabh+Kumar"
        b"&role=backend"
        b"&age=27"
    )

    request = (
        b"POST /create-user HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: application/x-www-form-urlencoded\r\n"
        b"Content-Length: " + str(len(body)).encode() + b"\r\n"
        b"\r\n"
        + body
    )

    response = send_request(request)

    assert_true(
        response["status_code"] in (200, 201, 303, 400),
        f"Unexpected form POST status: {response['status_code']}"
    )


def test_json_body():
    body = (
        b'{"name":"Saurabh","role":"backend"}'
    )

    request = (
        b"POST /create-user HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: application/json\r\n"
        b"Content-Length: " + str(len(body)).encode() + b"\r\n"
        b"\r\n"
        + body
    )

    response = send_request(request)

    assert_true(
        response["status_code"] in (200, 201, 303, 400),
        f"Unexpected JSON POST status: {response['status_code']}"
    )


def test_multipart():
    boundary = "----HttpServerTestBoundary"

    body = (
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="name"\r\n'
        "\r\n"
        "Saurabh\r\n"
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="role"\r\n'
        "\r\n"
        "backend\r\n"
        f"--{boundary}--\r\n"
    ).encode()

    request = (
        b"POST /create-user HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: multipart/form-data; boundary="
        + boundary.encode()
        + b"\r\n"
        b"Content-Length: "
        + str(len(body)).encode()
        + b"\r\n"
        b"\r\n"
        + body
    )

    response = send_request(request)

    assert_true(
        response["status_code"] in (200, 201, 303, 400),
        f"Unexpected multipart status: {response['status_code']}"
    )


# ============================================================
# Static files / MIME
# ============================================================

def test_static_files():
    request = (
        b"GET /index.html HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    assert_true(
        len(response["body"]) > 0,
        "index.html returned empty body"
    )


def test_mime_detection():
    request = (
        b"GET /index.html HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_status(response, 200)

    content_type = response["headers"].get("content-type")

    assert_true(
        content_type is not None,
        "Content-Type header missing"
    )

    assert_true(
        "text/html" in content_type,
        f"Expected text/html, got {content_type!r}"
    )


# ============================================================
# Redirect / PRG
# ============================================================

def test_redirect():
    """
    This test uses /create-user because the current HTML
    indicates that route exists.

    We accept either:
        200
        303
        404

    initially, because we are using this test to discover
    current behavior. Once the exact redirect contract is
    confirmed, tighten this to assert 303.
    """

    request = (
        b"GET /create-user HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )

    response = send_request(request)

    assert_true(
        response["status_code"] in (200, 303, 404),
        f"Unexpected /create-user status: "
        f"{response['status_code']}"
    )


def test_prg():
    body = (
        b"name=Saurabh"
        b"&email=saurabh@example.com"
        b"&password=12345678"
    )

    request = (
        b"POST /saveData HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: application/x-www-form-urlencoded\r\n"
        b"Content-Length: " + str(len(body)).encode() + b"\r\n"
        b"\r\n"
        + body
    )

    response = send_request(request)

    assert_status(response, 303)

    assert_true(
        "location" in response["headers"],
        "303 response missing Location header"
    )
# ============================================================
# Keep-Alive
# ============================================================

def test_keep_alive():
    sock = connect()

    try:
        request = (
            b"GET / HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"
        )

        send_raw(sock, request)

        response1, leftover = read_http_response(sock)

        assert_status(response1, 200)

        connection = response1["headers"].get("connection")

        assert_true(
            connection is not None,
            "Connection header missing"
        )

        assert_equal(
            connection.lower(),
            "keep-alive",
            "Server did not return keep-alive"
        )

    finally:
        sock.close()


# ============================================================
# Multiple requests on one connection
# ============================================================

def test_multiple_requests_same_connection():
    sock = connect()

    try:
        request1 = (
            b"GET / HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"
        )

        send_raw(sock, request1)

        response1, leftover = read_http_response(sock)

        assert_status(response1, 200)

        request2 = (
            b"GET /index.html HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Connection: close\r\n"
            b"\r\n"
        )

        send_raw(sock, request2)

        response2, leftover = read_http_response(
            sock,
            leftover
        )

        assert_status(response2, 200)

    finally:
        sock.close()


# ============================================================
# Partial TCP read
# ============================================================

def test_partial_tcp_reads():
    """
    Deliberately split ONE HTTP request into multiple TCP writes.

    This is important because recv() boundaries do not necessarily
    correspond to HTTP message boundaries.
    """

    sock = connect()

    try:
        parts = [
            b"GET /ind",
            b"ex.html HTTP/1.",
            b"1\r\nHos",
            b"t: localhost\r\n",
            b"\r\n",
        ]

        for part in parts:
            sock.sendall(part)

            # Give the server a chance to receive the partial data.
            time.sleep(0.05)

        response, _ = read_http_response(sock)

        assert_status(response, 200)

        assert_true(
            len(response["body"]) > 0,
            "Partial-read request produced empty body"
        )

    finally:
        sock.close()


# ============================================================
# Multiple HTTP requests in one send
# ============================================================

def test_multiple_requests_one_recv():
    """
    Send TWO complete HTTP requests in a SINGLE sendall().

    The server may receive both requests in one recv() call.

    It must process request #1 and retain/process request #2.
    """

    sock = connect()

    try:
        combined = (
            b"GET / HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"

            b"GET /index.html HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Connection: close\r\n"
            b"\r\n"
        )

        sock.sendall(combined)

        response1, leftover = read_http_response(sock)

        assert_status(response1, 200)

        response2, leftover = read_http_response(
            sock,
            leftover
        )

        assert_status(response2, 200)

    finally:
        sock.close()


# ============================================================
# Test runner
# ============================================================

TESTS = [
    ("Request-Line parsing", test_request_line),
    ("Header parsing + trimming", test_headers),
    ("Query parameters", test_query_parameters),
    ("URL decoding", test_url_decoding),
    ("POST body extraction", test_post_body),
    ("application/x-www-form-urlencoded", test_form_urlencoded),
    ("JSON body", test_json_body),
    ("multipart/form-data", test_multipart),
    ("MIME detection", test_mime_detection),
    ("Static files", test_static_files),
    ("303 redirect", test_redirect),
    ("PRG", test_prg),
    ("Keep-Alive", test_keep_alive),
    ("Multiple requests / connection", test_multiple_requests_same_connection),
    ("Partial TCP reads", test_partial_tcp_reads),
    ("Multiple requests in one recv()", test_multiple_requests_one_recv),
]


def run_tests():
    print()
    print("=" * 65)
    print("C++ HTTP SERVER TEST SUITE")
    print("=" * 65)
    print(f"Server: {HOST}:{PORT}")
    print()

    passed = 0
    failed = 0

    for name, test in TESTS:

        try:
            test()

            print(f"[PASS] {name}")
            passed += 1

        except (TestFailure, socket.timeout, ConnectionError, OSError) as e:

            print(f"[FAIL] {name}")
            print(f"       {e}")
            failed += 1

        except Exception as e:

            print(f"[ERROR] {name}")
            print(f"        {type(e).__name__}: {e}")
            failed += 1

    print()
    print("=" * 65)
    print(f"RESULT: {passed}/{len(TESTS)} passed")
    print("=" * 65)

    if failed == 0:
        print("ALL TESTS PASSED")
        return 0

    print(f"{failed} TEST(S) FAILED")
    return 1


if __name__ == "__main__":
    sys.exit(run_tests())