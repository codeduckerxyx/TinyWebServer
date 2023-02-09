#include "server.h"

WebServer::WebServer()
{
    
}

WebServer::~WebServer()
{
    delete [] users;
    delete [] events;
    delete [] signals;
    if( pool!=NULL ){
        delete pool;
    }
}

void WebServer::init(){
    users = new http_conn[ MAX_FD ];
    LOG_ERROR("user new failed");
    assert( users );

    events = new epoll_event[ MAX_EVENT_NUMBER ];
    LOG_ERROR("events new failed");
    assert( events );

    signals = new char[1024];
    LOG_ERROR("signals new failed");
    assert( signals );
    
    m_stop_server = false;
}


void WebServer::init_thread_pool( int thread_number, int max_requests )
{
    m_thread_num = thread_number;
    try
    {
        pool = new threadpool< http_conn >( thread_number,max_requests );  //线程数量与请求队列容量
    }
    catch( ... )
    {
        LOG_ERROR("Thread pool initialization failed");
        throw std::exception();
    }
    LOG_INFO("Thread pool initialization succeeded");
}


void WebServer::init_log( bool log_open, const char* dirname, const char* logname, int log_bufsize)
{
    m_log_stop = (!log_open);
    if( m_log_stop == false ){
        Log::init( dirname, logname,m_thread_num+1,log_bufsize );
        Log::get_instance()->open_log();
    }
}


void WebServer::init_timer(){}


void WebServer::deal_client_connection_handler(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
    if ( connfd < 0 )
    {
        LOG_INFO( "accept failed errno is %d", errno );
        return;
    }
    if( http_conn::m_user_count >= MAX_FD )
    {
        Utils::show_error( connfd, "Internal server busy" );
        return;
    }
    users[connfd].init( connfd, client_address );
}

void WebServer::deal_sigal_handler(){
    int ret = recv( utils.sig_pipefd[0], signals, 1024, 0 );
    if( ret <= 0 ){
        return;
    }else{
        for( int i = 0; i < ret; ++i ){
            switch( signals[i] ){

            }
        }
    }
}

void WebServer::deal_recvdata_handler( int sockfd ){
    if( users[sockfd].read() )
    {
        pool->append( users + sockfd );
    }
    else
    {
        users[sockfd].close_conn();
    }
}

void WebServer::deal_senddata_handler( int sockfd ){
    if( !users[sockfd].write() )
    {
        users[sockfd].close_conn();
    }
}

void WebServer::start_listen(const char* ip, int port ){
    listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    struct linger tmp = { 1, 0 };
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );     //close设置为强制关闭

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    
    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret >= 0 );
    
    ret = listen( listenfd, 5 );
    assert( ret >= 0 );
}


void WebServer::eventLoop(){
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );

    Utils::addfd( epollfd, listenfd, false );

    http_conn::m_epollfd = epollfd;
    http_conn::m_user_count = 0;
    
    Utils::setnonblocking(Utils::sig_pipefd[1]);
    Utils::addfd( epollfd, Utils::sig_pipefd[0], false );

    Utils::addsig(SIGPIPE, SIG_IGN);
    //utils.addsig(SIGALRM, Utils::sig_handler, false);
    Utils::addsig(SIGTERM, Utils::sig_handler, false);

    LOG_INFO("Server started successfully");

    while( ! m_stop_server )
    {
        int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; i++ )
        {
            int sockfd = events[i].data.fd;
            if( sockfd == listenfd )
            {
                deal_client_connection_handler();
                
            }
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
            {
                users[sockfd].close_conn();
                //移除对应定时器
            }
            else if( ( sockfd == utils.sig_pipefd[0] ) && events[i].events & EPOLLIN )
            {
                deal_sigal_handler();
            }
            else if( events[i].events & EPOLLIN )
            {
                deal_recvdata_handler( sockfd );
            }
            else if( events[i].events & EPOLLOUT )
            {
                deal_senddata_handler( sockfd );
            }
            else
            {}
        }
    }

}