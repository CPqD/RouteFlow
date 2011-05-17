/*
 *	Copyright 2011 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *	 you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *	 Unless required by applicable law or agreed to in writing, software
 *	 distributed under the License is distributed on an "AS IS" BASIS,
 *	 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef MUTEX_HH_
#define MUTEX_HH_

#include <pthread.h>


/**
 * Class for mutex handling.
 * This class represents a mutex. It should be used when a source code critical
 * section must be protected, avoiding two or more threads to access the same
 * code simultaneously. This should not be confused with semaphores!
 */
class Mutex {

protected:

	pthread_mutex_t tagLock;
	bool bIsLocked;

public:
	/**
	 * Default constructor
	 * This constructor initializes properly the mutex structure.
	 */
	Mutex();

	/**
	 * Destructor.
	 * This destructor erases and destroys the mutex structure.
	 */
	virtual ~Mutex();

	/**
	 * Locks the mutex.
	 * If successful, the lock() function shall return zero; otherwise, an
	 * error number shall be returned to indicate the error.
	 */
	int32_t lock();

	/**
	 * Unlocks the mutex.
	 * If successful, the unlock() function shall return zero; otherwise, an
	 * error number shall be returned to indicate the error.
	 */
	int32_t unlock();

	bool isLocked();
};

#endif /* MUTEX_HH_ */
