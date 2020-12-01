// NAME: Miles Kang
// EMAIL: milesjkang@gmail.com
// ID: 405106565

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    if (list == NULL || element == NULL || list->key != NULL) {
        return;
    }

    SortedListElement_t* curr = list->next;

    while (curr->key != NULL && strcmp(element->key, curr->key) > 0) {
        curr = curr->next;
    }

    if (opt_yield & INSERT_YIELD) {
        sched_yield();
    }

    element->prev = curr->prev;
    element->next = curr;

    curr->prev->next = element;
    curr->prev = element;
}

int SortedList_delete(SortedListElement_t *element) {
    if (element == NULL || element->key == NULL) {
        return 1;
    }

    if (element->prev->next != element || element->next->prev != element) {
        return 1;
    }

    if (opt_yield & DELETE_YIELD) {
        sched_yield();
    }

    element->prev->next = element->next;
    element->next->prev = element->prev;

    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    if (list == NULL || list->key != NULL || key == NULL) {
        return NULL;
    }

    SortedListElement_t* curr = list->next;

    while (curr->key != NULL) {
        if (strcmp(key, curr->key) == 0) {
            return curr;
        }
        if (opt_yield & LOOKUP_YIELD) {
            sched_yield();
        }
        curr = curr->next;
    }

    return NULL;
}

int SortedList_length(SortedList_t *list) {
    if (list == NULL || list->key != NULL) {
        return -1;
    }
    if (list->next->prev != list || list->prev->next != list) {
        return -1;
    }

    int count = 0;
    SortedListElement_t* curr = list->next;

    while (curr->key != NULL) {
        if (curr->next->prev != curr) {
            return -1;
        }
        if (opt_yield & LOOKUP_YIELD) {
            sched_yield();
        }
        curr = curr->next;
        count++;
    }

    return count;
}