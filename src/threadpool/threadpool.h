#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <memory>

#include "../lock/lock.h"
#include "../log/log.h"

/* 线程池类，定义为模板类 */
template< typename T >
class threadpool
{
public:
    /* 参数thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的，等待处理的请求数量 */
    threadpool( int thread_number = 8, int max_requests = 10000 );
    ~threadpool();
    /* 往请求队列中添加任务 */
    bool append( T* request );
private:
    /* 工作线程运行的函数，不断从工作队列中去任务并执行 */
    static void* worker( void* arg );
    void run();
private:
    int m_thread_number;    /*线程池中的线程数*/
    int m_max_requests;     /* 请求队列中允许的最大请求数 */
    std::unique_ptr<pthread_t[]> m_threads;   /* 线程池数组，大小为 m_thread_number */
    std::list< T* > m_workqueue;    /* 请求队列 */
    locker m_queuelocker;   /* 保护请求队列的互斥锁 */
    sem m_queuestat;        /* 是否有任务需要处理 */
    bool m_stop;            /* 是否结束线程 */
};

template< typename T >
threadpool< T >::threadpool( int thread_number, int max_requests ) : m_thread_number( thread_number ), m_max_requests( max_requests ), m_stop( false ), m_threads( nullptr )
{
    if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
    {
        LOG_ERROR("thread_number shoud be > 0 and max_requests shoud be > 0");
        throw std::exception();
    }

    m_threads.reset( new pthread_t[thread_number] );
    if( m_threads == nullptr )
    {
        LOG_ERROR("m_threads new failed");
        throw std::exception();
    }

    /* 创建thread_number个线程，并将它们都设置为脱离线程 */
    for( int i = 0; i < thread_number; ++i )
    {
        LOG_INFO( "create the %dth thread", i );
        if( pthread_create( m_threads.get() + i, NULL, worker, this ) != 0 )
        {
            LOG_ERROR("pthread_create failed");
            throw std::exception();
        }
        if( pthread_detach( m_threads[i] ) )
        {
            LOG_ERROR("pthread_detach failed");
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool()
{
    m_stop = true;
}

template< typename T >
bool threadpool< T > ::append( T* request )
{
    m_queuelocker.lock();
    if( m_workqueue.size() > m_max_requests ){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back( request );
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template< typename T >
void* threadpool< T >::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run()
{
    while( ! m_stop ){
        m_queuestat.wait();
        m_queuelocker.lock();
        if( m_workqueue.empty() )
        {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if( request == nullptr ){
            continue;
        }
        request->process();
    }
}
#endif