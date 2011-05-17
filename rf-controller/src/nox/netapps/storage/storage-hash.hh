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
#ifndef STORAGE_HASH_HH
#define STORAGE_HASH_HH 1

#include "hash.hh"
#include "fnv_hash.hh"

/* G++ 4.2+ has hash template specializations in std::tr1, while G++
   before 4.2 expect them to be in __gnu_cxx. Hence the namespace
   macro magic. */
ENTER_HASH_NAMESPACE

template<>
struct hash<vigil::applications::storage::GUID>
    : public std::unary_function<vigil::applications::storage::GUID, 
                                 std::size_t>
{      
    std::size_t
    operator()(const vigil::applications::storage::GUID& guid) const {
        return vigil::fnv_hash((const char*)guid.guid, sizeof(guid.guid));
    }
};

template<>
struct hash<vigil::applications::storage::Reference>
    : public std::unary_function<vigil::applications::storage::Reference, 
                                 std::size_t>
{      
    std::size_t
    operator()(const vigil::applications::storage::Reference& ref) const {
        uint32_t x = vigil::fnv_hash(ref.guid.guid, sizeof(ref.guid.guid));
        return vigil::fnv_hash(&ref.version, sizeof(ref.version), x);
    }
};

template<>
struct hash<vigil::applications::storage::Trigger_id>
    : public std::unary_function<vigil::applications::storage::Trigger_id, 
                                 std::size_t>
{      
    std::size_t
    operator()(const vigil::applications::storage::Trigger_id& t) const {
        uint32_t x = hash<std::string>()(t.ring);
        x = vigil::fnv_hash(t.ref.guid.guid, sizeof(t.ref.guid.guid), x);
        x = vigil::fnv_hash(&t.ref.version, sizeof(t.ref.version), x);
        x = vigil::fnv_hash(&t.tid, sizeof(t.tid), x);
        return x;
    }
};

EXIT_HASH_NAMESPACE
#endif
