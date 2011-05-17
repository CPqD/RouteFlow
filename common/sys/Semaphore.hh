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

#ifndef SEMAPHORE_HH_
#define SEMAPHORE_HH_

#include <sys/types.h>
#include <stdint.h>
#include <semaphore.h>
#include "Mutex.hh"
#include "Thread.hh"

/**
 * This value is passed as an argument for sem_init function.
 */
#define SEMAPHORE_INITIAL_VALUE 0

/**
 * Default timeout taken by Semaphore class
 */
#define SEMAPHORE_DEFAULT_TIMEOUT 5

/**
 * Class for semaphores handling.
 * Using this class is as simple as the Internet: when creating a new semaphore,
 * call its init() method. Afterwards, call wait() if you want to... well...
 * wait for other thread, which will call the post() method.
 */
class Semaphore : public Thread{

protected:

	pthread_t tagTimeoutThread; ///< Timeout counter thread.
	sem_t tagSemaphore; ///< Semaphore structure
	uint32_t ulTimeout; ///< Timeout of this semaphore.

	/**
	 * Timeout
	 */
//	static void * countTimeout(void * data);
	int32_t run();

	/**
	 * Timeout flag.
	 */
	bool bIsTimeoutEnabled;

	int32_t lBlocked;

	Mutex mtx;

	int32_t miID;
	uint32_t ulTimeoutID;


public:

	/**
	 * Constructor
	 * This constructor just clears every data structure of this class.
	 */
	Semaphore();

	virtual ~Semaphore();

	/**
	 * Inits this semaphore properly.
	 */
	int32_t init();

	/**
	 * Releases other threads blocked at wait() method.
	 * The decision of which of these threads will be unblocked is taken using
	 * a priority queue approach.
	 * @return 0 If everything is OK, -1 (and errno) if something is wrong. Call
	 * ErrorHandler::dumpError for more details.
	 */
	int32_t post();

	/**
	 * Blocks a thread.
	 * This method will block the caller thread until post() is called.
	 * @return 0 If everything is OK, -1 (and errno) if something is wrong. Call
	 * ErrorHandler::dumpError for more details.
	 */
	int32_t wait();

	/**
	 * Enables a timeout for this semaphore.
	 * Calling this method will make this semaphore auto-unblocking, i.e., after
	 * some time the method post() will automatically be called. After
	 * enableTimeout() is called, it can be canceled by executing disableTimeout()
	 * method.
	 * @param timeout Time (in milisseconds) to wait before post() is called.
	 */
	void enableTimeout(uint32_t timeout);

	/**
	 * Disables the current timeout value previously configured.
	 * If no timeout is set, this method will do nothing.
	 */
	void disableTimeout();

	bool isBlocked() { return lBlocked > 0; };


	int32_t ret;
};

#endif /* SEMAPHORE_HH_ */
