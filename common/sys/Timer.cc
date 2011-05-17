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

#include "Timer.hh"
#include <cstring>

int Timer::timerNum = 0;

Timer::Timer(tmrCallback callback, void * userData, int32_t period)
		throw (string)
{
	struct sigevent event;

	if (NULL == callback)
	{
		throw("Could not create a new timer");
	}

	if ((SIGRTMIN + Timer::timerNum) > SIGRTMAX)
	{
		throw("Could not create a new timer: resource unavailable");
	}

	if (this->setPeriod(period) < 0)
	{
		throw("Could not create a new timer: invalid period value");
	}

	this->m_data = userData;
	this->m_pFunc = callback;

	/* Set up event handler: */
	{
		std::memset(&event, 0, sizeof(event));
		event.sigev_notify = SIGEV_THREAD;
		event.sigev_signo = SIGRTMIN + Timer::timerNum++;
		event.sigev_notify_function = Timer::event;
		event.sigev_value.sival_ptr = this;
	}

	/* Set up timer: */
	if (timer_create(CLOCK_REALTIME, &event, &this->m_timerId) < 0)
	{
		throw("Could not create a new timer: timer_create error");
	}
}

Timer::~Timer()
{
	timer_delete(this->m_timerId);
}

void Timer::event(union sigval sigev_value)
{
	Timer * timer = (Timer *) (sigev_value.sival_ptr);

	timer->execute();
}

int32_t Timer::start()
{
	struct itimerspec aTimeVal;

	/* Fill timer value*/
	{
		memset(&aTimeVal, 0, sizeof(aTimeVal));

		aTimeVal.it_value.tv_sec = this->m_period.tv_sec;
		aTimeVal.it_value.tv_nsec = this->m_period.tv_nsec;

		aTimeVal.it_interval.tv_sec = aTimeVal.it_value.tv_sec;
		aTimeVal.it_interval.tv_nsec = aTimeVal.it_value.tv_nsec;
	}

	/* Arm the timer*/
	if (timer_settime(this->m_timerId, 0, &aTimeVal, NULL))
	{
		return -1;
	}

	return 0;

}

int32_t Timer::stop()
{
	struct itimerspec aTimeVal;

	/* Set aTimeVal.it_value to zero */
	{
		std::memset(&aTimeVal, 0, sizeof(aTimeVal));
	}

	/* Disarm the timer */
	if (timer_settime(this->m_timerId, 0, &aTimeVal, NULL))
	{
		return -1;
	}

	return 0;
}

int32_t Timer::restart()
{
	int32_t ret;

	if ((ret = this->stop()) >= 0)
	{
		ret = this->start();
	}

	return ret;
}

int32_t Timer::setPeriod(int32_t mSec)
{
	if (mSec < 0)
	{
		return -1;
	}

	this->m_period.tv_sec = mSec / 1000;
	this->m_period.tv_nsec = (mSec % 1000) * 1000000;

	return 0;
}

void Timer::execute()
{
	this->m_pFunc(this->m_data);
}
