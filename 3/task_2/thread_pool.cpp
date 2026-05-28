#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <cmath>
#include <map>
#include <fstream>
#include <random>
#include <memory>

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : stop(false)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this]
                                 {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                } });
        }
    }

    ~ThreadPool()
    {
        stop = true;
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using return_type = typename std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task]()
                          { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

template <typename T>
class TaskServer
{
private:
    ThreadPool pool;
    std::map<size_t, T> results;
    std::mutex results_mtx;
    std::condition_variable result_cv;
    size_t last_id = 0;
    bool running = false;

public:
    explicit TaskServer(size_t num_threads = std::thread::hardware_concurrency())
        : pool(num_threads), running(true) {}

    ~TaskServer()
    {
        stop();
    }

    void stop()
    {
        running = false;
        result_cv.notify_all();
    }

    size_t add_task(std::function<T()> task)
    {
        size_t id;
        {
            std::lock_guard<std::mutex> lock(results_mtx);
            id = ++last_id;
        }

        pool.enqueue([this, id, task]()
                     {
                         T res = task();
                         {
                             std::lock_guard<std::mutex> lock(results_mtx);
                             results[id] = res;
                         }
                         result_cv.notify_all(); });
        return id;
    }

    T request_result(size_t id)
    {
        std::unique_lock<std::mutex> lock(results_mtx);
        result_cv.wait(lock, [this, id]
                       { return results.find(id) != results.end(); });
        T res = results[id];
        results.erase(id);
        return res;
    }
};

template <typename T>
T fun_sin(T arg) { return std::sin(arg); }
template <typename T>
T fun_sqrt(T arg) { return std::sqrt(arg); }
template <typename T>
T fun_pow(T arg) { return std::pow(arg, 2.0); }

void client_process(TaskServer<double> &server, int client_type, int N)
{
    std::ofstream out("client_" + std::to_string(client_type) + ".txt");
    if (!out)
    {
        std::cerr << "Не удалось открыть файл для клиента " << client_type << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(1.0, 100.0);
    std::vector<size_t> ids;

    for (int i = 0; i < N; ++i)
    {
        double val = dist(gen);
        size_t id;
        switch (client_type)
        {
        case 1:
            id = server.add_task([val]()
                                 { return fun_sin(val); });
            break;
        case 2:
            id = server.add_task([val]()
                                 { return fun_sqrt(val); });
            break;
        case 3:
            id = server.add_task([val]()
                                 { return fun_pow(val); });
            break;
        default:
            id = 0;
        }
        ids.push_back(id);
        out << "ID: " << id << " | Arg: " << val;
    }

    for (size_t i = 0; i < ids.size(); ++i)
    {
        double res = server.request_result(ids[i]);
        out << " | Res: " << res << "\n";
    }
    out.close();
}

int main()
{
    const int NUM_THREADS = 3;
    const int N = 10;

    TaskServer<double> server(NUM_THREADS);

    std::thread c1(client_process, std::ref(server), 1, N);
    std::thread c2(client_process, std::ref(server), 2, N);
    std::thread c3(client_process, std::ref(server), 3, N);

    c1.join();
    c2.join();
    c3.join();

    std::cout << "Все клиенты завершили работу. Результаты записаны в файлы:\n";
    std::cout << "client_1.txt, client_2.txt, client_3.txt\n";
    return 0;
}