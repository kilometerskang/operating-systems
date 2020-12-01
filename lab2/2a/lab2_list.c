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
char opt_sync = 0;
int opt_yield = 0;
int n_elmts;

pthread_mutex_t m_lock;
int s_lock = 0;

SortedList_t* list;
SortedListElement_t* elmt;

void error_msg(char* message, int exit_code) {
    fprintf(stderr, "Error %d: %s\n", errno, message);
    exit(exit_code);
}

void seg_handler() {
    error_msg("Segmentation fault.", 1);
    free(list);
    exit(2);
}

void* thread_func(void* arg) {
    // Insert.
    int i = *(int *) arg;
    for (; i < n_elmts; i += n_threads) {
        if (opt_sync == 'm') {
            pthread_mutex_lock(&m_lock);
            SortedList_insert(list, &elmt[i]);
            pthread_mutex_unlock(&m_lock);
        }
        else if (opt_sync == 's') {
            while(__sync_lock_test_and_set(&s_lock, 1));
            SortedList_insert(list, &elmt[i]);
            __sync_lock_release(&s_lock);
        }
        else {
            SortedList_insert(list, &elmt[i]);
        }
    }

    // Check length.
    if (opt_sync == 'm') {
        pthread_mutex_lock(&m_lock);
        if (SortedList_length(list) < 0) {
            error_msg("List is corrupt; length could not be retrieved.", 2);
        }
        pthread_mutex_unlock(&m_lock);
    }
    else if (opt_sync == 's') {
        while(__sync_lock_test_and_set(&s_lock, 1));
        if (SortedList_length(list) < 0) {
            error_msg("List is corrupt; length could not be retrieved.", 2);
        }
        __sync_lock_release(&s_lock);
    }
    else {
        if (SortedList_length(list) < 0) {
            error_msg("List is corrupt; length could not be retrieved.", 2);
        }
    }

    // Lookup/delete.
    SortedListElement_t* tmp = NULL;

    i = *(int *) arg;
    for (; i < n_elmts; i += n_threads) {
        if (opt_sync == 'm') {
            pthread_mutex_lock(&m_lock);
            tmp = SortedList_lookup(list, elmt[i].key);
            if (SortedList_delete(tmp) == 1) {
                error_msg("List is corrupt; element could not be deleted.", 2);
            }
            pthread_mutex_unlock(&m_lock);
        }
        else if (opt_sync == 's') {
            while(__sync_lock_test_and_set(&s_lock, 1));
            tmp = SortedList_lookup(list, elmt[i].key);
            if (SortedList_delete(tmp) == 1) {
                error_msg("List is corrupt; element could not be deleted.", 2);
            }
            __sync_lock_release(&s_lock);
        }
        else {
            tmp = SortedList_lookup(list, elmt[i].key);
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

    signal(SIGSEGV, seg_handler);

    // Initialize list to random keys.
    list = malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->prev = list;
    list->next = list;

    n_elmts = n_threads * n_iter;

    elmt = malloc(n_elmts * sizeof(SortedListElement_t));

    srand(time(NULL));

    for (int i = 0; i < n_elmts; i++) {
        char* rand_key = malloc(sizeof(char) * 2);
        rand_key[0] = 'a' + rand() % 26;
        rand_key[1] = '\0';
        elmt[i].key = rand_key;
    }

    pthread_t* tid = malloc(n_threads * sizeof(pthread_t));
    int* thread_nums = malloc(n_threads * sizeof(int));

    long long tot_time = 0;
    struct timespec start, end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        error_msg("Could not get clock time.", 1);
    }

    // Run threads.
    for (int i = 0; i < n_threads; i++) {
        thread_nums[i] = i;
        if (pthread_create(&tid[i], NULL, *thread_func, &thread_nums[i]) != 0) {
            free(elmt);
            free(list);
            error_msg("Thread could not be created.", 1);
        }
    }

    for (int i = 0; i < n_threads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            free(elmt);
            free(list);
            error_msg("Threads could not be joined.", 1);
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        free(elmt);
        free(list);
        error_msg("Could not get clock time.", 1);
    }

    // Calculate parameters.
    tot_time = 1000000000 * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

    int n_ops = 3 * n_threads * n_iter;
    long long avg_time = tot_time / n_ops;
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
    int n_lists = 1;

    // Write data to csv.
    fprintf(stdout, "%s,%d,%d,%d,%d,%lld,%lld\n", test_name, n_threads, n_iter, n_lists, n_ops, tot_time, avg_time);

    free(tid);
    free(elmt);
    free(list);

    exit(0);
}