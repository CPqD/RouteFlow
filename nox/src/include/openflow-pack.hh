/** Copyright 2009 (C) Stanford University
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
#ifndef OPENFLOW_PACK_H
#define OPENFLOW_PACK_H

#include "openflow-pack-raw.hh"
#include "buffer.hh"
#include "xtoxll.h"
#include <boost/shared_array.hpp>
#include <stdlib.h>
#include <string.h>

/** \brief Minimum transaction id.
 *
 * All xid smaller than this value are reserved 
 * and will not be used.
 */
#define OPENFLOW_PACK_MIN_XID 256

/** \brief Mode of generating transaction id.
 * 
 * Defaults to increment if not defined.
 */
#define OPENFLOW_PACK_XID_METHOD_INCREMENT 0
#define OPENFLOW_PACK_XID_METHOD_RANDOM 1
#define OPENFLOW_PACK_XID_METHOD_PRIVSPACE 2
#define OPENFLOW_PACK_XID_METHOD OPENFLOW_PACK_XID_METHOD_PRIVSPACE

namespace vigil
{
  /** \brief Raw classes to help pack OpenFlow messages
   * \ingroup noxapi
   *
   * @author ykk
   * @date May 2010
   * @file openflow-pack-raw.hh
   * @file openflow.h
   */

  /** \brief Class to help pack OpenFlow messages
   * \ingroup noxapi
   *
   * @see <A HREF="openflow-pack-raw_8hh.html">openflow-pack-raw</A>
   * @author ykk
   * @date May 2010
   */
  class openflow_pack
  {
  public:
    /** \brief Create OpenFlow header
     *
     * @param type type of message
     * @param length lenght of message
     */
    static inline of_header header(uint8_t type, uint16_t length)
    {
      return of_header(OFP_VERSION, type, length, get_xid());
    }

    /** \brief Get pointer to array with offset
     *
     * @param raw raw array
     * @param offset offset to use
     * @return pointer to array with offset
     */
    static inline uint8_t* get_pointer(boost::shared_array<uint8_t>& raw,
				       size_t offset=0)
    {
      return ((uint8_t*) raw.get())+offset;
    }

    /** \brief Get pointer to array with offset
     *
     * @param buf buffer
     * @param offset offset to use
     * @return pointer to array with offset
     */
    static inline uint8_t* get_pointer(const Buffer& buf,
				       size_t offset=0)
    {
      return ((uint8_t*) buf.data())+offset;
    }

    /** \brief Get OpenFlow header pointer.
     *
     * @param buf buffer
     * @return pointer to OpenFlow header
     */
    static inline ofp_header* get_header(const Buffer& buf)
    {
      return (ofp_header*) buf.data();
    }

    /** \brief Get OpenFlow header pointer.
     *
     * @param of_raw raw OpenFlow packet
     * @return pointer to OpenFlow header
     */
    static inline ofp_header* get_header(boost::shared_array<uint8_t>& of_raw)
    {
      return (ofp_header*) of_raw.get();
    }

    /** \brief Extract xid from packet.
     * 
     * @param buf buffer
     * @return xid
     */
    static inline uint32_t xid(boost::shared_array<uint8_t>& buf)
    {
      return ntohl(get_header(buf)->xid);
    }

    /** \brief Get next xid for OpenFlow
     *
     * @return xid to use
     */
    static uint32_t get_xid();

    /** \brief Copy memory with offset in destination
     *
     * @param dest destination pointer
     * @param src source pointer
     * @param len lenght to copy
     * @param offset offset in destination
     * @param destination
     */
    static inline void* memcopy(void* dest, const void* src, size_t len, size_t offset=0)
    {
      return memcpy(((uint8_t*) dest)+offset, src, len);
    }
    
    /** \brief Copy memory with offset in destination
     *
     * @param dest destination array
     * @param src source pointer
     * @param len lenght to copy
     * @param offset offset in destination
     * @param destination
     */
    static inline void* memcopy(boost::shared_array<uint8_t>& dest,
				const void* src, size_t len, size_t offset=0)
    {
      return memcopy(dest.get(), src, len, offset);
    }
    
  private:
    /** Next OpenFlow xid (if method is increment)
     */
    static uint32_t nextxid;

    /** \brief Random unsigned 32 bit integer
     *
     * @return random unsigned long
     */
    static uint32_t rand_uint32();
  };
} // namespace vigil

#endif
