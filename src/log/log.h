#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <stdarg.h>
#include <pthread.h>
#include <list>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

#include "../timer/timer_set.h"
#include "../lock/lock.h"

class log_timer;

class Log{
    public:
        // enum LOGLEVEL
        // {   
        //     LOG_LEVEL_NONE=0,       // do not log
        //     LOG_LEVEL_ERROR,        // error
        //     LOG_LEVEL_WARNING,      // warning
        //     LOG_LEVEL_INFO,         // info	
        //     LOG_LEVEL_DEBUG,        // debug
        //     LOG_LEVEL_TRACE         // detailed information
        // };
        enum LOGTARGET
        {
            LOG_TARGET_NONE=0,
            LOG_TARGET_CONSOLE,
            LOG_TARGET_FILE
        };
        bool m_log_stop;
    private:
        
        locker io_mutex;   //写入缓冲数据锁
        std::shared_ptr<char[]> io_buf[2];
        int io_buf_idx[2];
        int io_buf_size;
        int now;

        locker log_tem_mutex;      //单线程日志缓冲锁
        int log_bufsize;
        std::list< std::unique_ptr<char[]> > log_buf_queue;

        sem io_sem;
        sem log_sem;

        char dir_name[256];     //路径名
        char log_name[256];     //文件名
        int log_fp;
        pthread_t tid;

        timer_set* timer;
        log_timer* m_timer;

    private:
        Log();
        void run_log();
        static void* worker(void* arg);
    public:
        virtual ~Log();
        pthread_t get_pid();
        void write_log(int level,const char *format, ...); //异步写函数
        void open_log();
        void close_log();
        void io_timer();
        //C++11后，使用局部变量懒汉不用加锁
        static Log *get_instance()
        {
            static Log instance;
            return &instance;
        }
        static void init(const char* dirname,const char *logname, int thread_number, int buf_size, timer_set* timer_tem );
};

class log_timer : public util_timer_node{
    public:
        Log* ptr;
        void process()
        {
            ptr->io_timer();
        }
};

#define LOG_LEVEL_NONE 0        // do not log
#define LOG_LEVEL_ERROR 1       // error
#define LOG_LEVEL_WARNING 2     // warning
#define LOG_LEVEL_INFO 4        // info	
#define LOG_LEVEL_DEBUG 8       // debug
#define LOG_LEVEL_TRACE 16

#define LOG_ERROR(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_ERROR,format, ##__VA_ARGS__);}
#define LOG_WARN(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_WARNING, format, ##__VA_ARGS__);}
#define LOG_INFO(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_INFO, format, ##__VA_ARGS__);}
#define LOG_DEBUG(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__);}
#define LOG_TRACE(format, ...) if( ! Log::get_instance()->m_log_stop ) {Log::get_instance()->write_log(LOG_LEVEL_TRACE, format, ##__VA_ARGS__);}

#endif