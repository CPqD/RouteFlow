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
#ifndef BUFFER_HH
#define BUFFER_HH 1

#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <stdexcept>

namespace vigil {

/* A Buffer is just a contiguous memory region, analogous to a struct sk_buff
 * in the Linux kernel.  Much of the terminology here is adopted from sk_buff
 * also.
 *
 * Buffer is an abstract class, but most of the functionality is in Buffer
 * itself, and thus most Buffer member functions are non-virtual.  Buffer's
 * subclasses differ among themselves in only two ways:
 *
 *      - Whether and how the underlying storage is released by the destructor,
 *        e.g.  Array_buffer uses "delete[]" of a uint8_t array,
 *        Nonowning_buffer doesn't release storage at all.
 *
 *      - Whether and how storage is reallocated when data is added to a
 *        buffer.  Not all buffers can have new data added to them at all.
 *        There is no interface for finding out whether data can be added to a
 *        buffer: this fact must be part of the contract between interfaces
 *        that pass buffers around.
 *
 * On the other hand, any buffer's data area may shrink and move around,
 * e.g. with .pull() and .trim().  Thus, subclasses cannot depend on Buffer's
 * data() remaining constant for passing to a free function and must
 * independently maintain a pointer for that purpose (if necessary).
 *
 * Buffer is not reference-counted.  If you need reference counting, pass
 * around boost::shared_ptr<Buffer>.
 *
 * Whether the underlying storage of a Buffer is modifiable is not specified.
 * Again, there is no interface for finding out and thus this fact too must be
 * part of the contract between interfaces that pass buffers around.
 *
 * Buffer is very likely open to sigificant improvement.  boost::asio, for
 * example, has both a const_buffer and an immutable_buffer, and it would be
 * interesting to see whether this model can be made to work here too. */
class Buffer
{
public:
    virtual ~Buffer() = 0;

    /* Inspectors. */
    const uint8_t* data() const { return m_data; }
    uint8_t* data() { return m_data; }
    void* tail() { return m_data + m_size; }
    size_t size() const { return m_size; }
    template <class T> T& at(size_t offset) const;
    template <class T> T* try_at(size_t offset) const;

    /* Shrinking. */
    uint8_t* pull(size_t);
    uint8_t* try_pull(size_t);
    template <class T> T* pull();
    template <class T> T* try_pull();
    void trim(size_t);

    /* Extending.
     *
     * Not available in all subclasses.  May invalidate all pointers and
     * references.*/
    virtual uint8_t* push(size_t) = 0;
    virtual uint8_t* put(size_t) = 0;

protected:
    uint8_t* m_data;
    size_t m_size;

    void init(uint8_t* data_, size_t size_);

    Buffer(uint8_t* data_, size_t size_) : m_data(data_), m_size(size_) { }
    Buffer& operator=(const Buffer&);
    Buffer(): m_data(0), m_size(0) { }
private:
    Buffer(const Buffer&);
};

inline Buffer::~Buffer() { }

/* Initialize buffer outside of contstructor.  Only valid on Buffers
 * which have been created with the default constructor */
inline void Buffer::init(uint8_t* data_, size_t size_)
{
    if (m_data || m_size) {
        throw std::runtime_error("initializing already initialized buffer");
    }
    m_data = data_;
    m_size = size_;
}

/* Drops 'n' bytes from the front of the buffer, thereby reducing its size by
 * 'n' bytes.  Returns a pointer to the first byte dropped.  The buffer's size
 * must be at least 'n' bytes. */
inline uint8_t* Buffer::pull(size_t n)
{
    assert(n <= m_size);
    uint8_t* p = m_data;
    m_data += n;
    m_size -= n;
    return p;
}

/* If the buffer's size is at least 'n' bytes, drops those bytes from the front
 * of the buffer and returns a pointer to the first byte dropped.  Otherwise,
 * returns a null pointer without modifying the buffer. */
inline uint8_t* Buffer::try_pull(size_t n)
{
    return size() < n ? NULL : pull(n);
}

/* Pulls an object of type T off the front of the buffer and returns its
 * address.  The buffer's size must be at least 'n' bytes.
 *
 * Data structure alignment must be considered in use of this function. */
template <class T>
T* Buffer::pull() {
    return reinterpret_cast<T*>(pull(sizeof(T)));
}

/* If the buffer is as big as type T, pulls an object of type T off the front
 * of the buffer and returns its address.  Otherwise, returns a null pointer
 * without modifying the buffer.
 *
 * Data structure alignment must be considered in use of this function. */
template <class T>
T* Buffer::try_pull() {
    return reinterpret_cast<T*>(try_pull(sizeof(T)));
}

/* Sets the length of the buffer's data to 'n' bytes, discarding bytes
 * at the end of the buffer if necessary. */
inline void Buffer::trim(size_t n)
{
    assert(n <= m_size);
    m_size = n;
}

/* Returns a reference to data structure T mapped 'offset' bytes from the
 * beginning of the buffer.  It is an error if the buffer is not at least
 * 'offset + sizeof(T)' bytes in size.
 *
 * Data structure alignment must be considered in use of this function. */
template <class T>
T& Buffer::at(size_t offset) const
{
    assert(offset + sizeof(T) <= m_size);
    return *reinterpret_cast<T*>(&m_data[offset]);
}

/* Returns a pointer to data structure T mapped 'offset' bytes from the
 * beginning of the buffer, or a null pointer if the buffer is not at least
 * 'offset + sizeof(T)' bytes in size.
 *
 * Data structure alignment must be considered in use of this function. */
template <class T>
T* Buffer::try_at(size_t offset) const
{
    return offset + sizeof(T) <= m_size ? &at<T>(offset) : NULL;
}

/* Buffer for uint8_t[] that destroys its buffer with delete[].  Implements
 * operations that extend the buffer by creating a new underlying array and
 * copying data. */
class Array_buffer
    : public Buffer
{
public:
    Array_buffer(size_t size);
    Array_buffer(uint8_t*, size_t);
    ~Array_buffer() { delete[] base; }

    uint8_t* push(size_t n);
    uint8_t* put(size_t n);

protected:    
    Array_buffer() : Buffer() { }

    uint8_t* base;
    size_t capacity;

private:
    Array_buffer(const Array_buffer&);
    Array_buffer& operator=(const Array_buffer&);
};

/* Allocate a new Array_buffer of 'size' bytes. */
inline Array_buffer::Array_buffer(size_t size_)
    : Buffer(new uint8_t[size_], size_), base(data()), capacity(size_)
{ }

/* Constructs an Array_buffer that takes possession of the 'size' bytes at
 * 'data'.  The data will be destroyed with delete[] upon the Array_buffer's
 * destruction.
 *
 * Note that C++ language rules technically require 'data' to have been
 * allocated with 'new uint8_t[]', not, say, 'new char[]' and definitely not
 * 'malloc'.  Use a different Buffer subclass if you need either of those. */
inline Array_buffer::Array_buffer(uint8_t* data_, size_t size_)
    : Buffer(data_, size_), base(data_), capacity(size_)
{ }

/* A buffer that does not own its content.  The destructor does not do
 * anything, and the buffer may not be extended.
 *
 * Because a Nonowning_buffer does not own its content, the client must take
 * care to ensure that the content is not destroyed (even by reallocation)
 * while the Nonowning_buffer is still in use.
 *
 * This class would be better named Subbuffer, because that's one of its
 * primary usages in practice.
 */
class Nonowning_buffer
    : public Buffer
{
public: 
    Nonowning_buffer(): Buffer() { }
    Nonowning_buffer(const Buffer&);
    Nonowning_buffer(const Nonowning_buffer&);
    Nonowning_buffer(const Buffer&, size_t offset, size_t length = SIZE_MAX);
    Nonowning_buffer(const void*, size_t);
    ~Nonowning_buffer() { }

    /* initialize after construction. */ 
    void init(uint8_t* data_, size_t size_);

    /* initialize at any point */
    void reinit(uint8_t* data_, size_t size_);

    /* A Nonowning_buffer cannot be extended. */
    uint8_t* push(size_t n) { ::abort(); }
    uint8_t* put(size_t n) { ::abort(); }
};

/* Constructs a Nonowning_buffer whose contents are the same as 'buffer''s. */
inline Nonowning_buffer::Nonowning_buffer(const Buffer& buffer)
    : Buffer(const_cast<uint8_t*>(buffer.data()), buffer.size())
{}

/* Constructs a Nonowning_buffer whose contents are the same as 'buffer''s. */
inline Nonowning_buffer::Nonowning_buffer(const Nonowning_buffer& that)
    : Buffer(const_cast<uint8_t*>(that.data()), that.size()) 
{}

/* Constructs a Nonowning_buffer whose contents are a substring of 'buffer''s
 * that starts at the given 'offset' and extends for up to 'length' bytes.
 *
 * 'offset' must be no longer than the size of 'buffer', but 'length' will be
 * trimmed as necessary to fit. */
inline Nonowning_buffer::Nonowning_buffer(const Buffer& buffer,
                                          size_t offset, size_t length)
    : Buffer(const_cast<uint8_t*>(buffer.data()), buffer.size())
{
    pull(offset);
    if (length < size()) {
        trim(length);
    }
}

/* Constructs a Nonowning_buffer whose contents are the 'length' bytes starting
 * at 'data'. */
inline Nonowning_buffer::Nonowning_buffer(const void* data_, size_t length)
    : Buffer(static_cast<uint8_t*>(const_cast<void*>(data_)), length)
{}

inline void Nonowning_buffer::init(uint8_t* data_, size_t size_)
{
    Buffer::init(data_, size_);
}

/* Reset underlying content store to new location at data_, discarding old 
 * location.  It is the caller's responsibility to ensure no external
 * references will be negatively impacted */
inline void Nonowning_buffer::reinit(uint8_t* data_, size_t size_)
{
    m_data = data_;
    m_size = size_;
}

} // namespace vigil

#endif /* buffer.hh */
