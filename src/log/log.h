#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <stdarg.h>
#include <pthread.h>
#include <list>
#include <unistd.h>
#include <fcntl.h>

#include "../lock/lock.h"


class Log{
    public:
        enum LOGLEVEL
        {   
            LOG_LEVEL_NONE=0,       // do not log
            LOG_LEVEL_ERROR,        // error
            LOG_LEVEL_WARNING,      // warning
            LOG_LEVEL_INFO,         // info	
            LOG_LEVEL_DEBUG,        // debug
            LOG_LEVEL_TRACE         // detailed information
        };
        enum LOGTARGET
        {
            LOG_TARGET_NONE=0,
            LOG_TARGET_CONSOLE,
            LOG_TARGET_FILE
        };
    private:
        
        typedef struct
        {
            char* ptr;
            int size;
        }task_node;
        locker task_mutex;
        std::list< task_node > task_queue;
        
        locker log_mutex;
        int log_bufsize;
        std::list< char* > log_buf_queue;

        char dir_name[256];     //路径名
        char log_name[256];     //文件名
        int log_fp;
        sem task_sem;
        pthread_t tid;
        bool m_log_stop;
    private:
        Log();
        void run_log();
        static void* worker(void* arg);
    public:
        virtual ~Log();
        pthread_t get_pid();
        void write_log(LOGLEVEL level,const char *format, ...); //异步写函数
        void open_log();
        void close_log();
        //C++11后，使用局部变量懒汉不用加锁
        static Log *get_instance()
        {
            static Log instance;
            return &instance;
        }
        static void init( char* dirname, char *logname, int thread_number, int buf_size );
};

#ifdef PRINTF_ERROR
    #define LOG_ERROR(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_ERROR,format, ##__VA_ARGS__);}
#else
    #define LOG_ERROR(format, ...)  
#endif

#ifdef PRINTF_WARNING
    #define LOG_WARN(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_WARNING, format, ##__VA_ARGS__);}
#else
    #define LOG_WARN(format, ...)  
#endif

#ifdef PRINTF_INFO
    #define LOG_INFO(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_INFO, format, ##__VA_ARGS__);}
#else
    #define LOG_INFO(format, ...)  
#endif

#ifdef PRINTF_DEBUG
    #define LOG_DEBUG(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__);}
#else
    #define LOG_DEBUG(format, ...)  
#endif

#ifdef PRINTF_TRACE
    #define LOG_TRACE(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_TRACE, format, ##__VA_ARGS__);}
#else
    #define LOG_TRACE(format, ...)  
#endif 

#endif