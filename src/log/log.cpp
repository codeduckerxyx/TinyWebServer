#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include <exception>
#include <memory>

#include "../lock/lock.h"
#include "log.h"

Log::Log(){
    m_log_stop = true;
    log_fp = -1;
    log_bufsize = 0;
    m_timer = nullptr;
}

Log::~Log(){
    if( log_fp != -1 ){
        close( log_fp );
    }
    if( m_timer!=nullptr ){
        delete m_timer;
        m_timer = nullptr;
    }
}

void Log::write_log(int level,const char *format, ...){
    
    int log_buf_size = log_bufsize;
    int log_write_idx = 0;  //日志写缓冲坐标偏移

    log_tem_mutex.lock();
    std::unique_ptr<char[]> log_buf_ptr = std::move( log_buf_queue.front() );
    char *log_buf = log_buf_ptr.get();
    log_buf_queue.pop_front();
    log_tem_mutex.unlock();

    switch(level)
    {
        case LOG_LEVEL_ERROR:
        {
            strcpy( log_buf,"[erro]" );
            log_write_idx += 6;
            break;
        }
        case LOG_LEVEL_WARNING:
        {
            strcpy( log_buf,"[warn]" );
            log_write_idx += 6;
            break;
        }
        case LOG_LEVEL_INFO:
        {
            strcpy( log_buf,"[info]" );
            log_write_idx += 6;
            break;
        }
        case LOG_LEVEL_DEBUG:
        {
            strcpy( log_buf,"[debug]" );
            log_write_idx += 7;
            break;
        }
        case LOG_LEVEL_TRACE:
        {
            strcpy( log_buf,"[trace]" );
            log_write_idx += 7;
            break;
        }
        default:
        {
            break;
        }
    }
    
    struct timeval now_timeval;
    gettimeofday(&now_timeval, NULL);
    time_t t = now_timeval.tv_sec;
    struct tm *now_tm = localtime(&t);

    int len;
    len = snprintf( log_buf + log_write_idx, log_buf_size - log_write_idx,
                    " %4d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
                    now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_timeval.tv_usec );

    if( len >= log_buf_size - log_write_idx )
    {
        log_write_idx = log_buf_size;
    }else{
        log_write_idx += len;
    }

    va_list valst;
    va_start(valst, format);
    len = vsnprintf( log_buf + log_write_idx, log_buf_size - log_write_idx, format, valst );
    va_end(valst);

    if( len >= log_buf_size - log_write_idx )
    {
        log_write_idx = log_buf_size;
    }else{
        log_write_idx += len;
    }

    io_mutex.lock();
    while(1){
        if( log_write_idx + 1 <= io_buf_size - io_buf_idx[now]  ){
            strncpy( io_buf[now].get() + io_buf_idx[now], log_buf, log_write_idx );
            io_buf_idx[now] += log_write_idx ;
            io_buf[now][ io_buf_idx[now]++ ] = '\n';
            break;
        }else{
            io_sem.post();
            log_sem.wait();
        }
    }
    io_mutex.unlock();

    log_tem_mutex.lock();
    log_buf_queue.push_front( std::move( log_buf_ptr ) );
    log_tem_mutex.unlock();

}

void Log::io_timer(){
    io_mutex.lock();
    io_sem.post();
    log_sem.wait();
    io_mutex.unlock();

    time_t newtime = time(NULL) + 5;
    timer->adjust_timer( m_timer, newtime );
}

void Log::run_log()
{
    while( ! m_log_stop )
    {
        io_sem.wait();
        int last = now;
        now = (now+1)%2;
        log_sem.post();
        write( log_fp, io_buf[last].get(), io_buf_idx[last] );
        io_buf_idx[last] = 0;
    }
}

void Log::open_log()
{
    if( ! m_log_stop ) return;
    pthread_create( &tid, NULL, Log::worker, NULL );
    pthread_detach( tid );
    m_log_stop = false;
    
    m_timer->timer_id = -1;
    m_timer->expire = time(NULL) + 5;
    timer->add_timer( m_timer );
}

void Log::close_log()
{
    if( m_log_stop ) return;
    m_log_stop = true;
}

pthread_t Log::get_pid()
{
    return tid;
}

void* Log::worker(void* arg)
{
    get_instance()->run_log();
    return get_instance();
}

void Log::init(const char* dirname,const char *logname, int thread_number, int buf_size, timer_set* timer_tem  ){
    Log* m_log = get_instance();

    m_log->timer = timer_tem;
    m_log->m_timer = new log_timer;
    m_log->m_timer->ptr = m_log;

    m_log->log_bufsize = buf_size;
    strcpy( m_log->dir_name,dirname );
    strcpy( m_log->log_name,logname );

    std::unique_ptr<char[]> full_name( new char[ strlen( dirname ) + strlen( logname ) + 2 ] );

    strcpy( full_name.get(),dirname );
    full_name[ strlen( dirname ) ] = '/';
    strcpy( full_name.get() + strlen(dirname) + 1 ,logname );
    full_name[ strlen( dirname ) + strlen( logname ) + 1 ] = '\0';

    if( m_log->log_fp != -1 ){
        close( m_log->log_fp );
    }

    m_log->log_fp = open( full_name.get(), O_APPEND | O_CREAT | O_WRONLY );
    
    for( int i = 0; i< thread_number ; ++i ){
        std::unique_ptr<char []> ptr( new char[buf_size] );
        m_log->log_buf_queue.push_back( std::move( ptr ) );
    }

    m_log->io_buf[0].reset( new char[524288] );
    m_log->io_buf[1].reset( new char[524288] );
    m_log->io_buf_idx[0] = 0;
    m_log->io_buf_idx[1] = 0;
    m_log->io_buf_size = 524288;
}

