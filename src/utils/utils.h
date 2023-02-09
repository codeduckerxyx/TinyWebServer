#ifndef UTILS_H
#define UTILS_H

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
#include <signal.h>

class Utils{
    public:
        Utils();
        ~Utils();
        static int sig_pipefd[2];
        static void sig_handler(int sig);
        static void addsig( int sig, void( handler )(int), bool restart = true );
        static void show_error( int connfd, const char* info );
        static void addfd( int epollfd, int fd, bool one_shot );
        static void removefd( int epollfd, int fd );
        static int setnonblocking( int fd );
        static void modfd( int epollfd, int fd, int ev );
};

#endif