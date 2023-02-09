#include "utils.h"

Utils::Utils()
{
    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, Utils::sig_pipefd );
    assert( ret != -1 );
}

Utils::~Utils(){

}

int Utils::sig_pipefd[2];

int Utils::setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void Utils::sig_handler(int sig)
{
    int save_errno = errno; //保存原有errno
    int msg = sig;
    send(Utils::sig_pipefd[1], (char *)&msg, 1, 0);  //直接传一个char是因为信号值都小于等于64的，有用的只有前8位
    errno = save_errno;
}

//设置信号处理函数
void Utils::addsig( int sig, void( handler )(int), bool restart)
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}


void Utils::show_error( int connfd, const char* info )
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}


void Utils::addfd( int epollfd, int fd, bool one_shot ){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;  //EPOLLRDHUP客户端关闭
    if( one_shot )
    {
        event.events |= EPOLLONESHOT;   //一次触发后，epoll不再监听，等到下次用epoll_ctl的EPOLL_CTL_MOD来手动来添加
    }
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    Utils::setnonblocking( fd );
}

void Utils::removefd( int epollfd, int fd )
{
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}

void Utils::modfd( int epollfd, int fd, int ev )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}