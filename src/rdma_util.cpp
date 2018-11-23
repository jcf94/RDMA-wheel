/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_UTIL_CPP
************************************************ */

#include "rdma_util.h"

#include <stdexcept>

std::string make_string(const char *fmt, ...)
{
    char * sz;
    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&sz, fmt, ap) == -1)
        throw std::runtime_error("memory allocation failed\n");
    va_end(ap);
    std::string str(sz);
    free(sz);

    return str;
}

std::mt19937_64* InitRng()
{
    std::random_device device("/dev/urandom");
    return new std::mt19937_64(device());
}

long long New64()
{
    static std::mt19937_64* rng = InitRng();
    // static mutex mu;
    // mutex_lock l(mu);
    return (*rng)();
}