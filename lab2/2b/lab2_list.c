// NAME: Miles Kang
// EMAIL: milesjkang@gmail.com
// ID: 405106565

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "SortedList.h"

int n_threads = 1;
int n_iter = 1;
int n_lists = 1;
char opt_sync = 'a';
int opt_yield = 0;
int n_elmts;

pthread_mutex_t* m_locks;
int* s_locks = 0;

long long tot_wait_time = 0;

SortedList_t* lists;
SortedListElement_t* elmts;
int* indices;

void error_msg(char* message, int exit_code) {

    // Generic error msg.

    fprintf(stderr, "Error %d: %s\n", errno, message);
    exit(exit_code);
}

void seg_handler() {
    error_msg("Segmentation fault.", 1);
    exit(2);
}

void initialize_locks() {
    if (opt_sync == 'm') {
        m_locks = malloc(n_lists * sizeof(pthread_mutex_t));
        for (int i = 0; i < n_lists; i++) {
            if (pthread_mutex_init(&m_locks[i], NULL) != 0) {
                error_msg("Mutex lock could not be initialized.", 1);
            }
        }
    }
    else if (opt_sync == 's') {
        s_locks = malloc(n_lists * sizeof(int));
        for (int i = 0; i < n_lists; i++) {
            s_locks[i] = 0;
        }
    }
}

int hash(const char* key) {
	return ((key[0] + key[1]) % n_lists);
}

void* thread_func(void* arg) {
    struct timespec start, end;

    // INSERT.

    int i = *(int *) arg;
    for (; i < n_elmts; i += n_threads) {
        if (opt_sync == 'm') {
            if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            pthread_mutex_lock(&m_locks[indices[i]]);
            if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            tot_wait_time += 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

            SortedList_insert(&lists[indices[i]], &elmts[i]);
            pthread_mutex_unlock(&m_locks[indices[i]]);
        }
        else if (opt_sync == 's') {
            if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            while(__sync_lock_test_and_set(&s_locks[indices[i]], 1));
            if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            tot_wait_time += 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

            SortedList_insert(&lists[indices[i]], &elmts[i]);
            __sync_lock_release(&s_locks[indices[i]]);
        }
        else {
            SortedList_insert(&lists[indices[i]], &elmts[i]);
        }
    }

    // LOOKUP/DELETE.

    SortedListElement_t* tmp = NULL;

    i = *(int *) arg;
    for (; i < n_elmts; i += n_threads) {
        if (opt_sync == 'm') {
            if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            pthread_mutex_lock(&m_locks[indices[i]]);
            if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            tot_wait_time += 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

            tmp = SortedList_lookup(&lists[indices[i]], elmts[i].key);
            if (SortedList_delete(tmp) == 1) {
                error_msg("List is corrupt; element could not be deleted.", 2);
            }
            pthread_mutex_unlock(&m_locks[indices[i]]);
        }
        else if (opt_sync == 's') {
            if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            while(__sync_lock_test_and_set(&s_locks[indices[i]], 1));
            if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
                error_msg("Could not get clock time.", 1);
            }
            tot_wait_time += 1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

            tmp = SortedList_lookup(&lists[indices[i]], elmts[i].key);
            if (SortedList_delete(tmp) == 1) {
                error_msg("List is corrupt; element could not be deleted.", 2);
            }
            __sync_lock_release(&s_locks[indices[i]]);
        }
        else {
            tmp = SortedList_lookup(&lists[indices[i]], elmts[i].key);
            if (SortedList_delete(tmp) == 1) {
                error_msg("List is corrupt; element could not be deleted.", 2);
            }
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {

    // Get arguments.

    const struct option long_options[] = {
        {"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
        {"lists", required_argument, NULL, 'l'},
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
                for (int i = 0; i < (int) strlen(optarg); i++) {
                    if (optarg[i] == 'i') {
                        opt_yield |= INSERT_YIELD;
                    }
                    else if (optarg[i] == 'd') {
                        opt_yield |= DELETE_YIELD;
                    }
                    else if (optarg[i] == 'l') {
                        opt_yield |= LOOKUP_YIELD;
                    }
                    else {
                        error_msg("Unrecognized yield argument was given.", 1);
                    }
                }
                break;
            case 's':
                if (optarg[0] != 's' && optarg[0] != 'm') {
                    error_msg("Unrecognized sync argument was given.", 1);
                }
                opt_sync = optarg[0];
                break;
            case 'l':
                n_lists = atoi(optarg);
                break;
            default:
                error_msg("Unrecognized argument was given.", 1);
        }
    }

    signal(SIGSEGV, seg_handler);

    initialize_locks();

    // Initialize lists to random keys.

    lists = malloc(n_lists * sizeof(SortedList_t));
    for (int i = 0; i < n_lists; i++) {
        lists[i].key = NULL;
        lists[i].prev = &lists[i];
        lists[i].next = &lists[i];
    }

    n_elmts = n_threads * n_iter;
    elmts = malloc(n_elmts * sizeof(SortedListElement_t));

    srand(time(NULL));

    for (int i = 0; i < n_elmts; i++) {
        char* rand_key = malloc(sizeof(char) * 3);
        rand_key[0] = 'a' + rand() % 26;
        rand_key[1] = 'a' + rand() % 26;
        rand_key[2] = '\0';
        elmts[i].key = rand_key;
    }

    // Hash keys.

    indices = malloc(n_elmts * sizeof(int));
    for (int i = 0; i < n_elmts; i++) {
        indices[i] = hash(elmts[i].key);
    }

    // Prepare for threads.

    pthread_t* tid = malloc(n_threads * sizeof(pthread_t));
    int* thread_nums = malloc(n_threads * sizeof(int));

    struct timespec run_start, run_end;

    if (clock_gettime(CLOCK_MONOTONIC, &run_start) < 0) {
        error_msg("Could not get clock time.", 1);
    }

    // Run threads.

    for (int i = 0; i < n_threads; i++) {
        thread_nums[i] = i;
        if (pthread_create(&tid[i], NULL, *thread_func, &thread_nums[i]) != 0) {
            error_msg("Thread could not be created.", 1);
        }
    }

    for (int i = 0; i < n_threads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            error_msg("Threads could not be joined.", 1);
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &run_end) < 0) {
        error_msg("Could not get clock time.", 1);
    }

    // Finished thread work; output results.

    long long tot_run_time = 1000000000L * (run_end.tv_sec - run_start.tv_sec)
        + (run_end.tv_nsec - run_start.tv_nsec);

    if (opt_sync != 's' && opt_sync != 'm') {
        tot_wait_time = 0;
    }

    // CHECK LENGTHS.

    for (int i = 0; i < n_lists; i++) {
        if (SortedList_length(&lists[i]) != 0) {
            error_msg("List length did not end as 0.", 2);
        }
    }

    int n_ops = 3 * n_threads * n_iter;
    long long avg_op_time = tot_run_time / n_ops;
    long long avg_lock_time = tot_wait_time / n_ops;
    char test_name[20] = "list-";
    
    if (!opt_yield) {
        strcat(test_name, "none");
    }
    else {
        if (opt_yield & INSERT_YIELD) {
            strcat(test_name, "i");
        }
        if (opt_yield & DELETE_YIELD) {
            strcat(test_name, "d");
        }
        if (opt_yield & LOOKUP_YIELD) {
            strcat(test_name, "l");
        }
    }

    if (opt_sync == 'm') {
        strcat(test_name, "-m");
    }
    else if (opt_sync == 's') {
        strcat(test_name, "-s");
    }
    else {
        strcat(test_name, "-none");
    }

    // Write data to csv.
    fprintf(stdout, "%s,%d,%d,%d,%d,%lld,%lld,%lld\n", test_name, n_threads,
        n_iter, n_lists, n_ops, tot_run_time, avg_op_time, avg_lock_time);

    free(tid);
    free(elmts);
    free(lists);

    exit(0);
}