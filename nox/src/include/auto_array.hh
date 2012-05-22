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
#ifndef AUTO_ARRAY_HH
#define AUTO_ARRAY_HH 1

namespace vigil {

/* This is like std::auto_ptr but it uses delete[] to destroy its data.  It
 * doesn't have all of the member functions that auto_ptr does, because it
 * doesn't make sense to convert a pointer to the first element of an array to
 * another pointer type (except in occasional special cases where you know the
 * two element types are the same size).
 */
template <class T>
class auto_array
{
public:
    typedef T element_type;

    explicit auto_array(T* ptr_ = 0) : ptr(ptr_) { }
    auto_array(auto_array& that) : ptr(that.release()) { }
    ~auto_array() { delete[] ptr; }

    auto_array& operator=(auto_array& rhs) {
	reset(rhs.release());
	return *this;
    }

    T* get() const { return ptr; }
    T& operator[](size_t idx) { return ptr[idx]; }

    T* release() { T* tmp = ptr; ptr = 0; return tmp; }
    void reset(T* ptr_ = 0) {
	if (ptr != ptr_) {
	    delete[] ptr;
	    ptr = ptr_;
	}
    }
private:
    T* ptr;
};

} // namespace vigil

#endif /* auto_array.hh */
