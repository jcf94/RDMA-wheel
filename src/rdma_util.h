/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_UTIL_H
************************************************ */

#ifndef RDMA_UTIL_H
#define RDMA_UTIL_H

#include <stdint.h>
#include <stdarg.h>

#include <iostream>
#include <string>
#include <random>

//#define DEV_MODE

// ---- WHEEL
#ifdef _WIN32
#define RESET       ""
#define BLACK       ""
#define RED         ""
#define GREEN       ""
#define YELLOW      ""
#define BLUE        ""
#define MAGENTA     ""
#define CYAN        ""
#define WHITE       ""
#define BOLD_BLACK   "\033[1m\033[30m"
#define BOLD_RED     "\033[1m\033[31m"
#define BOLD_GREEN   "\033[1m\033[32m"
#define BOLD_YELLOW  "\033[1m\033[33m"
#define BOLD_BLUE    "\033[1m\033[34m"
#define BOLD_MAGENTA "\033[1m\033[35m"
#define BOLD_CYAN    "\033[1m\033[36m"
#define BOLD_WHITE   "\033[1m\033[37m"
#else
#define RESET       "\033[0m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define BOLD_BLACK   "\033[1m\033[30m"
#define BOLD_RED     "\033[1m\033[31m"
#define BOLD_GREEN   "\033[1m\033[32m"
#define BOLD_YELLOW  "\033[1m\033[33m"
#define BOLD_BLUE    "\033[1m\033[34m"
#define BOLD_MAGENTA "\033[1m\033[35m"
#define BOLD_CYAN    "\033[1m\033[36m"
#define BOLD_WHITE   "\033[1m\033[37m"
#endif // _WIN32

#define DIM(a) (sizeof(a)/sizeof(a[0]))

#define log_error(x) do{std::cerr << RED << "[Error] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#define log_warning(x) do{std::cerr << YELLOW << "[Warning] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#define log_ok(x) do{std::cerr << GREEN << "[Ok] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)

#ifdef DEV_MODE
#define VAL(x) do{std::cout << #x << " = " << (x) << std::endl;}while(false)
#define VAL_ARRAY(a) do{std::cout << #a << " = ["; std::for_each(a, a + DIM(a), [](int val__){std::cout << " " << val__ << " ";}); std::cout << "]\n";}while(false)
#define log_func() do{std::cerr << CYAN << "[Function] " << __PRETTY_FUNCTION__ << RESET << std::endl;}while(false)
#define log_info(x) do{std::cerr << CYAN << "[Info] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#else
#define VAL(x)
#define VAL_ARRAY(a)
#define log_func()
#define log_info(x)
#endif // DEV_MODE

#define ADDR(x) ((void**)&(x))

std::string make_string(const char *fmt, ...);
// ---- WHEEL

// ---- UTILS
std::mt19937_64* InitRng();
long long New64();

#define __bswap_32_bit(x) \
    ((((x) & 0xff000000) >> 24)     \
    | (((x) & 0x00ff0000) >>  8)    \
    | (((x) & 0x0000ff00) <<  8)    \
    | (((x) & 0x000000ff) << 24))

#define __bswap_64_bit(x) \
    ((((x) & 0xff00000000000000ull) >> 56)  \
    | (((x) & 0x00ff000000000000ull) >> 40) \
    | (((x) & 0x0000ff0000000000ull) >> 24) \
    | (((x) & 0x000000ff00000000ull) >> 8)  \
    | (((x) & 0x00000000ff000000ull) << 8)  \
    | (((x) & 0x0000000000ff0000ull) << 24) \
    | (((x) & 0x000000000000ff00ull) << 40) \
    | (((x) & 0x00000000000000ffull) << 56))

namespace rdma_util {

#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t htonll(uint64_t x) { return __bswap_64_bit(x); }
static inline uint64_t ntohll(uint64_t x) { return __bswap_64_bit(x); }
static inline uint32_t htonl(uint32_t x) { return __bswap_32_bit(x); }
static inline uint32_t ntohl(uint32_t x) { return __bswap_32_bit(x); }
#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t htonll(uint64_t x) { return x; }
static inline uint64_t ntohll(uint64_t x) { return x; }
static inline uint32_t htonl(uint32_t x) { return x; }
static inline uint32_t ntohl(uint32_t x) { return x; }
#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif

}
// ---- UTILS

// ---- RDMA

// structure to exchange data which is needed to connect the QPs
struct cm_con_data_t {
    uint64_t    maddr;      // Buffer address
    uint32_t    mrkey;      // Remote key
    uint32_t    qpn;        // QP number
    uint32_t    lid;        // LID of the IB port
    uint32_t    psn;
} __attribute__ ((packed));

// ---- RDMA

#endif // !RDMA_UTIL_H