#include <iostream>
#include <vector>
#include <cmath>

#ifdef USE_FLOAT
using type = float;
#else
using type = double;
#endif

double f(size_t size)
{
    type total = 0;
    std::vector<type> res(size);
    for (size_t i = 0; i < size; i++)
    {

#ifdef USE_FLOAT
        res[i] = sinf(2 * M_PI * (type)i / (type)size);
#else
        res[i] = sin(2 * M_PI * (type)i / (type)size);
#endif
        total += res[i];
    }
    return total;
}

int main()
{
    std::cout << f(std::pow(10, 7)) << std::endl;
    return 0;
}