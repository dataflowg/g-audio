#ifndef _THREAD_SAFETY_
#define _THREAD_SAFETY_

#include <stdint.h>
#include <stdlib.h>
#include <unordered_map>
// Need to #define THREAD_IMPLEMENTATION before including thread.h
// Don't do it in this header, as it will cause linkage issues
// Instead define it in thread_safety.cpp
#include "thread.h"

static inline void create_refnums_mutex();
static void lock_refnums_mutex();
static void unlock_refnums_mutex();

// Create and insert a new unique reference to the global references
int32_t create_insert_refnum_data(void* data);
void update_reference_data(int32_t reference, void* data);
void* get_reference_data(int32_t reference);
void* remove_reference(int32_t reference);

#endif
