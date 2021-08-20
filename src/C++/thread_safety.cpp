#include "stdafx.h"
#define THREAD_IMPLEMENTATION
#include "thread_safety.h"

// These variables will be kept in memory while LabVIEW has DLL loaded.
// LabVIEW won't unload DLL until entire VI call chain is removed from memory, or explicitly removed by wiring empty path to CLFN.

ga_refnum refnums[ga_refnum_count];
thread_mutex_t ga_mutexes[ga_mutex_count] = { 0 };

// Create the refnums mutex if it doesn't already exist.
// If it does exist, do nothing.
// Could maybe use the CLFN's reserve callback to call this rather than every lock / unlock
static inline void create_refnums_mutex(ga_refnum_type refnum_type)
{
	// Init the mutex if it doesn't exist
	if (strlen(refnums[refnum_type].refnums_mutex.data) == 0)
	{
		thread_mutex_init(&(refnums[refnum_type].refnums_mutex));
	}
}

// Lock the refnums mutex. Will attempt to create the mutex first in case it doesn't exist.
static void lock_refnums_mutex(ga_refnum_type refnum_type)
{
	create_refnums_mutex(refnum_type);
	thread_mutex_lock(&(refnums[refnum_type].refnums_mutex));
	return;
}

// Unlock the refnums mutex. Will attempt to create the mutex first in case it doesn't exist.
static void unlock_refnums_mutex(ga_refnum_type refnum_type)
{
	create_refnums_mutex(refnum_type);
	thread_mutex_unlock(&(refnums[refnum_type].refnums_mutex));
	return;
}

// Creates a new refnum and stores it and its data into the allocated refnums, returning the new refnum value.
// If the refnum allocation is exhausted, returns -1.
int32_t create_insert_refnum_data(ga_refnum_type refnum_type, void* data)
{
	int32_t new_reference = 0;

	lock_refnums_mutex(refnum_type);
	//// START CRITICAL SECTION ////
	do
	{
		// update reference id
		refnums[refnum_type].refnum_counter++;
		// Check for roll over of reference counter.
		if (refnums[refnum_type].refnum_counter < 0)
		{
			refnums[refnum_type].refnum_counter = 0;
		}
		new_reference = refnums[refnum_type].refnum_counter;
		if (!refnums[refnum_type].refnums.count(new_reference))
		{
			refnums[refnum_type].refnums[new_reference] = data;
			break;
		}
		// Check if maximum reference allocation has been reached.
		else if (refnums[refnum_type].refnums.size() >= INT_MAX)
		{
			new_reference = -1;
			break;
		}
	} while (1);
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex(refnum_type);

	return new_reference;
}

// Updates the data in the refnum allocation for the given refnum.
void update_reference_data(ga_refnum_type refnum_type, int32_t reference, void * data)
{
	lock_refnums_mutex(refnum_type);
	//// START CRITICAL SECTION ////
	if (refnums[refnum_type].refnums.count(reference))
	{
		refnums[refnum_type].refnums[reference] = data;
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex(refnum_type);
}

// Retrieves the data in the refnum allocation for the given refnum.
void* get_reference_data(ga_refnum_type refnum_type, int32_t reference)
{
	void* data = NULL;

	lock_refnums_mutex(refnum_type);
	//// START CRITICAL SECTION ////
	if (refnums[refnum_type].refnums.count(reference))
	{
		data = refnums[refnum_type].refnums[reference];
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex(refnum_type);

	return data;
}

// Removes the refnum from the refnum allocation, returning its data.
// The caller is responsible for freeing the data.
void* remove_reference(ga_refnum_type refnum_type, int32_t reference)
{
	void* data = NULL;

	lock_refnums_mutex(refnum_type);
	//// START CRITICAL SECTION ////
	if (refnums[refnum_type].refnums.count(reference))
	{
		data = refnums[refnum_type].refnums[reference];
		refnums[refnum_type].refnums.erase(reference);
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex(refnum_type);

	return data;
}

std::vector<int32_t> get_all_references(ga_refnum_type refnum_type)
{
	std::vector<int32_t> all_refnums;

	lock_refnums_mutex(refnum_type);
	//// START CRITICAL SECTION ////
	for (std::unordered_map<int32_t, void*>::iterator it = refnums[refnum_type].refnums.begin(); it != refnums[refnum_type].refnums.end(); ++it)
	{
		all_refnums.push_back(it->first);
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex(refnum_type);

	return all_refnums;
}



// Create the context mutex if it doesn't already exist.
// If it does exist, do nothing.
// Could maybe use the CLFN's reserve callback to call this rather than every lock / unlock
inline void create_ga_mutex(ga_mutex_type mutex_type)
{
	// Init the mutex if it doesn't exist
	if (strlen(ga_mutexes[mutex_type].data) == 0)
	{
		thread_mutex_init(&ga_mutexes[mutex_type]);
	}
}

// Lock the context mutex. Will attempt to create the mutex first in case it doesn't exist.
void lock_ga_mutex(ga_mutex_type mutex_type)
{
	create_ga_mutex(mutex_type);
	thread_mutex_lock(&ga_mutexes[mutex_type]);
	return;
}

// Unlock the context mutex. Will attempt to create the mutex first in case it doesn't exist.
void unlock_ga_mutex(ga_mutex_type mutex_type)
{
	create_ga_mutex(mutex_type);
	thread_mutex_unlock(&ga_mutexes[mutex_type]);
	return;
}
