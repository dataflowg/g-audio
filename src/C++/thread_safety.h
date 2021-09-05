#ifndef _THREAD_SAFETY_
#define _THREAD_SAFETY_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unordered_map>
#include <vector>
// Need to #define THREAD_IMPLEMENTATION before including thread.h
// Don't do it in this header, as it will cause linkage issues
// Instead define it in thread_safety.cpp
#include "thread.h"
#include "miniaudio.h"

typedef struct ga_refnum
{
	std::unordered_map<int32_t, void*> refnums;
	thread_mutex_t refnums_mutex = { 0 };
	int32_t refnum_counter = 0;
} ga_refnum;

typedef enum
{
	ga_refnum_audio_file = 0,
	ga_refnum_audio_device,
	ga_refnum_count
} ga_refnum_type;

typedef enum
{
	ga_mutex_common = 0,
	ga_mutex_context,
	ga_mutex_device,
	ga_mutex_count
} ga_mutex_type;

static inline void create_refnums_mutex(ga_refnum_type refnum_type);
static void lock_refnums_mutex(ga_refnum_type refnum_type);
static void unlock_refnums_mutex(ga_refnum_type refnum_type);

inline void create_ga_mutex(ga_mutex_type mutex_type);
void lock_ga_mutex(ga_mutex_type mutex_type);
void unlock_ga_mutex(ga_mutex_type mutex_type);

// Create and insert a new unique reference to the global references
int32_t create_insert_refnum_data(ga_refnum_type refnum_type, void* data);
void update_reference_data(ga_refnum_type refnum_type, int32_t reference, void* data);
void* get_reference_data(ga_refnum_type refnum_type, int32_t reference);
void* remove_reference(ga_refnum_type refnum_type, int32_t reference);
std::vector<int32_t> get_all_references(ga_refnum_type refnum_type);

#endif
