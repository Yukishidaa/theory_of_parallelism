#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <boost/program_options.hpp>

using namespace std;
namespace po = boost::program_options;
using namespace std::chrono;

void initialize_boundaries(vector<vector<double>> &grid, int N)
{
    double top_left = 10.0;
    double top_right = 20.0;
    double bottom_right = 30.0;
    double bottom_left = 20.0;

    for (int j = 0; j < N; j++)
        grid[0][j] = top_left + (top_right - top_left) * j / (N - 1);

    for (int j = 0; j < N; j++)
        grid[N - 1][j] = bottom_left + (bottom_right - bottom_left) * j / (N - 1);

    for (int i = 0; i < N; i++)
        grid[i][0] = top_left + (bottom_left - top_left) * i / (N - 1);

    for (int i = 0; i < N; i++)
        grid[i][N - 1] = top_right + (bottom_right - top_right) * i / (N - 1);
}

void save_matrix(const vector<vector<double>> &grid, const string &filename)
{
    ofstream out(filename);

    for (const auto &row : grid)
    {
        for (double val : row)
            out << val << " ";
        out << "\n";
    }
}

void print_matrix(const vector<vector<double>> &grid)
{
    for (const auto &row : grid)
    {
        for (double val : row)
            printf("%8.2f ", val);
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

        desc.add_options()("help,h", "show help")("size", po::value<int>(&N)->default_value(128),
                                                  "grid size")("iter", po::value<int>(&max_iter)->default_value(1000000),
                                                               "max iterations")("eps", po::value<double>(&eps)->default_value(1e-6),
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

    vector<vector<double>> grid(N, vector<double>(N, 0.0));
    vector<vector<double>> new_grid(N, vector<double>(N, 0.0));

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
                    new_grid[i][j] = 0.25 * (grid[i - 1][j] +
                                             grid[i + 1][j] +
                                             grid[i][j - 1] +
                                             grid[i][j + 1]);

                    double diff = fabs(new_grid[i][j] - grid[i][j]);

                    if (diff > error)
                        error = diff;
                }
            }

#pragma acc parallel loop collapse(2)
            for (int i = 1; i < N - 1; i++)
            {
                for (int j = 1; j < N - 1; j++)
                    grid[i][j] = new_grid[i][j];
            }

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

    save_matrix(grid, "result.txt");

    if (N == 10 || N == 13)
        print_matrix(grid);

    return 0;
}