#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>

#define N 1000

const double tau = 0.000399;
const double eps = 1e-5;

void init_matrix(std::vector<double> &A)
{
#pragma omp parallel for
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            A[i * N + j] = (i == j) ? 2.0 : 1.0;
}

void init_vector(std::vector<double> &b)
{
#pragma omp parallel for
    for (int i = 0; i < N; i++)
        b[i] = N + 1;
}
// compare left part with right part and make correction
void correction(std::vector<double> &Ax, std::vector<double> &x, std::vector<double> &x_update, std::vector<double> &b, int num_threads)
{
#pragma omp parallel for num_threads(num_threads)
    for (size_t i = 0; i < x_update.size(); i++)

    {
        x_update[i] = x[i] - tau * (Ax[i] - b[i]);
    }
}
// calc an error
double calc_error(std::vector<double> &x, std::vector<double> &x_update, int num_threads)
{
    double error = 0.0;
#pragma omp parallel for reduction(+ : error) num_threads(num_threads)
    for (int i = 0; i < N; i++)

    {
        error += (x_update[i] - x[i]) * (x_update[i] - x[i]);
    }

    return std::sqrt(error);
}

void solve_v1(std::vector<double> &A, std::vector<double> &b,
              std::vector<double> &x, int num_threads)
{
    std::vector<double> x_update(x.size(), 0.0);
    std::vector<double> Ax(x.size(), 0.0);

    // calc left part of expression
    double error = 0.0;
    do

    {
#pragma omp parallel for num_threads(num_threads)
        for (int i = 0; i < N; i++)

        {
            x[i] = x_update[i];
        }
#pragma omp parallel for num_threads(num_threads)
        for (size_t i = 0; i < N; i++)

        {
            Ax[i] = 0;

            for (size_t j = 0; j < x.size(); j++)

            {
                Ax[i] += A[i * N + j] * x[j];
            }
        }

        correction(Ax, x, x_update, b, num_threads);
        error = calc_error(x, x_update, num_threads);

    } while (error > eps);
}

void solve_v2(std::vector<double> &A, std::vector<double> &b,
              std::vector<double> &x, int num_threads)
{
    std::vector<double> x_update(N, 0.0);
    std::vector<double> Ax(N, 0.0);
    double error = 0.0;

#pragma omp parallel num_threads(num_threads)

    {
        do

        {
#pragma omp for
            for (int i = 0; i < N; i++)

            {
                x[i] = x_update[i];
            }

#pragma omp for
            for (int i = 0; i < N; i++)

            {
                Ax[i] = 0;
                for (int j = 0; j < N; j++)

                {
                    Ax[i] += A[i * N + j] * x[j];
                }
            }

#pragma omp single

            {
                error = 0.0;
            }

#pragma omp for reduction(+ : error)
            for (int i = 0; i < N; i++)

            {
                x_update[i] = x[i] - tau * (Ax[i] - b[i]);
                error += (x_update[i] - x[i]) * (x_update[i] - x[i]);
            }

#pragma omp single

            {
                error = std::sqrt(error);
            }

        } while (error > eps);
    }
}

int main()
{
    std::vector<double> A(1000 * 1000);
    std::vector<double> b(N);
    std::vector<double> x1(N, 0.0);
    std::vector<double> x2(N, 0.0);

    std::vector<int> count_of_threads = {1, 2, 4, 7, 8, 16, 20, 40};

    init_matrix(A);
    init_vector(b);

    std::ofstream out("output.txt");

    for (int threads : count_of_threads)

    {
        // Variant 1
        std::cout << "V1: " << threads << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        solve_v1(A, b, x1, threads);
        auto end = std::chrono::high_resolution_clock::now();
        out << "Variant 1 Time: "
            << std::chrono::duration<double>(end - start).count() << " s\n";
        std::fill(x1.begin(), x1.end(), 0.0);
    }
    for (int threads : count_of_threads)

    {
        // Variant 2
        std::cout << "V2: " << threads << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        solve_v2(A, b, x2, threads);
        auto end = std::chrono::high_resolution_clock::now();
        out << "Variant 2 Time: "
            << std::chrono::duration<double>(end - start).count() << " s\n";
        std::fill(x2.begin(), x2.end(), 0.0);
    }
    out.close();
    return 0;
}