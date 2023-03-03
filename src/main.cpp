#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <memory>

#include "server/server.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

int main()
{
    std::unique_ptr<WebServer> server = std::make_unique<WebServer>();
    server->init_log( true, "/home/ubuntu/web_server/doc", "1.log", 1024  );
    server->init();
    server->init_thread_pool(1,10000);
    server->start_listen("0.0.0.0",22222);
    server->eventLoop();
    return 0;
}