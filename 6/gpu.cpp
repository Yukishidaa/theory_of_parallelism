#include <iostream>
#include <cmath>
#include <fstream>
#include <chrono>
#include <cstring>
#include <getopt.h>

using namespace std;
using namespace std::chrono;

inline int idx(int i, int j, int N)
{
    return i * N + j;
}

void initialize_boundaries(double *grid, int N)
{
    double top_left = 10.0;
    double top_right = 20.0;
    double bottom_right = 30.0;
    double bottom_left = 20.0;

    for (int j = 0; j < N; ++j)
        grid[idx(0, j, N)] = top_left + (top_right - top_left) * j / (N - 1);
    for (int j = 0; j < N; ++j)
        grid[idx(N - 1, j, N)] = bottom_left + (bottom_right - bottom_left) * j / (N - 1);
    for (int i = 0; i < N; ++i)
        grid[idx(i, 0, N)] = top_left + (bottom_left - top_left) * i / (N - 1);
    for (int i = 0; i < N; ++i)
        grid[idx(i, N - 1, N)] = top_right + (bottom_right - top_right) * i / (N - 1);
}

void save_matrix(double *grid, int N, const string &filename)
{
    ofstream out(filename);
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
            out << grid[idx(i, j, N)] << " ";
        out << "\n";
    }
}

void print_matrix(double *grid, int N)
{
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
            printf("%8.2f ", grid[idx(i, j, N)]);
        cout << endl;
    }
}

int main(int argc, char *argv[])
{
#ifdef _OPENACC
    acc_init(acc_device_nvidia);
    if (acc_get_device_type() == acc_device_nvidia)
        std::cout << "GPU NVIDIA используется" << std::endl;
    else
        std::cout << "Эмуляция или другой device" << std::endl;
#endif
    int N = 128;
    int max_iter = 1000000;
    double eps = 1e-6;

    int opt;
    while ((opt = getopt(argc, argv, "n:i:e:h")) != -1)
    {
        switch (opt)
        {
        case 'n':
            N = atoi(optarg);
            break;
        case 'i':
            max_iter = atoi(optarg);
            break;
        case 'e':
            eps = atof(optarg);
            break;
        case 'h':
            cout << "Usage: " << argv[0] << " [-n size] [-i max_iter] [-e eps]" << endl;
            return 0;
        default:
            cerr << "Unknown option" << endl;
            return 1;
        }
    }

    double *u = new double[N * N];
    double *u_new = new double[N * N];

    memset(u, 0, N * N * sizeof(double));
    memset(u_new, 0, N * N * sizeof(double));
    initialize_boundaries(u, N);
    initialize_boundaries(u_new, N);

    int iter = 0;
    double error = 0.0;

    auto start = high_resolution_clock::now();

#pragma acc data copy(u[0 : N * N], u_new[0 : N * N])
    {
        for (iter = 0; iter < max_iter; ++iter)
        {
            error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max : error)
            for (int i = 1; i < N - 1; ++i)
            {
                for (int j = 1; j < N - 1; ++j)
                {
                    u_new[idx(i, j, N)] = 0.25 * (u[idx(i - 1, j, N)] +
                                                  u[idx(i + 1, j, N)] +
                                                  u[idx(i, j - 1, N)] +
                                                  u[idx(i, j + 1, N)]);
                    double diff = fabs(u_new[idx(i, j, N)] - u[idx(i, j, N)]);
                    if (diff > error)
                        error = diff;
                }
            }

#pragma acc parallel loop collapse(2)
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                    u[idx(i, j, N)] = u_new[idx(i, j, N)];

            if (error < eps)
                break;
        }
    }

    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<milliseconds>(end - start).count();

    cout << "Iterations: " << iter << endl;
    cout << "Error: " << error << endl;
    cout << "Time: " << elapsed << " ms" << endl;

    save_matrix(u, N, "result.txt");

    if (N == 10 || N == 13)
    {
        print_matrix(u, N);
    }

    delete[] u;
    delete[] u_new;
    return 0;
}