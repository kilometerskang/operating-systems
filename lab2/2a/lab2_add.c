// NAME: Miles Kang
// EMAIL: milesjkang@gmail.com
// ID: 405106565

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>

long long counter = 0;
int n_threads = 1;
int n_iter = 1;
int opt_yield = 0;
char opt_sync = 0;

pthread_mutex_t m_lock;
int s_lock = 0;

void error_msg(char* message, int exit_code) {
    fprintf(stderr, "Error %d: %s\n", errno, message);
    exit(exit_code);
}

void add(long long *pointer, long long value) {
    // Handle sync flags.
    if (opt_sync == 'm') {
        pthread_mutex_lock(&m_lock);
    }
    else if (opt_sync == 's') {
        while(__sync_lock_test_and_set(&s_lock, 1));
    }
    else if (opt_sync == 'c') {
        long long old, new;
        do {
            if (opt_yield) {
                sched_yield();
            }
            old = *pointer;
            new = old + value;
        } while (__sync_val_compare_and_swap(&counter, old, new) != old);
        return;
    }

    // --Original add function--
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }

    *pointer = sum;
    // --Original add function--

    // Handle sync flags.
    if (opt_sync == 'm') {
        pthread_mutex_unlock(&m_lock);
    }
    else if (opt_sync == 's') {
        __sync_lock_release(&s_lock);
    }
}

void* thread_func(void* arg) {
    for (int i = 0; i < n_iter; i++) {
        add(&counter, 1);
    }
    for (int i = 0; i < n_iter; i++) {
        add(&counter, -1);
    }
    return arg;
}

int main(int argc, char* argv[]) {

    // Get arguments.
    const struct option long_options[] = {
        {"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;

        switch(c) {
            case 't':
                n_threads = atoi(optarg);
                break;
            case 'i':
                n_iter = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                if (optarg[0] != 's' && optarg[0] != 'm' && optarg[0] != 'c') {
                    error_msg("Unrecognized sync argument was given.", 1);
                }
                opt_sync = optarg[0];
                if (opt_sync == 'm') {
                    if (pthread_mutex_init(&m_lock, NULL) != 0) {
                        error_msg("Mutex lock could not be initialized.", 1);
                    }
                }
                break;
            default:
                error_msg("Unrecognized argument was given.", 1);
        }
    }

    pthread_t* tid = malloc(n_threads * sizeof(pthread_t));

    long long tot_time = 0;
    struct timespec start, end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        error_msg("Could not get clock time.", 1);
    }

    // Run threads.
    for (int i = 0; i < n_threads; i++) {
        if (pthread_create(&tid[i], NULL, *thread_func, NULL) != 0) {
            error_msg("Thread could not be created.", 1);
        }
    }

    for (int i = 0; i < n_threads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            error_msg("Threads could not be joined.", 1);
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        error_msg("Could not get clock time.", 1);
    }

    // Calculate parameters.
    tot_time = 1000000000 * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

    free(tid);
    int n_ops = 2 * n_threads * n_iter;
    long long avg_time = tot_time / n_ops;
    char test_name[20] = "add";
    
    if (opt_yield) {
        strcat(test_name, "-yield");
    }
    if (opt_sync == 'm') {
        strcat(test_name, "-m");
    }
    else if (opt_sync == 's') {
        strcat(test_name, "-s");
    }
    else if (opt_sync == 'c') {
        strcat(test_name, "-c");
    }
    else {
        strcat(test_name, "-none");
    }

    // Write data to csv.
    fprintf(stdout, "%s,%d,%d,%d,%lld,%lld,%lld\n", test_name, n_threads, n_iter, n_ops, tot_time, avg_time, counter);

    exit(0);
}