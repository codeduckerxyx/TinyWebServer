#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include <exception>

#include "../lock/lock.h"
#include "log.h"

Log::Log()
{
    m_log_stop = true;
    log_fp = -1;
    log_bufsize = 0;
}

Log::~Log()
{
    if( log_fp != -1 ){
        close( log_fp );
    }
    for(auto i:task_queue){
        delete [] i.ptr;
    }
    for(auto i:log_buf_queue){
        delete [] i;
    }
}

void Log::write_log(LOGLEVEL level,const char *format, ...)
{
    
    int log_buf_size = log_bufsize;
    int log_write_idx = 0;  //日志写缓冲坐标偏移
    char* log_buf;

    log_mutex.lock();
    log_buf_queue.front();
    log_buf_queue.pop_front();
    log_mutex.unlock();

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
        throw std::exception(); //字符缓冲区太小
    }
    log_write_idx += len;

    va_list valst;
    va_start(valst, format);
    len = vsnprintf( log_buf + log_write_idx, log_buf_size - log_write_idx, format, valst );
    va_end(valst);

    if( len >= log_buf_size - log_write_idx )
    {
        throw std::exception(); //字符缓冲区太小
    }
    log_write_idx += len;
    
    char* line_log = new char[ log_write_idx + 1 ];
    strncpy( line_log, log_buf, log_write_idx );
    line_log[ log_write_idx ] = '\n';

    log_mutex.lock();
    log_buf_queue.push_front( log_buf );
    log_mutex.unlock();

    task_mutex.lock();
    task_queue.push_back( task_node{ line_log, log_write_idx + 1 } );
    task_mutex.unlock();
    task_sem.post();
}

void Log::run_log()
{
    while( ! m_log_stop )
    {
        task_sem.wait();
        task_mutex.lock();

        task_node tem = task_queue.front();
        task_queue.pop_front();
        
        task_mutex.unlock();

        write( log_fp, tem.ptr, tem.size );
        delete [] tem.ptr;
    }
}

void Log::open_log()
{
    if( ! m_log_stop ) return;
    pthread_create( &tid, NULL, Log::worker, NULL );
    pthread_detach( tid );
    m_log_stop = false;
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
}

void Log::init( char* dirname, char *logname, int thread_number, int buf_size ){
    Log* m_log = get_instance();
    strcpy( m_log->dir_name,dirname );
    strcpy( m_log->log_name,logname );

    char* full_name = new char[ strlen( dirname ) + strlen( logname ) + 2 ];

    strcpy( full_name,dirname );
    full_name[ strlen( dirname ) ] = '/';
    strcpy( full_name + strlen(dirname) + 1 ,logname );
    full_name[ strlen( dirname ) + strlen( logname ) + 1 ] = '\0';

    if( m_log->log_fp != -1 ){
        close( m_log->log_fp );
    }

    m_log->log_fp = open( full_name, O_APPEND | O_CREAT | O_WRONLY );
    delete [] full_name;
    
    for( int i = 0; i< thread_number ; ++i ){
        char* log_buf = new char[buf_size];
        m_log->log_buf_queue.push_back( log_buf );
    }
    
}

