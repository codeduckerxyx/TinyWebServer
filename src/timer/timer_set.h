#ifndef TIMER_SET_h
#define TIMER_SET_h

/**
 * @file timer_set.h
 * 继承 util_timer_node 后重写process作为回调函数即可
 */

#include<netinet/in.h>
#include <time.h>
#include <set>

#define BUFFER_SIZE 64

class http_timer;   //前向声明

class util_timer_node
{
    public:
        util_timer_node(){}
        virtual ~util_timer_node(){}    /* 确保调用到正确的析构函数 */
    public:
        time_t expire;  /* 任务的超时时间，绝对时间 */
        int timer_id;   /* 每一个定时任务都对应一个标识码,需要调用者自定义，并保证不会出现重复 */
        virtual void process() = 0;
};

struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    http_timer* timer;
};

class http_timer : public util_timer_node
{
    public:
        client_data* user_data;
        void (*cb_func)(client_data*);  /* 回调函数 */

        void process()  /* 每个具体的timer必须实现process方法，以后再有时间再回来改成虚函数继承的结构 */
        {
            cb_func( user_data );
        }
};

struct set_comp
{
    bool operator ()( util_timer_node* a, util_timer_node* b) const
    {
        if( a->expire != b->expire ) return a->expire < b->expire;
        return a->timer_id < b->timer_id;
    }
};

class timer_set
{
    private:
        std::set<util_timer_node*,set_comp> set;
    public:
        timer_set(){}
        ~timer_set()
        {
            for(util_timer_node* tmp:set){
                delete tmp;
            }
        }
        /* 添加定时任务 */
        void add_timer( util_timer_node* timer ){
            set.insert( timer );
        }
        /* 删除定时任务 */
        void del_timer( util_timer_node* timer ){
            auto tmp = set.lower_bound( timer );
            util_timer_node* ptr = *tmp;
            set.erase( tmp );
            delete ptr;
        }
        /* 调整任务结束时间 */
        void adjust_timer( util_timer_node* timer,time_t new_end_time ){
            auto tmp = set.lower_bound( timer );
            set.erase( tmp );

            timer->expire = new_end_time;
            set.insert( timer );
        }

        void tick(){
            time_t cur = time( NULL );
            while( !set.empty() ){
                util_timer_node* ptr = *set.begin();
                if( ptr->expire <= cur ){
                    set.erase( set.begin() );
                    ptr->process();
                    delete ptr;
                }else{
                    break;
                }
            }
        }
};
#endif