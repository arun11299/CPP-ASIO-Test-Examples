#include <boost/asio.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <thread>

std::atomic<bool> running{true};
std::atomic<int> counter{0};

struct Work
{
    Work(boost::asio::io_service & io_service)
        : _strand(io_service)
    { }

    static void start_the_work(boost::asio::io_service & io_service)
    {
        std::shared_ptr<Work> _this(new Work(io_service));

        _this->_strand.get_io_service().post(_this->_strand.wrap(std::bind(do_the_work, _this)));
    }

    static void do_the_work(std::shared_ptr<Work> _this)
    {
        counter.fetch_add(1, std::memory_order_relaxed);

        if (running.load(std::memory_order_relaxed)) {
            start_the_work(_this->_strand.get_io_service());
        }
    }

    boost::asio::strand _strand;
};

struct BlockingWork
{
    BlockingWork(boost::asio::io_service & io_service)
        : _strand(io_service)
    { }

    static void start_the_work(boost::asio::io_service & io_service)
    {
        std::shared_ptr<BlockingWork> _this(new BlockingWork(io_service));

         _this->_strand.get_io_service().post(_this->_strand.wrap(std::bind(do_the_work, _this)));
    }

    static void do_the_work(std::shared_ptr<BlockingWork> _this)
    {
        sleep(5);
    }

    boost::asio::strand _strand;
};


int main(int argc, char ** argv)
{
    boost::asio::io_service io_service;
    std::unique_ptr<boost::asio::io_service::work> work{new boost::asio::io_service::work(io_service)};

    for (std::size_t i = 0; i < 8; ++i) {
        Work::start_the_work(io_service);
    }

    std::vector<std::thread> workers;

    for (std::size_t i = 0; i < 8; ++i) {
        workers.push_back(std::thread([&io_service] {
            io_service.run();
        }));
    }

    if (argc > 1) {
        std::cout << "Spawning a blocking work" << std::endl;
        workers.push_back(std::thread([&io_service] {
            io_service.run();
        }));
        BlockingWork::start_the_work(io_service);
    }

    sleep(5);
    running = false;
    work.reset();

    for (auto && worker : workers) {
        worker.join();
    }

    std::cout << "Work performed:" << counter.load() << std::endl;
    return 0;
}
