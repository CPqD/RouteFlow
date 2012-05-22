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
#include "openflow-action.hh"
#include "vlog.hh"

namespace vigil
{
  static Vlog_module lg("openflow-action");

  void ofp_action::set_action_nw_addr(uint16_t type, uint32_t ip)
  {
    action_raw.reset(new uint8_t[sizeof(ofp_action_nw_addr)]);
    of_action_nw_addr oana(type, sizeof(ofp_action_nw_addr),ip);
    oana.pack((ofp_action_nw_addr*) openflow_pack::get_header(action_raw));
  }

  void ofp_action::set_action_dl_addr(uint16_t type, ethernetaddr mac)
  {
    action_raw.reset(new uint8_t[sizeof(ofp_action_dl_addr)]);
    of_action_dl_addr oada;
    oada.type = type;
    oada.len = sizeof(ofp_action_dl_addr);
    memcpy(oada.dl_addr, mac.octet, ethernetaddr::LEN);
    oada.pack((ofp_action_dl_addr*) openflow_pack::get_header(action_raw));
  }

  void ofp_action::set_action_enqueue(uint16_t port, uint32_t queueid)
  {
    action_raw.reset(new uint8_t[sizeof(ofp_action_enqueue)]);
    of_action_enqueue oae;
    oae.type = OFPAT_ENQUEUE;
    oae.len = sizeof(ofp_action_enqueue);
    oae.port = port;
    oae.queue_id = queueid;
    oae.pack((ofp_action_enqueue*) openflow_pack::get_header(action_raw));
  }

  void ofp_action::set_action_output(uint16_t port, uint16_t max_len)
  {
    action_raw.reset(new uint8_t[sizeof(ofp_action_output)]);
    of_action_output oao(OFPAT_OUTPUT, sizeof(ofp_action_output), port, max_len);
    oao.pack((ofp_action_output*) openflow_pack::get_header(action_raw));
  }

  of_action_header ofp_action::get_header()
  {
    of_action_header oah;
    oah.unpack((ofp_action_header*) openflow_pack::get_pointer(action_raw));
    return oah;
  }

  uint16_t ofp_action_list::mem_size()
  {
    uint16_t size = 0;
    std::list<ofp_action>::iterator i = action_list.begin();
    while (i != action_list.end())
    {
      size += i->get_header().len;
      i++;
    }

    return size;
  }

  void ofp_action_list::pack(uint8_t* actions)
  {
    std::list<ofp_action>::iterator i = action_list.begin();
    while (i != action_list.end())
    {
      of_action_header oah = i->get_header();
      memcpy(actions, i->action_raw.get(), oah.len);
      actions += oah.len;
      i++;
    }
  }
  
}
