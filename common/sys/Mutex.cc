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

#include <sys/types.h>
#include "Mutex.hh"

Mutex::Mutex() {

	this->bIsLocked = false;
	pthread_mutex_init(&tagLock, NULL);
}

Mutex::~Mutex() {
	this->bIsLocked = false;
	pthread_mutex_destroy(&tagLock);
}


int32_t Mutex::lock()
{
	int32_t ret = 0;
	ret = pthread_mutex_lock(&tagLock);

	if (!ret)
	{
		this->bIsLocked = true;
	}

	return ret;
}

int32_t Mutex::unlock()
{
	int32_t ret = 0;
	ret = pthread_mutex_unlock(&tagLock);

	if (!ret)
	{
		this->bIsLocked = false;
	}

	return ret;
}

bool Mutex::isLocked()
{
	return this->bIsLocked;
}
