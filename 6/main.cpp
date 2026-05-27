#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <boost/program_options.hpp>

using namespace std;
using namespace std::chrono;

namespace po = boost::program_options;

inline int idx(int i, int j, int N)
{
    return i * N + j;
}

void initialize_boundaries(vector<double> &grid, int N)
{
    double top_left = 10.0;
    double top_right = 20.0;
    double bottom_right = 30.0;
    double bottom_left = 20.0;

    for (int j = 0; j < N; j++)
    {
        grid[idx(0, j, N)] =
            top_left + (top_right - top_left) * j / (N - 1);
    }

    for (int j = 0; j < N; j++)
    {
        grid[idx(N - 1, j, N)] =
            bottom_left + (bottom_right - bottom_left) * j / (N - 1);
    }

    for (int i = 0; i < N; i++)
    {
        grid[idx(i, 0, N)] =
            top_left + (bottom_left - top_left) * i / (N - 1);
    }

    for (int i = 0; i < N; i++)
    {
        grid[idx(i, N - 1, N)] =
            top_right + (bottom_right - top_right) * i / (N - 1);
    }
}

void save_matrix(const vector<double> &grid,
                 int N,
                 const string &filename)
{
    ofstream out(filename);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            out << grid[idx(i, j, N)] << " ";
        }

        out << "\n";
    }
}

void print_matrix(const vector<double> &grid, int N)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            printf("%8.2f ", grid[idx(i, j, N)]);
        }

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

        desc.add_options()("help,h", "show help")("size",
                                                  po::value<int>(&N)->default_value(128),
                                                  "grid size")("iter",
                                                               po::value<int>(&max_iter)->default_value(1000000),
                                                               "max iterations")("eps",
                                                                                 po::value<double>(&eps)->default_value(1e-6),
                                                                                 "accuracy");

        po::variables_map vm;

        po::store(
            po::parse_command_line(argc, argv, desc),
            vm);

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

    vector<double> grid(N * N, 0.0);
    vector<double> new_grid(N * N, 0.0);

    initialize_boundaries(grid, N);
    initialize_boundaries(new_grid, N);

    int iter = 0;
    double error = 0.0;

    auto start = high_resolution_clock::now();

#pragma acc data copy(grid, new_grid)
    {
        for (iter = 0; iter < max_iter; iter++)
        {
            error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max : error)
            for (int i = 1; i < N - 1; i++)
            {
                for (int j = 1; j < N - 1; j++)
                {
                    new_grid[idx(i, j, N)] =
                        0.25 * (grid[idx(i - 1, j, N)] +
                                grid[idx(i + 1, j, N)] +
                                grid[idx(i, j - 1, N)] +
                                grid[idx(i, j + 1, N)]);

                    double diff =
                        fabs(
                            new_grid[idx(i, j, N)] -
                            grid[idx(i, j, N)]);

                    if (diff > error)
                        error = diff;
                }
            }

            swap(grid, new_grid);

            if (error < eps)
                break;
        }
    }

    auto end = high_resolution_clock::now();

    double elapsed =
        duration_cast<milliseconds>(end - start).count();

    cout << "Iterations: " << iter << endl;
    cout << "Error: " << error << endl;
    cout << "Time: " << elapsed << " ms" << endl;

    save_matrix(grid, N, "result.txt");

    if (N == 10 || N == 13)
    {
        print_matrix(grid, N);
    }

    return 0;
}