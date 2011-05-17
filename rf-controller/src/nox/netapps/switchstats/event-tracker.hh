/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <deque>
#include <cassert>
#include "timeval.hh"

extern "C" {
#include <sys/time.h>
}

template <typename T, typename alloc = std::allocator<T> > 
struct fixed_len_queue :  public std::deque<T, alloc>
{
    protected:

        int max_size; 

    public:    

    fixed_len_queue(int size): max_size(size) {
    }

   void  push(T t) {
       std::deque<T,alloc>::push_front(t);
       if (this->size() > max_size){
           std::deque<T,alloc>::pop_back();
       }
    }

};

class EventTracker
{
    int       timeslice; // timeslice length in seconds
    timeval   tv_start;  // timestamp of slice start 

    int   current_total; // aggregate value this timeslice

    int       event_count;

    fixed_len_queue<float>    history_q; 
    fixed_len_queue<long int> count_q;   

    public:
        EventTracker(int ts = 1000, int queue_len = 10, timeval* tv_ = 0);

        void reset(int ts = 1000, int queue_len = 10, timeval* tv_ = 0);

        void add_event(int val);
        void add_event(int val, timeval* tv_);

        void sync(timeval* tv);

        fixed_len_queue<float>&    get_history_q_ref();
        fixed_len_queue<long int>& get_count_q_ref();
};

inline
EventTracker::EventTracker(int ts, int queue_len, timeval* tv_) :
    timeslice(ts),  current_total(0.), 
    event_count(0), 
    history_q(queue_len), count_q(queue_len)
{
    if(tv_){
        tv_start = *tv_;
    }else{
        ::gettimeofday(&tv_start, 0);
    }
}

inline
void 
EventTracker::reset(int ts, int queue_len, timeval* tv_)
{
    timeslice = ts;
    current_total = 0;
    event_count   = 0;
    history_q.clear();
    count_q.clear();

    if(tv_){
        tv_start = *tv_;
    }else{
        ::gettimeofday(&tv_start, 0);
    }
}

inline
void 
EventTracker::add_event(int val)
{
    timeval tv;
    ::gettimeofday(&tv, 0);
    add_event(val, &tv);
}


inline
void 
EventTracker::add_event(int val, timeval* tv)
{
    sync(tv);
    current_total += val;
}

inline
void
EventTracker::sync(timeval* tv)
{
    timeval dtv = (*tv) - tv_start; 
    long int delta = timeval_to_ms(dtv);
    assert(timeval_to_ms(dtv) >= 0);

    if(delta <= (timeslice)){
        return; // slice not over
    }

    // all events must have happened in the first timeslice 
    float new_avg =  (float)current_total / 
                    ((float)(timeslice)/1000.);
    history_q.push(new_avg);
    count_q   .push(event_count++);
    current_total = 0.; 

    // add 0 sums for all subsequent time slices
    delta -= (timeslice);
    while ( delta > (timeslice)){
        history_q.push(0);
        delta -= (timeslice);
    }

    ::gettimeofday(&tv_start, 0);
}

inline
fixed_len_queue<float>& 
EventTracker::get_history_q_ref()
{
    return history_q;
}

inline
fixed_len_queue<long int>& 
EventTracker::get_count_q_ref()
{
    return count_q;
}
