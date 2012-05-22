/* Copyright 2008 (C) Nicira, Inc.
 * Copyright 2009 (C) Stanford University.
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
#ifndef rfproxy_HH
#define rfproxy_HH

#include "component.hh"
#include "config.h"
#include "ipc/IPC.h"
#include "ipc/RFProtocolFactory.h"
#include "rftable/RFTable.h"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

struct packet_data {
	uint8_t packet[2048];
	uint32_t size;

}__attribute__((packed));

struct eth_data {
	uint8_t eth_dst[6]; /* Destination MAC address. */
	uint8_t eth_src[6]; /* Source MAC address. */
	uint16_t eth_type; /* Packet type. */
	uint64_t vm_id; /* Number which identifies a Virtual Machine .*/
	uint8_t vm_port; /* Number of the Virtual Machine port */

}__attribute__((packed));

namespace vigil
{
using namespace std;
using namespace vigil::container;

class rfproxy : public Component, private IPCMessageProcessor 
{
    public:
        rfproxy(const Context* c, const json_object* node) : Component(c) {}
        void configure(const Configuration* c);
        void install();
        static void getInstance(const container::Context* c, rfproxy*& component);

    private:
        IPCMessageService* ipc;
        IPCMessageProcessor *processor;
        RFProtocolFactory *factory;
        RFTable *rftable
        ;
        Disposition handle_datapath_join(const Event& e);
        Disposition handle_datapath_leave(const Event& e);
        Disposition handle_packet_in(const Event& e);
        bool process(const string &from, const string &to, const string &channel, IPCMessage& msg);
};
}

#endif
