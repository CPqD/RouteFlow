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

#ifndef TIMER_HH_
#define TIMER_HH_

#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <string>

using std::string;

typedef void (* tmrCallback)(void * userData);

class Timer
{
	static int32_t timerNum;

	timer_t m_timerId;
	tmrCallback m_pFunc;
	void * m_data;
	struct timespec m_period;

	static void event(union sigval sigev_value);

public:

	Timer(tmrCallback callback, void * userData, int32_t period) throw (string);
	~Timer();

	int32_t start();
	int32_t stop();
	int32_t restart();
	void execute();
	int32_t setPeriod(int32_t mSec);
};

#endif /* TIMER_HH_ */
