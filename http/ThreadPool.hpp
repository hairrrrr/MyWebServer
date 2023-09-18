#pragma once 

#include <pthread.h>

#include <queue>
#include <unordered_map>
#include <memory>

#include "HttpTask.hpp"

#define DEFAULT_NUMNER 6

template<class Task>
class ThreadPool 
{
public:
    static std::shared_ptr<ThreadPool<Task>> 
    GetInstance() 
    {
        static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        if( nullptr == _threadpool ) 
        {
            pthread_mutex_lock(&mutex);
            
            if( nullptr == _threadpool )
            {
                _threadpool.reset(new ThreadPool<Task>());
                LOGMESSAGE(INFO, DEBUG_MODE, "Thread Pool Create Success! [%p]", _threadpool);
            }

            pthread_mutex_unlock(&mutex);
        }
        return _threadpool;
    }


    void PushTask(const Task& task)
    {
        Lock();
        LOGMESSAGE(DEBUG, LOG_MODE, "new task add to threadpool!");
        _que.push(task);
        Unlock();
        Signal();
    }

    bool PopTask(Task* task)
    {
        if( TaskQueueEmpty() )
            return false;   
        
        *task = _que.front();
        _que.pop();

        return true;
    }


private:
    ThreadPool(int thread_num = DEFAULT_NUMNER) 
    {
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init (&cond, nullptr);
        
        for(int i = 0; i < thread_num; ++i) 
        {
            pthread_t tid;
            pthread_create(&tid, nullptr, Routine, this);
            pthread_detach(tid);
            tid2id[tid] = i;
        }
    }  
  
    static void* Routine(void* arg)
    {
        ThreadPool<Task>* thread_pool = static_cast<ThreadPool<Task>*>(arg);
        int id = thread_pool->GetId(gettid());

        while(true)
        {
            thread_pool->Lock();  

            while(thread_pool->TaskQueueEmpty())
                thread_pool->Wait();

            Task task;
            thread_pool->PopTask(&task);

            thread_pool->Unlock();

            LOGMESSAGE(DEBUG, LOG_MODE, "Thread #%d GetTask Success!", id);
                    
            task(id);
        }
    }

private:

    void Lock()   { pthread_mutex_lock(&mutex); }

    void Unlock() { pthread_mutex_unlock(&mutex); }

    void Wait()   { pthread_cond_wait(&cond, &mutex); }

    void Signal() { pthread_cond_signal(&cond); }

    bool TaskQueueEmpty() const 
    { return _que.empty(); }

    int GetId(pid_t tid)  { return  tid2id[tid]; }

private:
    static std::shared_ptr<ThreadPool<Task>>  _threadpool;
    std::queue<Task> _que;
    std::unordered_map<int, int> tid2id; 
    pthread_mutex_t mutex;
    pthread_cond_t  cond;        
};

template<class Task>
std::shared_ptr<ThreadPool<Task>> ThreadPool<Task>::_threadpool = nullptr;