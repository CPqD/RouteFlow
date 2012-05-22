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
#include "buffer.hh"
#include <algorithm>
#include <cstring>

namespace vigil {

/* Adds 'n' bytes to the front of the buffer and returns the first byte of the
 * added storage. */
uint8_t* Array_buffer::push(size_t n)
{
    size_t headroom = data() - base;
    if (headroom >= n) {
        m_data -= n;
    } else {
        capacity += n - headroom;

        uint8_t* new_base = new uint8_t[capacity];
        std::memcpy(new_base + n, data(), size());
        delete[] base;
        m_data = base = new_base;
    }
    m_size += n;
    return m_data;
}

/* Adds 'n' bytes to the end of the buffer and returns the first byte of the
 * added storage. */
uint8_t* Array_buffer::put(size_t n)
{
    size_t tailroom = (base + capacity) - (data() + size());
    if (tailroom < n) {
        size_t headroom = data() - base;
        capacity += std::max(capacity, n - tailroom);

        uint8_t* new_base = new uint8_t[capacity];
        std::memcpy(new_base + headroom, data(), size());
        delete[] base;

        base = new_base;
        m_data = base + headroom;
    }

    uint8_t* p = data() + size();
    m_size += n;
    return p;
}

} // namespace vigil
