#ifndef UDP_CONF_H
#define UDP_CONF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdint.h>

#define SHMSZ     1
#define PORT    5001
#define MAXLINE 4096
#define LATENCY_SIZE 1000
#define NSECS_PER_SEC 1000000000.0
#define RATE_NUM 505
#define USEC_PER_SEC 1000000L
#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_USEC 1000L

#define STAT_INTERVAL 100.0

#define TIMERSUB(a, b, result)                                                \
    do {                                                                      \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                         \
        (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                      \
        if ((result)->tv_nsec < 0) {                                          \
            --(result)->tv_sec;                                               \
            (result)->tv_nsec += 1000000000;                                     \
        }                                                                     \
    } while (0)

#define ts_sub(a, b, result)                                                                       \
    do {                                                                                           \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                                              \
        (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                                           \
        if ((result)->tv_nsec < 0) {                                                               \
            --(result)->tv_sec;                                                                    \
            (result)->tv_nsec += NSEC_PER_SEC;                                                     \
        }                                                                                          \
    } while (0)


struct thread_data
{
  int thread_id;
  int process_id;
  int latency_measure;
  double T;
  int app_id;
  int pkt_size;
  char dst_IP[20];
  int trace;
  float target_rate;
  uint64_t tsc_frequency;
  int app_arg_1;
  int app_arg_2;
  int app_arg_3;
};

int cmpfunc (const void * a, const void * b) {
    if (*(double*)a > *(double*)b) return 1;
    else if (*(double*)a < *(double*)b) return -1;
    else return 0;
}

int cmpfunc1 (const void * a, const void * b) {
    if (*(int*)a > *(int*)b) return 1;
    else if (*(int*)a < *(int*)b) return -1;
    else return 0;
}


double next_poisson_time (double packet_rate, uint64_t tsc_frequency) {
    return -logf(1.0f - ((double)random()) / (double)(RAND_MAX)) / (packet_rate / tsc_frequency);
}

inline uint64_t tsc_counter()
{
#if defined(__i386__) || defined(__x86_64__)
        uint32_t lo, hi;
        asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
        uint64_t ticks;
        asm volatile("mrs %0, CNTVCT_EL0" : "=r" (ticks));
        return ticks;
#else
#error "TscClock is not supported on this architecture!"
#endif
}

inline void os_ts_gettimeofclock(struct timespec *pts) {
#ifdef __windows__
    ticks_t val = os_gettimeoftsc(); // probably just NSEC_IN_SEC
    pts->tv_sec = val / NSEC_IN_SEC;
    pts->tv_nsec = val % NSEC_IN_SEC;
#else

    if (clock_gettime(CLOCK_MONOTONIC, pts)) {
        printf("clock_gettime failed\n");
    }
#endif
}

uint64_t get_tsc_rate_per_second() {
    static uint64_t tsc_per_second = 0;
    if (!tsc_per_second) {
        uint64_t delta_usec;
        struct timespec ts_before, ts_after, ts_delta;
        uint64_t tsc_before, tsc_after, tsc_delta;

        // Measure the time actually slept because usleep() is very inaccurate.
        os_ts_gettimeofclock(&ts_before);
        tsc_before = tsc_counter();
        usleep(100000); // 0.1 sec
        os_ts_gettimeofclock(&ts_after);
        tsc_after = tsc_counter();

        // Calc delta's
        tsc_delta = tsc_after - tsc_before;
        ts_sub(&ts_after, &ts_before, &ts_delta);
        delta_usec = ts_delta.tv_sec * USEC_PER_SEC + ts_delta.tv_nsec / NSEC_PER_USEC;

        // Calc rate
        tsc_per_second = tsc_delta * USEC_PER_SEC / delta_usec;
    }
    return tsc_per_second;
}


#endif /* UDP_CONF_H */

