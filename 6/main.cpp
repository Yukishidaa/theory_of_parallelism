#include <iostream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <chrono>

int main(int argc, char *argv[])
{
    int N = 128;
    double eps = 1e-6;
    int max_iter = 1000000;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc)
        {
            N = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--eps") == 0 && i + 1 < argc)
        {
            eps = atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--max_iter") == 0 && i + 1 < argc)
        {
            max_iter = atoi(argv[++i]);
        }
    }

    int size = N + 2;

    double **u = new double *[size];
    double **un = new double *[size];
    for (int i = 0; i < size; i++)
    {
        u[i] = new double[size];
        un[i] = new double[size];
        for (int j = 0; j < size; j++)
        {
            u[i][j] = 0.0;
            un[i][j] = 0.0;
        }
    }

    u[0][0] = 10.0;
    u[0][size - 1] = 20.0;
    u[size - 1][0] = 30.0;
    u[size - 1][size - 1] = 20.0;

    for (int j = 1; j < size - 1; j++)
    {
        double t = (double)j / (size - 1);
        u[0][j] = 10.0 + (20.0 - 10.0) * t;
        u[size - 1][j] = 30.0 + (20.0 - 30.0) * t;
    }

    for (int i = 1; i < size - 1; i++)
    {
        double t = (double)i / (size - 1);
        u[i][0] = 10.0 + (30.0 - 10.0) * t;
        u[i][size - 1] = 20.0;
    }

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            un[i][j] = u[i][j];
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    int iter;
    double error;

#pragma acc data copy(u[0 : size][0 : size]) create(un[0 : size][0 : size])
    {
        for (iter = 0; iter < max_iter; iter++)
        {
            error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max : error)
            for (int i = 1; i < size - 1; i++)
            {
                for (int j = 1; j < size - 1; j++)
                {
                    un[i][j] = (u[i - 1][j] + u[i + 1][j] + u[i][j - 1] + u[i][j + 1]) / 4.0;
                    double diff = fabs(un[i][j] - u[i][j]);
                    if (diff > error)
                        error = diff;
                }
            }

#pragma acc parallel loop collapse(2)
            for (int i = 1; i < size - 1; i++)
            {
                for (int j = 1; j < size - 1; j++)
                {
                    u[i][j] = un[i][j];
                }
            }

            if (error < eps)
                break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Размер: " << N << "x" << N << std::endl;
    std::cout << "Время: " << duration.count() << " мс" << std::endl;
    std::cout << "Итерации: " << iter << std::endl;
    std::cout << "Ошибка: " << error << std::endl;

    for (int i = 0; i < size; i++)
    {
        delete[] u[i];
        delete[] un[i];
    }
    delete[] u;
    delete[] un;

    return 0;
}