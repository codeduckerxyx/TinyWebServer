#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <exception>

#include "../threadpool/threadpool.h"
#include "../log/log.h"
#include "../http/http_conn.h"
#include "../lock/lock.h"
#include "../timer/timer_set.h"
#include "../utils/utils.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时时间

class WebServer{
    public:
        WebServer();
        ~WebServer();
    
        void init();
        void init_thread_pool( int thread_number, int max_requests );
        void init_log( bool log_open, const char* dirname, const char *logname, int log_bufsize);
        void init_timer();
        void start_listen( const char* ip, int port  );
        void eventLoop();

    public:
        int listenfd;
        Utils utils;
        bool m_stop_server;
        char* signals;

        /* 日志 */
        bool m_log_stop;
        
        /* 客户数据 */
        http_conn* users;

        /* 线程池 */
        threadpool< http_conn >* pool;
        int m_thread_num;

        /* io复用 */
        epoll_event* events;

        /* 定时器 */

        /* 数据库 */

    private:
        void deal_client_connection_handler();
        void deal_sigal_handler();
        void deal_recvdata_handler( int sockfd );
        void deal_senddata_handler( int sockfd );
};

#endif