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

template <typename T>
class TaskServer
{
private:
    std::vector<std::jthread> workers;
    std::queue<std::pair<size_t, std::function<T()>>> tasks;
    std::map<size_t, T> results;

    std::mutex mtx;
    std::condition_variable cv;
    size_t last_id = 0;
    bool running = false;

public:
    void start(int num_threads)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (running)
            return;
        running = true;
        for (int i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this](std::stop_token st)
                                 {
                while (!st.stop_requested()) {
                    std::pair<size_t, std::function<T()>> task_pair;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this, &st] { return !tasks.empty() || st.stop_requested(); });
                        if (st.stop_requested() && tasks.empty()) return;
                        
                        task_pair = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    T res = task_pair.second();
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        results[task_pair.first] = res;
                    }
                    cv.notify_all(); 
                } });
        }
    }

    void stop()
    {
        workers.clear();
        running = false;
    }

    size_t add_task(std::function<T()> task)
    {
        std::lock_guard<std::mutex> lock(mtx);
        size_t id = ++last_id;
        tasks.push({id, std::move(task)});
        cv.notify_one();
        return id;
    }

    T request_result(size_t id)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this, id]
                { return results.count(id); });
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
T fun_pow(T x, T y) { return std::pow(x, y); }

void client_process(TaskServer<double> &server, int client_type, int N)
{
    std::ofstream out("client_" + std::to_string(client_type) + ".txt");
    std::vector<size_t> ids;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(1.0, 100.0);

    for (int i = 0; i < N; ++i)
    {
        double val = dist(gen);
        size_t id;
        if (client_type == 1)
            id = server.add_task([val]
                                 { return fun_sin(val); });
        else if (client_type == 2)
            id = server.add_task([val]
                                 { return fun_sqrt(val); });
        else
            id = server.add_task([val]
                                 { return fun_pow(val, 2.0); });
        ids.push_back(id);
        out << "ID: " << id << " | Arg: " << val;


        double res = server.request_result(id);
        out << " | Res: " << res << "\n";
    }
}

int main()
{
    TaskServer<double> server;
    server.start(4);

    std::thread c1(client_process, std::ref(server), 1, 10);
    std::thread c2(client_process, std::ref(server), 2, 10);
    std::thread c3(client_process, std::ref(server), 3, 10);

    c1.join();
    c2.join();
    c3.join();
    server.stop();
    return 0;
}