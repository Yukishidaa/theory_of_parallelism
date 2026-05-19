#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <iomanip>

// Функция для инициализации части данных
void init_data(double *matrix, double *vec, int start, int end, int N)
{
    for (int i = start; i < end; ++i)
    {
        vec[i] = static_cast<double>(N - i);
        for (int j = 0; j < N; ++j)
        {
            matrix[i * N + j] = static_cast<double>(j + 1);
        }
    }
}

// Функция для умножения части матрицы на вектор
void multiply(const double *matrix, const double *vec, double *res, int start_row, int end_row, int N)
{
    for (int i = start_row; i < end_row; ++i)
    {
        double sum = 0;
        for (int j = 0; j < N; ++j)
        {
            sum += matrix[i * N + j] * vec[j];
        }
        res[i] = sum;
    }
}

int main(int argc, char *argv[])
{
    // По умолчанию N=20000, или берем из аргументов
    const int N = (argc > 2) ? std::atoi(argv[2]) : 20000;
    const int NUM_THREADS = (argc > 1) ? std::atoi(argv[1]) : 1;

    std::vector<double> matrix(static_cast<size_t>(N) * N);
    std::vector<double> vec(N);
    std::vector<double> res(N, 0.0);

    std::vector<std::thread> threads;
    int chunk = N / NUM_THREADS;

    auto start_init = std::chrono::steady_clock::now();
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        int start = i * chunk;
        int end = (i == NUM_THREADS - 1) ? N : start + chunk;
        threads.emplace_back(init_data, matrix.data(), vec.data(), start, end, N);
    }
    for (auto &t : threads)
        t.join();
    threads.clear();
    auto end_init = std::chrono::steady_clock::now();

    auto start_work = std::chrono::steady_clock::now();
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        int start = i * chunk;
        int end = (i == NUM_THREADS - 1) ? N : start + chunk;
        threads.emplace_back(multiply, matrix.data(), vec.data(), res.data(), start, end, N);
    }
    for (auto &t : threads)
        t.join();
    auto end_work = std::chrono::steady_clock::now();

    std::chrono::duration<double> diff_init = end_init - start_init;
    std::chrono::duration<double> diff_work = end_work - start_work;

    std::cout << "Threads: " << NUM_THREADS << " | N: " << N << std::endl;
    std::cout << "Init time: " << std::fixed << std::setprecision(4) << diff_init.count() << " s" << std::endl;
    std::cout << "Work time: " << std::fixed << std::setprecision(4) << diff_work.count() << " s" << std::endl;
    std::cout << "Total:     " << diff_init.count() + diff_work.count() << " s" << std::endl;

    return 0;
}