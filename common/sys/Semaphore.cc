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


#include "Semaphore.hh"
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

using std::memset;


Semaphore::Semaphore() {
	memset(&this->tagSemaphore, 0, sizeof(sem_t));
	memset(&this->tagTimeoutThread, 0, sizeof(pthread_t));
	this->ulTimeout = 0;
	this->bIsTimeoutEnabled = false;
	init();
}

Semaphore::~Semaphore() {
	sem_destroy(&this->tagSemaphore);
	disableTimeout();
}

int32_t Semaphore::init() {
	int32_t ret = 0;

	this->mtx.lock();
	this->lBlocked = 0;
	this->mtx.unlock();

	ret = sem_init(&this->tagSemaphore, 0, SEMAPHORE_INITIAL_VALUE);
	if (ret >= 0)
	{
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}

int32_t Semaphore::post() {
	int32_t ret = 0;

	this->mtx.lock();
	this->ulTimeoutID = 1001;
	this->lBlocked--;
	this->mtx.unlock();

	ret = sem_post(&this->tagSemaphore);

	if(ret != 0)
	{
	   ret = -1; // EXIT
	}

	return ret;
}

int32_t Semaphore::wait() {
	int32_t ret = 0;

	this->mtx.lock();
	this->miID = rand() % 100;
	this->lBlocked++;
	this->mtx.unlock();

	if (this->bIsTimeoutEnabled == true)
	{
		start();
	}

	ret = sem_wait(&this->tagSemaphore);

	if(ret != 0)
	{
	   ret = -1; // EXIT
	}

	return ret;
}

void Semaphore::enableTimeout(uint32_t timeout)
{
	this->ulTimeout = timeout;
	this->bIsTimeoutEnabled = true;

	this->ulTimeoutID = rand() % 1000;
}

void Semaphore::disableTimeout()
{
	if (this->bIsTimeoutEnabled == true)
	{
		pthread_detach(this->tagTimeoutThread);
	}
}


int32_t Semaphore::run()
{
	uint32_t ulPrevID = this->ulTimeoutID;

	sleep(this->ulTimeout);

	if (ulPrevID == this->ulTimeoutID)
	{
		post();
		ret = -1;
	}

	return 0;
}
