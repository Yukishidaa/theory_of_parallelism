#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <omp.h>


double cpu_second()
{
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}

void matrix_vector_product_omp(double *a, double *b, double *c, size_t m, size_t n, int num_of_threads)
{
    // int n_threads = omp_get_max_threads();
    // if(m < n_threads){
    //     n_threads = m; 
    // }
#pragma omp parallel num_threads(num_of_threads)
    {
        //  Threads id
        int thread_id = omp_get_thread_num();
        //  Init start and end pos for each of threads
        int items_per_thread = m / num_of_threads; //  6 / 4 = 1 (bcs int, else 1.5)
        int start_pos = thread_id * items_per_thread; //  0 * 1 = 0, 1 * 1 = 1, 2 * 1 = 2, 3 * 1 = 3
        int end_pos = (thread_id == num_of_threads - 1) ? (m - 1) : (start_pos + items_per_thread - 1);
        //  Calculations
        for (size_t i = start_pos; i <= end_pos; i++)
        {
            c[i] = 0.0;
            for (size_t j = 0; j < n; j++)
                c[i] += a[i * n + j] * b[j];
        }
    }
}

void run_parallel(size_t m, size_t n, int num_of_threads) {
    
    std::vector<double> a(m * n);
    std::vector<double> b(n);
    std::vector<double> c(m);
    // int n_threads = omp_get_max_threads();
    // if(m < n_threads){
    //     n_threads = m;
    // }else{
    //     n_threads = omp_get_max_threads();
    // }
#pragma omp parallel num_threads(num_of_threads)
    {
        int id_thread = omp_get_thread_num();
        int items_per_thread = m / num_of_threads;
        int start_pos = id_thread * items_per_thread;
        int end_pos = (id_thread == num_of_threads - 1) ? (m - 1) : (start_pos + items_per_thread - 1);
        // Initialization matrix
        for (size_t i = start_pos; i <= end_pos; i++) {
            for (size_t j = 0; j < n; j++)
            a[i * n + j] = i + j;
        }   

        items_per_thread = n / num_of_threads;
        start_pos = id_thread * items_per_thread;
        end_pos = (id_thread == num_of_threads - 1) ? (n - 1) : (start_pos + items_per_thread - 1);
        // Initialization vector
        for (size_t j = start_pos; j <= end_pos; j++)
            b[j] = j;

    }

    // Вычисление
    double t = cpu_second();
    matrix_vector_product_omp(a.data(), b.data(), c.data(), m, n, num_of_threads);
    t = cpu_second() - t;

    std::cout << t << std::endl;
}

int main(int argc, char *argv[])
{
    // size_t M = 20000;
    // size_t N = 20000;

    // if (argc > 1)
    //     M = atoi(argv[1]);
    // if (argc > 2)
    //     N = atoi(argv[2]);
    std::vector<size_t> size_matrix = {20000, 40000};
    std::vector<int> count_threads = {1, 2, 4, 7, 8, 16, 20, 40};
    
    for(size_t size : size_matrix){
        for(int threads : count_threads){
            run_parallel(size, size, threads);
        }
    }
    
    return 0;
}
