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

#ifndef THREAD_HH_
#define THREAD_HH_

#include <sys/types.h>
#include <pthread.h>

class Thread
{
	public:
		Thread ();
		virtual ~Thread();

		int32_t start ();
		int32_t stop ();
		bool isRunning ();
		int32_t waitOn ();
		void arg(void * arg);

		pthread_t id ();

	protected:
		virtual int32_t setup ();
		virtual int32_t run (void * arg) = 0;

	private:
		static void* entryPoint (void * pThis);
		bool running;
		pthread_t thId;
		void * thArg;
};


#endif /* THREAD_HH_ */
