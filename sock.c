#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

double _start_microtime = 0.0;
int _server_pid = 0;
int _client_pid = 0;

static char test_buf[64*1024];
#define SEND_RECV_COUNT 163840
#define TEST_PORT 55555
#define QUEUE_SIZE 100


double microtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

void LOG(char* format, ...)
{
    va_list arglist;
    static char log_buf[260];
    double ts = microtime() - _start_microtime;
    va_start(arglist, format);
    vsprintf(log_buf, format, arglist);
    va_end(arglist);
    fprintf(stderr, "[%12.6f] %s\n", ts, log_buf);
}

void test_send(const char* name, int sock)
{
    int res;
    long size, total;
    double ts, mbps;

    memset(test_buf, 'Z', sizeof(test_buf));

    //LOG("%s: start send", name);

    ts = microtime();
    size = 0;
    total = sizeof(test_buf) * SEND_RECV_COUNT;

    for (int n=0; size<total; n++) {
        res = send(sock, test_buf, sizeof(test_buf), 0);
        if (res < sizeof(test_buf)) {
            LOG("%s: send iter(%d) res(%d)", name, n, res);
            perror("send");
            break;
        }
        size += res;
    }

    ts = microtime() - ts;
    mbps = 8.0 * size / 1024 / 1024 / ts;

    LOG("%s: sent %ld bytes in %1.3f sec %1.3f mbps",
        name, size, ts, mbps);
}

void test_recv(const char* name, int sock)
{
    int res;
    long size, total;
    double ts, mbps;

    //LOG("%s: start recv", name);

    ts = microtime();
    size = 0;
    total = sizeof(test_buf) * SEND_RECV_COUNT;

    for (int n=0; size < total; n++) {
        memset(test_buf, 'A', sizeof(test_buf));
        res = recv(sock, test_buf, sizeof(test_buf), 0);
        if (res <= 0) {
            LOG("%s: error recv iter(%d) res(%d)", name, n, res);
            perror("recv");
            break;
        }
        if (test_buf[0] != 'Z' || test_buf[res-1] != 'Z') {
            LOG("%s: error recv iter(%d) wrong content", name, n);
            break;
        }
        size += res;
    }

    ts = microtime() - ts;
    mbps = 8.0 * size / 1024 / 1024 / ts;

    LOG("%s: recv %ld bytes in %1.3f sec %1.3f mbps",
        name, size, ts, mbps);
}

void bind_unix_socket(char* socket_name)
{
    struct sockaddr_un server;
    struct sockaddr_un remote;
    int sock, client, res, size, enable=1;
    int type = SOCK_STREAM;
    double ts, mbps;

    sock = socket(AF_UNIX, type, 0);
    if (sock < 0) {
        perror("server socket");
        exit(1);
    }

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (res < 0) {
        perror("server setsockopt");
        exit(1);
    }

    if (strlen(socket_name) > sizeof(server.sun_path)) {
        perror("server socket name length");
        exit(2);
    }

    memset(&server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, socket_name, sizeof(server.sun_path));
    unlink(server.sun_path);
    size = SUN_LEN(&server);

    res = bind(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        perror("server bind");
        exit(3);
    }

    res = listen(sock, QUEUE_SIZE);
    if (res < 0) {
        perror("server listen");
        exit(4);
    }

    LOG("server: listen %s type %d", server.sun_path, type);

    size = sizeof(remote);
    memset(&remote, 0, size);
    client = accept(sock, (struct sockaddr *)&remote, &size);
    if (client < 0) {
        perror("accept");
        exit(5);
    }

    LOG("server: accepted client");

    test_send("server", client);

    test_recv("server", client);

    close(client);
    close(sock);
    unlink(server.sun_path);
}

void bind_inet_socket(const char* ip_addr)
{
    struct sockaddr_in server;
    struct sockaddr_in remote;
    int sock, client, res, size, enable=1;
    int type = SOCK_STREAM;
    double ts, mbps;

    sock = socket(AF_INET, type, 0);
    if (sock < 0) {
        perror("server socket");
        exit(1);
    }

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (res < 0) {
        perror("server setsockopt");
        exit(1);
    }

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    if (res < 0) {
        perror("server setsockopt");
        exit(1);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_addr);
    server.sin_port = htons(TEST_PORT);
    size = sizeof(server);

    res = bind(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        perror("server bind");
        exit(3);
    }

    res = listen(sock, QUEUE_SIZE);
    if (res < 0) {
        perror("server listen");
        exit(4);
    }

    LOG("server: listen %s port %d type %d", ip_addr, TEST_PORT, type);

    size = sizeof(remote);
    memset(&remote, 0, size);
    client = accept(sock, (struct sockaddr *)&remote, &size);
    if (client < 0) {
        perror("accept");
        exit(5);
    }

    LOG("server: accepted client");

    test_send("server", client);

    test_recv("server", client);

    close(client);
    close(sock);
}

void bind_ipv6_socket(const char* ip6_addr)
{
    struct sockaddr_in6 server;
    struct sockaddr_in6 remote;
    int sock, client, res, size, enable=1;
    int type = SOCK_STREAM;
    double ts, mbps;

    sock = socket(AF_INET6, type, 0);
    if (sock < 0) {
        perror("server socket");
        exit(1);
    }

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (res < 0) {
        perror("server setsockopt");
        exit(1);
    }

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    if (res < 0) {
        perror("server setsockopt");
        exit(1);
    }

    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    res = inet_pton(AF_INET6, ip6_addr, &server.sin6_addr);
    server.sin6_port = htons(TEST_PORT);
    size = sizeof(server);

    if (res != 1) {
        LOG("server: error not valid ipv6 address: %s", ip6_addr);
        exit(2);
    }

    res = bind(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        LOG("server error bind socket %s:%d", ip6_addr, TEST_PORT);
        perror("server bind");
        exit(3);
    }

    res = listen(sock, QUEUE_SIZE);
    if (res < 0) {
        perror("server listen");
        exit(4);
    }

    LOG("server: listen %s port %d type %d", ip6_addr, TEST_PORT, type);

    size = sizeof(remote);
    memset(&remote, 0, size);
    client = accept(sock, (struct sockaddr *)&remote, &size);
    if (client < 0) {
        perror("accept");
        exit(5);
    }

    LOG("server: accepted client");

    test_send("server", client);

    test_recv("server", client);

    close(client);
    close(sock);
}

void start_server(int argc, char* argv[])
{
    if (argc > 1 && argv[1][0] != '-') {
        LOG("=== test unix sockets ===");

        bind_unix_socket(argv[1]);
    }

    if (argc > 2 && argv[2][0] != '-') {
        LOG("=== test inet sockets ===");

        bind_inet_socket("127.0.0.1");

        bind_inet_socket(argv[2]);
    }

    if (argc > 3 && argv[3][0] != '-') {
        LOG("=== test ipv6 sockets ===");

        bind_ipv6_socket("::1");

        bind_ipv6_socket(argv[3]);
    }

    LOG("exit server");
}

void test_unix_socket(char* socket_name)
{
    struct sockaddr_un server;
    int sock, res, size;
    int type = SOCK_STREAM;
    double ts;

    sock = socket(AF_UNIX, type, 0);
    if (sock < 0) {
        perror("client socket");
        exit(1);
    }

    if (strlen(socket_name) > sizeof(server.sun_path)) {
        perror("client socket name length");
        exit(2);
    }

    memset(&server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, socket_name, sizeof(server.sun_path));
    size = SUN_LEN(&server);

    ts = microtime();
    res = connect(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        perror("client connect");
        exit(3);
    }
    ts = microtime() - ts;
    LOG("client: connect %s time %1.6f", server.sun_path, ts);

    test_recv("client", sock);

    test_send("client", sock);

    close(sock);
}

void test_inet_socket(const char* ip_addr)
{
    struct sockaddr_in server;
    int sock, res, size;
    int type = SOCK_STREAM;
    double ts, mbps;

    sock = socket(AF_INET, type, 0);
    if (sock < 0) {
        perror("server socket");
        exit(1);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_addr);
    server.sin_port = htons(TEST_PORT);
    size = sizeof(server);

    ts = microtime();
    res = connect(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        LOG("client error client connect %s:%d", ip_addr, TEST_PORT);
        perror("client connect");
        exit(3);
    }
    ts = microtime() - ts;
    LOG("client: connect %s port %d time %1.6f", ip_addr, TEST_PORT, ts);

    test_recv("client", sock);

    test_send("client", sock);

    close(sock);
}

void test_ipv6_socket(const char* ip6_addr)
{
    struct sockaddr_in6 server;
    int sock, res, size;
    int type = SOCK_STREAM;
    double ts, mbps;

    sock = socket(AF_INET6, type, 0);
    if (sock < 0) {
        perror("server socket");
        exit(1);
    }

    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    res = inet_pton(AF_INET6, ip6_addr, &server.sin6_addr);
    server.sin6_port = htons(TEST_PORT);
    size = sizeof(server);

    if (res != 1) {
        LOG("client error not valid ipv6 address %s", ip6_addr);
        exit(2);
    }

    ts = microtime();
    res = connect(sock, (struct sockaddr *)&server, size);
    if (res < 0) {
        LOG("client error connect %s:%d", ip6_addr, TEST_PORT);
        perror("client connect");
        exit(3);
    }
    ts = microtime() - ts;
    LOG("client: connect %s port %d time %1.6f", ip6_addr, TEST_PORT, ts);

    test_recv("client", sock);

    test_send("client", sock);

    close(sock);
}

void start_client(int argc, char* argv[])
{
    if (argc > 1 && argv[1][0] != '-') {
        sleep(1);

        test_unix_socket(argv[1]);
    }

    if (argc > 2 && argv[2][0] != '-') {
        sleep(1);

        test_inet_socket("127.0.0.1");

        sleep(1);

        test_inet_socket(argv[2]);
    }

    if (argc > 3 && argv[3][0] != '-') {
        sleep(1);

        test_ipv6_socket("::1");

        sleep(1);

        test_ipv6_socket(argv[3]);
    }

    sleep(1);

    LOG("exit client");
}

void sig_handler(int signum)
{
    if (_server_pid)
        kill(_server_pid, 9);
    if (_client_pid)
        kill(_client_pid, 9);
    LOG("exit %d", signum);
    exit(signum);
}

int main(int argc, char* argv[])
{
    _start_microtime = microtime();

    if (argc < 2) {
        printf("usage: sock [socket_name] [ip_addr] [ip6_addr]\n");
        return 1;
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    _server_pid = fork();

    if (_server_pid == 0) {
        start_server(argc, argv);
        return 0;
    }

    _client_pid = fork();

    if (_client_pid == 0) {
        start_client(argc, argv);
        return 0;
    }

    while (waitpid(-1, NULL, WNOHANG) <= 0)
        usleep(1000);

    sleep(1);

    kill(_server_pid, 9);
    kill(_client_pid, 9);

    return 0;
}
