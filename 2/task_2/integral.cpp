#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>
#include "omp.h"


const double PI = 3.14159265358979323846;
const double a = -4.0;
const double b = 4.0;
const int nsteps = 40000000;

double cpu_second()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}

double func(double x)
{
    return exp(-x * x);
}

double integrate_omp(double (*func)(double), double a, double b, int n, int num_threads)
{
    double h = (b - a) / n;
    double sum = 0.0;

#pragma omp parallel num_threads(num_threads)
    {
        double local_sum = 0.0;
        int thread_id = omp_get_thread_num();
        int items_per_thread = n / num_threads;
        int start_pos = thread_id * items_per_thread;
        int end_pos = (thread_id == num_threads - 1) ? (n - 1) : (start_pos + items_per_thread - 1);

        for (int i = start_pos; i <= end_pos; i++)
            local_sum += func(a + h * (i + 0.5));
        
        #pragma omp atomic
        sum += local_sum;
    }

    return sum*h;
}

double run_parallel(int num_threads)
{
    double t = cpu_second();
    double res = integrate_omp(func, a, b, nsteps, num_threads);
    t = cpu_second() - t;
    return t;
}

int main(int argc, char **argv)
{
    std::vector<int> count_threads = {1, 2, 4, 7, 8, 16, 20, 40};

    for(int threads : count_threads){
        double time_parallel = run_parallel(threads);
        std::cout << time_parallel << std::endl;
    }
    return 0;
}