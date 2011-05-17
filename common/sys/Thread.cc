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

#include "Thread.hh"

Thread::Thread ()
{
	this->running = false;
	this->thArg = NULL;
}

Thread::~Thread()
{
	if (this->running)
	{
		this->stop();
		this->waitOn();
	}
}

bool Thread::isRunning ()
{
	return this->running;
}

int32_t Thread::waitOn ()
{
	int32_t ret = 0;

	if (this->running)
	{
		ret = pthread_join(this->thId, NULL);
	}

	return ret;
}

void Thread::arg(void * arg)
{
	this->thArg = arg;
}

int32_t Thread::setup ()
{
	return 0;
}

int32_t Thread::start ()
{
	int32_t ret = 0;

	if (!this->running)
	{
		ret = pthread_create(&this->thId, NULL, entryPoint, (void*)this);
		if (ret == 0)
		{
			this->running = true;
		}
	}

	return ret;
}

int32_t Thread::stop ()
{
	int32_t ret = 0;

	if (this->running)
	{
		ret = pthread_cancel(this->thId);
		if (ret == 0)
		{
			this->running = false;
		}
	}

	return ret;
}

pthread_t Thread::id ()
{
	return this->thId;
}

void* Thread::entryPoint (void* pThis)
{
	Thread* temp = (Thread*)pThis;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	temp->setup();
	temp->run(temp->thArg);

	return 0;
}
