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
#ifndef AUTO_FREE_HH
#define AUTO_FREE_HH 1

/* This is like std::auto_ptr but it uses free to destroy its data. */

#include <cstdlib>

namespace vigil {

template <class T>
struct auto_free_ref
{
    T* ptr;
    auto_free_ref(T* that) : ptr(that) { }
};

template <class T>
class auto_free
{
public:
    typedef T element_type;

    explicit auto_free(T* ptr_ = 0) : ptr(ptr_) { }
    auto_free(auto_free& that) : ptr(that.release()) { }
    template <class U>
    auto_free(auto_free<U>& that) : ptr(that.release()) { }

    ~auto_free() { std::free(ptr); }

    auto_free& operator=(auto_free& rhs) {
	reset(rhs.release());
	return *this;
    }
    template <class U>
    auto_free& operator=(auto_free<U>& rhs) {
	reset(rhs.release());
	return *this;
    }

    T* get() const { return ptr; }
    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
    T& operator[](size_t idx) { return ptr[idx]; }

    T* release() { T* tmp = ptr; ptr = 0; return tmp; }
    void reset(T* ptr_ = 0) {
	if (ptr != ptr_) {
	    std::free(ptr);
	    ptr = ptr_;
	}
    }

    auto_free(auto_free_ref<T> rhs) : ptr(rhs.ptr) { }
    auto_free& operator=(auto_free_ref<T> rhs) {
	reset(rhs.ptr);
	return *this;
    }
    template <class U>
    operator auto_free_ref<U>()
	{ return auto_free_ref<U>(release()); }
    template <class U>
    operator auto_free<U>()
	{ return auto_free<U>(release()); }

private:
    T* ptr;
};

} // namespace vigil

#endif /* auto_free.hh */
