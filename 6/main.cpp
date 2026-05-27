#include <iostream>
#include <cmath>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <boost/program_options.hpp>

using namespace std;
using namespace std::chrono;

namespace po = boost::program_options;

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
    int N = 128;
    int max_iter = 1000000;
    double eps = 1e-6;

    try
    {
        po::options_description desc("Options");
        desc.add_options()("help,h", "show help")("size", po::value<int>(&N)->default_value(128), "grid size")("iter", po::value<int>(&max_iter)->default_value(1000000), "max iterations")("eps", po::value<double>(&eps)->default_value(1e-6), "accuracy");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << endl;
            return 0;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    double *u = new double[N * N];
    double *u_new = new double[N * N];

    fill(u, u + N * N, 0.0);
    fill(u_new, u_new + N * N, 0.0);
    initialize_boundaries(u, N);
    initialize_boundaries(u_new, N);

    int iter = 0;
    double error = 0.0;

    auto start = high_resolution_clock::now();

#ifdef _OPENACC
#pragma acc data copy(u[0 : N * N], u_new[0 : N * N])
    {
        for (iter = 0; iter < max_iter; ++iter)
        {
            error = 0.0;

#pragma acc parallel loop gang reduction(max : error)
            for (int i = 1; i < N - 1; ++i)
            {
#pragma acc loop vector reduction(max : error)
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

#pragma acc parallel loop gang
            for (int i = 0; i < N; ++i)
            {
#pragma acc loop vector
                for (int j = 0; j < N; ++j)
                {
                    u[idx(i, j, N)] = u_new[idx(i, j, N)];
                }
            }

            if (error < eps)
                break;
        }
    }
#else
    double *cur = u;
    double *next = u_new;

    for (iter = 0; iter < max_iter; ++iter)
    {
        error = 0.0;

#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(max : error)
#endif
        for (int i = 1; i < N - 1; ++i)
        {
            for (int j = 1; j < N - 1; ++j)
            {
                next[idx(i, j, N)] = 0.25 * (cur[idx(i - 1, j, N)] +
                                             cur[idx(i + 1, j, N)] +
                                             cur[idx(i, j - 1, N)] +
                                             cur[idx(i, j + 1, N)]);
                double diff = fabs(next[idx(i, j, N)] - cur[idx(i, j, N)]);
                if (diff > error)
                    error = diff;
            }
        }

        swap(cur, next);

        if (error < eps)
            break;
    }

    if (cur != u)
    {
        copy(cur, cur + N * N, u);
    }
#endif

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