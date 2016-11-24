#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <asio.hpp>

class ThreadPool {
public:
    asio::io_service io_service;
    boost::thread_group threads;
    asio::io_service::work work {io_service} ;
    ThreadPool();
    void call();
    void calling(); 
};

ThreadPool::ThreadPool() {
    /* Create thread-pool now */
    size_t numThreads = boost::thread::hardware_concurrency();
    for(size_t t = 0; t < numThreads; t++) {
        threads.create_thread(boost::bind(&asio::io_service::run, &io_service));
    }
}

void ThreadPool::call() {
    std::cout << "Hi i'm thread no " << boost::this_thread::get_id() << std::endl;
};

void ThreadPool::calling() {
    sleep(1); 
    io_service.post(boost::bind(&ThreadPool::call, this));
}

int main(int argc, char **argv)
{
   ThreadPool pool;
   for (int i = 0; i < 5; i++) {
    pool.calling();
   }
   pool.threads.join_all();
   return 0;
}
