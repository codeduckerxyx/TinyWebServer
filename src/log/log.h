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

#ifdef PRINTF_ERROR
    #define LOG_ERROR(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_ERROR,log_buf,log_buf_size, format, ##__VA_ARGS__);}
#else
    #define LOG_ERROR(format, ...)  
#endif

#ifdef PRINTF_WARNING
    #define LOG_WARN(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_WARNING,log_buf,log_buf_size, format, ##__VA_ARGS__);}
#else
    #define LOG_WARN(format, ...)  
#endif

#ifdef PRINTF_INFO
    #define LOG_INFO(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_INFO,log_buf,log_buf_size, format, ##__VA_ARGS__);}
#else
    #define LOG_INFO(format, ...)  
#endif

#ifdef PRINTF_DEBUG
    #define LOG_DEBUG(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_DEBUG,log_buf,log_buf_size, format, ##__VA_ARGS__);}
#else
    #define LOG_DEBUG(format, ...)  
#endif

#ifdef PRINTF_TRACE
    #define LOG_TRACE(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_TRACE,log_buf,log_buf_size, format, ##__VA_ARGS__);}
#else
    #define LOG_TRACE(format, ...)  
#endif 

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
        void write_log(LOGLEVEL level,char* log_buf,int log_buf_size,const char *format, ...); //异步写函数
        void open_log();
        void close_log();
        //C++11后，使用局部变量懒汉不用加锁
        static Log *get_instance()
        {
            static Log instance;
            return &instance;
        }
        static Log* init( char* dirname, char *logname );
};

#endif