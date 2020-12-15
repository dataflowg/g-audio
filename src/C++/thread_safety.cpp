#include "stdafx.h"
#define THREAD_IMPLEMENTATION
#include "thread_safety.h"

// These variables will be kept in memory while LabVIEW has DLL loaded.
// LabVIEW won't unload DLL until entire VI call chain is removed from memory, or explicitly removed by wiring empty path to CLFN.
std::unordered_map<int32_t, void*> refnums;
thread_mutex_t refnums_mutex = { 0 };
int32_t refnum_counter = 0;

// Create the refnums mutex if it doesn't already exist.
// If it does exist, do nothing.
// Could maybe use the CLFN's reserve callback to call this rather than every lock / unlock
static inline void create_refnums_mutex()
{
	// Init the mutex if it doesn't exist
	if (strlen(refnums_mutex.data) == 0)
	{
		thread_mutex_init(&refnums_mutex);
	}
}

// Lock the refnums mutex. Will attempt to create the mutex first in case it doesn't exist.
static void lock_refnums_mutex()
{
	create_refnums_mutex();
	thread_mutex_lock(&refnums_mutex);
	return;
}

// Unlock the refnums mutex. Will attempt to create the mutex first in case it doesn't exist.
static void unlock_refnums_mutex()
{
	create_refnums_mutex();
	thread_mutex_unlock(&refnums_mutex);
	return;
}

// Creates a new refnum and stores it and its data into the allocated refnums, returning the new refnum value.
// If the refnum allocation is exhausted, returns -1.
int32_t create_insert_refnum_data(void* data)
{
	int32_t new_reference = 0;

	lock_refnums_mutex();
	//// START CRITICAL SECTION ////
	do
	{
		// update reference id
		refnum_counter++;
		// Check for roll over of reference counter.
		if (refnum_counter < 0)
		{
			refnum_counter = 0;
		}
		new_reference = refnum_counter;
		if (!refnums.count(new_reference))
		{
			refnums[new_reference] = data;
			break;
		}
		// Check if maximum reference allocation has been reached.
		else if (refnums.size() >= INT_MAX)
		{
			new_reference = -1;
			break;
		}
	} while (1);
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex();

	return new_reference;
}

// Updates the data in the refnum allocation for the given refnum.
void update_reference_data(int32_t reference, void * data)
{
	lock_refnums_mutex();
	//// START CRITICAL SECTION ////
	if (refnums.count(reference))
	{
		refnums[reference] = data;
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex();
}

// Retrieves the data in the refnum allocation for the given refnum.
void* get_reference_data(int32_t reference)
{
	void* data = NULL;

	lock_refnums_mutex();
	//// START CRITICAL SECTION ////
	if (refnums.count(reference))
	{
		data = refnums[reference];
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex();

	return data;
}

// Removes the refnum from the refnum allocation, returning its data.
// The caller is responsible for freeing the data.
void* remove_reference(int32_t reference)
{
	void* data = NULL;

	lock_refnums_mutex();
	//// START CRITICAL SECTION ////
	if (refnums.count(reference))
	{
		data = refnums[reference];
		refnums.erase(reference);
	}
	//// END CRITICAL SECTION ////
	unlock_refnums_mutex();

	return data;
}
