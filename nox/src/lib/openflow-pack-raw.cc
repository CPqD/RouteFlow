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
#include <stdlib.h>
#include <string.h>
#include "xtoxll.h"
#include "openflow-pack-raw.hh"

namespace vigil
{

        /** \brief Empty constructor with default assigned
         */
    of_phy_port::of_phy_port()
    {
        port_no = 0;
        hw_addr[0] = 0;
        hw_addr[1] = 0;
        hw_addr[2] = 0;
        hw_addr[3] = 0;
        hw_addr[4] = 0;
        hw_addr[5] = 0;
        config = 0;
        state = 0;
        curr = 0;
        advertised = 0;
        supported = 0;
        peer = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_phy_port::of_phy_port(uint16_t port_no_, uint8_t hw_addr_[], std::string name_, uint32_t config_, uint32_t state_, uint32_t curr_, uint32_t advertised_, uint32_t supported_, uint32_t peer_)
    {
        port_no = port_no_;
        hw_addr[0] = hw_addr_[0];
        hw_addr[1] = hw_addr_[1];
        hw_addr[2] = hw_addr_[2];
        hw_addr[3] = hw_addr_[3];
        hw_addr[4] = hw_addr_[4];
        hw_addr[5] = hw_addr_[5];
        name = name_;
        config = config_;
        state = state_;
        curr = curr_;
        advertised = advertised_;
        supported = supported_;
        peer = peer_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_phy_port::pack(ofp_phy_port* buffer)
    {
        buffer->port_no = htons(port_no);
        buffer->hw_addr[0] = (hw_addr[0]);
        buffer->hw_addr[1] = (hw_addr[1]);
        buffer->hw_addr[2] = (hw_addr[2]);
        buffer->hw_addr[3] = (hw_addr[3]);
        buffer->hw_addr[4] = (hw_addr[4]);
        buffer->hw_addr[5] = (hw_addr[5]);
        strncpy(buffer->name,name.c_str(),15);
        buffer->name[15] = '\0';
        buffer->config = htonl(config);
        buffer->state = htonl(state);
        buffer->curr = htonl(curr);
        buffer->advertised = htonl(advertised);
        buffer->supported = htonl(supported);
        buffer->peer = htonl(peer);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_phy_port::unpack(ofp_phy_port* buffer)
    {
        port_no = ntohs(buffer->port_no);
        hw_addr[0] = (buffer->hw_addr[0]);
        hw_addr[1] = (buffer->hw_addr[1]);
        hw_addr[2] = (buffer->hw_addr[2]);
        hw_addr[3] = (buffer->hw_addr[3]);
        hw_addr[4] = (buffer->hw_addr[4]);
        hw_addr[5] = (buffer->hw_addr[5]);
        name.assign(buffer->name);
        config = ntohl(buffer->config);
        state = ntohl(buffer->state);
        curr = ntohl(buffer->curr);
        advertised = ntohl(buffer->advertised);
        supported = ntohl(buffer->supported);
        peer = ntohl(buffer->peer);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_phy_port& of_phy_port::operator=(const of_phy_port& peer_)
    {
        port_no = peer_.port_no;
        hw_addr[0] = peer_.hw_addr[0];
        hw_addr[1] = peer_.hw_addr[1];
        hw_addr[2] = peer_.hw_addr[2];
        hw_addr[3] = peer_.hw_addr[3];
        hw_addr[4] = peer_.hw_addr[4];
        hw_addr[5] = peer_.hw_addr[5];
        name = peer_.name;
        config = peer_.config;
        state = peer_.state;
        curr = peer_.curr;
        advertised = peer_.advertised;
        supported = peer_.supported;
        peer = peer_.peer;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_phy_port::operator==(const of_phy_port& peer_) const
    {
        return \
            (port_no == peer_.port_no) &&\
            (hw_addr[0] == peer_.hw_addr[0]) &&\
            (hw_addr[1] == peer_.hw_addr[1]) &&\
            (hw_addr[2] == peer_.hw_addr[2]) &&\
            (hw_addr[3] == peer_.hw_addr[3]) &&\
            (hw_addr[4] == peer_.hw_addr[4]) &&\
            (hw_addr[5] == peer_.hw_addr[5]) &&\
            (name == peer_.name) &&\
            (config == peer_.config) &&\
            (state == peer_.state) &&\
            (curr == peer_.curr) &&\
            (advertised == peer_.advertised) &&\
            (supported == peer_.supported) &&\
            (peer == peer_.peer);
    }


        /** \brief Empty constructor with default assigned
         */
    of_aggregate_stats_reply::of_aggregate_stats_reply()
    {
        packet_count = 0;
        byte_count = 0;
        flow_count = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_aggregate_stats_reply::of_aggregate_stats_reply(uint64_t packet_count_, uint64_t byte_count_, uint32_t flow_count_, uint8_t pad_[])
    {
        packet_count = packet_count_;
        byte_count = byte_count_;
        flow_count = flow_count_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_aggregate_stats_reply::pack(ofp_aggregate_stats_reply* buffer)
    {
        buffer->packet_count = htonll(packet_count);
        buffer->byte_count = htonll(byte_count);
        buffer->flow_count = htonl(flow_count);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_aggregate_stats_reply::unpack(ofp_aggregate_stats_reply* buffer)
    {
        packet_count = ntohll(buffer->packet_count);
        byte_count = ntohll(buffer->byte_count);
        flow_count = ntohl(buffer->flow_count);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_aggregate_stats_reply& of_aggregate_stats_reply::operator=(const of_aggregate_stats_reply& peer_)
    {
        packet_count = peer_.packet_count;
        byte_count = peer_.byte_count;
        flow_count = peer_.flow_count;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_aggregate_stats_reply::operator==(const of_aggregate_stats_reply& peer_) const
    {
        return \
            (packet_count == peer_.packet_count) &&\
            (byte_count == peer_.byte_count) &&\
            (flow_count == peer_.flow_count) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_table_stats::of_table_stats()
    {
        table_id = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        wildcards = 0;
        max_entries = 0;
        active_count = 0;
        lookup_count = 0;
        matched_count = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_table_stats::of_table_stats(uint8_t table_id_, uint8_t pad_[], std::string name_, uint32_t wildcards_, uint32_t max_entries_, uint32_t active_count_, uint64_t lookup_count_, uint64_t matched_count_)
    {
        table_id = table_id_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        name = name_;
        wildcards = wildcards_;
        max_entries = max_entries_;
        active_count = active_count_;
        lookup_count = lookup_count_;
        matched_count = matched_count_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_table_stats::pack(ofp_table_stats* buffer)
    {
        buffer->table_id = (table_id);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        strncpy(buffer->name,name.c_str(),31);
        buffer->name[31] = '\0';
        buffer->wildcards = htonl(wildcards);
        buffer->max_entries = htonl(max_entries);
        buffer->active_count = htonl(active_count);
        buffer->lookup_count = htonll(lookup_count);
        buffer->matched_count = htonll(matched_count);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_table_stats::unpack(ofp_table_stats* buffer)
    {
        table_id = (buffer->table_id);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        name.assign(buffer->name);
        wildcards = ntohl(buffer->wildcards);
        max_entries = ntohl(buffer->max_entries);
        active_count = ntohl(buffer->active_count);
        lookup_count = ntohll(buffer->lookup_count);
        matched_count = ntohll(buffer->matched_count);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_table_stats& of_table_stats::operator=(const of_table_stats& peer_)
    {
        table_id = peer_.table_id;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        name = peer_.name;
        wildcards = peer_.wildcards;
        max_entries = peer_.max_entries;
        active_count = peer_.active_count;
        lookup_count = peer_.lookup_count;
        matched_count = peer_.matched_count;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_table_stats::operator==(const of_table_stats& peer_) const
    {
        return \
            (table_id == peer_.table_id) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (name == peer_.name) &&\
            (wildcards == peer_.wildcards) &&\
            (max_entries == peer_.max_entries) &&\
            (active_count == peer_.active_count) &&\
            (lookup_count == peer_.lookup_count) &&\
            (matched_count == peer_.matched_count);
    }


        /** \brief Empty constructor with default assigned
         */
    of_flow_removed::of_flow_removed()
    {
        (*this).header.type = OFPT_FLOW_REMOVED;
        cookie = 0;
        priority = 0;
        reason = 0;
        pad[0] = 0;
        duration_sec = 0;
        duration_nsec = 0;
        idle_timeout = 0;
        pad2[0] = 0;
        pad2[1] = 0;
        packet_count = 0;
        byte_count = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_flow_removed::of_flow_removed(of_header header_, of_match match_, uint64_t cookie_, uint16_t priority_, uint8_t reason_, uint8_t pad_[], uint32_t duration_sec_, uint32_t duration_nsec_, uint16_t idle_timeout_, uint8_t pad2_[], uint64_t packet_count_, uint64_t byte_count_)
    {
        header = header_;
        match = match_;
        cookie = cookie_;
        priority = priority_;
        reason = reason_;
        pad[0] = pad_[0];
        duration_sec = duration_sec_;
        duration_nsec = duration_nsec_;
        idle_timeout = idle_timeout_;
        pad2[0] = pad2_[0];
        pad2[1] = pad2_[1];
        packet_count = packet_count_;
        byte_count = byte_count_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_flow_removed::pack(ofp_flow_removed* buffer)
    {
        header.pack(&buffer->header);
        match.pack(&buffer->match);
        buffer->cookie = htonll(cookie);
        buffer->priority = htons(priority);
        buffer->reason = (reason);
        buffer->pad[0] = (pad[0]);
        buffer->duration_sec = htonl(duration_sec);
        buffer->duration_nsec = htonl(duration_nsec);
        buffer->idle_timeout = htons(idle_timeout);
        buffer->pad2[0] = (pad2[0]);
        buffer->pad2[1] = (pad2[1]);
        buffer->packet_count = htonll(packet_count);
        buffer->byte_count = htonll(byte_count);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_flow_removed::unpack(ofp_flow_removed* buffer)
    {
        header.unpack(&buffer->header);
        match.unpack(&buffer->match);
        cookie = ntohll(buffer->cookie);
        priority = ntohs(buffer->priority);
        reason = (buffer->reason);
        pad[0] = (buffer->pad[0]);
        duration_sec = ntohl(buffer->duration_sec);
        duration_nsec = ntohl(buffer->duration_nsec);
        idle_timeout = ntohs(buffer->idle_timeout);
        pad2[0] = (buffer->pad2[0]);
        pad2[1] = (buffer->pad2[1]);
        packet_count = ntohll(buffer->packet_count);
        byte_count = ntohll(buffer->byte_count);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_flow_removed& of_flow_removed::operator=(const of_flow_removed& peer_)
    {
        header = peer_.header;
        match = peer_.match;
        cookie = peer_.cookie;
        priority = peer_.priority;
        reason = peer_.reason;
        pad[0] = peer_.pad[0];
        duration_sec = peer_.duration_sec;
        duration_nsec = peer_.duration_nsec;
        idle_timeout = peer_.idle_timeout;
        pad2[0] = peer_.pad2[0];
        pad2[1] = peer_.pad2[1];
        packet_count = peer_.packet_count;
        byte_count = peer_.byte_count;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_flow_removed::operator==(const of_flow_removed& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (match == peer_.match) &&\
            (cookie == peer_.cookie) &&\
            (priority == peer_.priority) &&\
            (reason == peer_.reason) &&\
            (pad[0] == peer_.pad[0]) &&\
            (duration_sec == peer_.duration_sec) &&\
            (duration_nsec == peer_.duration_nsec) &&\
            (idle_timeout == peer_.idle_timeout) &&\
            (pad2[0] == peer_.pad2[0]) &&\
            (pad2[1] == peer_.pad2[1]) &&\
            (packet_count == peer_.packet_count) &&\
            (byte_count == peer_.byte_count);
    }


        /** \brief Empty constructor with default assigned
         */
    of_port_stats::of_port_stats()
    {
        port_no = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
        rx_packets = 0;
        tx_packets = 0;
        rx_bytes = 0;
        tx_bytes = 0;
        rx_dropped = 0;
        tx_dropped = 0;
        rx_errors = 0;
        tx_errors = 0;
        rx_frame_err = 0;
        rx_over_err = 0;
        rx_crc_err = 0;
        collisions = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_port_stats::of_port_stats(uint16_t port_no_, uint8_t pad_[], uint64_t rx_packets_, uint64_t tx_packets_, uint64_t rx_bytes_, uint64_t tx_bytes_, uint64_t rx_dropped_, uint64_t tx_dropped_, uint64_t rx_errors_, uint64_t tx_errors_, uint64_t rx_frame_err_, uint64_t rx_over_err_, uint64_t rx_crc_err_, uint64_t collisions_)
    {
        port_no = port_no_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
        rx_packets = rx_packets_;
        tx_packets = tx_packets_;
        rx_bytes = rx_bytes_;
        tx_bytes = tx_bytes_;
        rx_dropped = rx_dropped_;
        tx_dropped = tx_dropped_;
        rx_errors = rx_errors_;
        tx_errors = tx_errors_;
        rx_frame_err = rx_frame_err_;
        rx_over_err = rx_over_err_;
        rx_crc_err = rx_crc_err_;
        collisions = collisions_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_port_stats::pack(ofp_port_stats* buffer)
    {
        buffer->port_no = htons(port_no);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
        buffer->rx_packets = htonll(rx_packets);
        buffer->tx_packets = htonll(tx_packets);
        buffer->rx_bytes = htonll(rx_bytes);
        buffer->tx_bytes = htonll(tx_bytes);
        buffer->rx_dropped = htonll(rx_dropped);
        buffer->tx_dropped = htonll(tx_dropped);
        buffer->rx_errors = htonll(rx_errors);
        buffer->tx_errors = htonll(tx_errors);
        buffer->rx_frame_err = htonll(rx_frame_err);
        buffer->rx_over_err = htonll(rx_over_err);
        buffer->rx_crc_err = htonll(rx_crc_err);
        buffer->collisions = htonll(collisions);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_port_stats::unpack(ofp_port_stats* buffer)
    {
        port_no = ntohs(buffer->port_no);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
        rx_packets = ntohll(buffer->rx_packets);
        tx_packets = ntohll(buffer->tx_packets);
        rx_bytes = ntohll(buffer->rx_bytes);
        tx_bytes = ntohll(buffer->tx_bytes);
        rx_dropped = ntohll(buffer->rx_dropped);
        tx_dropped = ntohll(buffer->tx_dropped);
        rx_errors = ntohll(buffer->rx_errors);
        tx_errors = ntohll(buffer->tx_errors);
        rx_frame_err = ntohll(buffer->rx_frame_err);
        rx_over_err = ntohll(buffer->rx_over_err);
        rx_crc_err = ntohll(buffer->rx_crc_err);
        collisions = ntohll(buffer->collisions);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_port_stats& of_port_stats::operator=(const of_port_stats& peer_)
    {
        port_no = peer_.port_no;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        rx_packets = peer_.rx_packets;
        tx_packets = peer_.tx_packets;
        rx_bytes = peer_.rx_bytes;
        tx_bytes = peer_.tx_bytes;
        rx_dropped = peer_.rx_dropped;
        tx_dropped = peer_.tx_dropped;
        rx_errors = peer_.rx_errors;
        tx_errors = peer_.tx_errors;
        rx_frame_err = peer_.rx_frame_err;
        rx_over_err = peer_.rx_over_err;
        rx_crc_err = peer_.rx_crc_err;
        collisions = peer_.collisions;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_port_stats::operator==(const of_port_stats& peer_) const
    {
        return \
            (port_no == peer_.port_no) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]) &&\
            (rx_packets == peer_.rx_packets) &&\
            (tx_packets == peer_.tx_packets) &&\
            (rx_bytes == peer_.rx_bytes) &&\
            (tx_bytes == peer_.tx_bytes) &&\
            (rx_dropped == peer_.rx_dropped) &&\
            (tx_dropped == peer_.tx_dropped) &&\
            (rx_errors == peer_.rx_errors) &&\
            (tx_errors == peer_.tx_errors) &&\
            (rx_frame_err == peer_.rx_frame_err) &&\
            (rx_over_err == peer_.rx_over_err) &&\
            (rx_crc_err == peer_.rx_crc_err) &&\
            (collisions == peer_.collisions);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_stats::of_queue_stats()
    {
        port_no = 0;
        pad[0] = 0;
        pad[1] = 0;
        queue_id = 0;
        tx_bytes = 0;
        tx_packets = 0;
        tx_errors = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_stats::of_queue_stats(uint16_t port_no_, uint8_t pad_[], uint32_t queue_id_, uint64_t tx_bytes_, uint64_t tx_packets_, uint64_t tx_errors_)
    {
        port_no = port_no_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        queue_id = queue_id_;
        tx_bytes = tx_bytes_;
        tx_packets = tx_packets_;
        tx_errors = tx_errors_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_stats::pack(ofp_queue_stats* buffer)
    {
        buffer->port_no = htons(port_no);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->queue_id = htonl(queue_id);
        buffer->tx_bytes = htonll(tx_bytes);
        buffer->tx_packets = htonll(tx_packets);
        buffer->tx_errors = htonll(tx_errors);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_stats::unpack(ofp_queue_stats* buffer)
    {
        port_no = ntohs(buffer->port_no);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        queue_id = ntohl(buffer->queue_id);
        tx_bytes = ntohll(buffer->tx_bytes);
        tx_packets = ntohll(buffer->tx_packets);
        tx_errors = ntohll(buffer->tx_errors);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_stats& of_queue_stats::operator=(const of_queue_stats& peer_)
    {
        port_no = peer_.port_no;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        queue_id = peer_.queue_id;
        tx_bytes = peer_.tx_bytes;
        tx_packets = peer_.tx_packets;
        tx_errors = peer_.tx_errors;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_stats::operator==(const of_queue_stats& peer_) const
    {
        return \
            (port_no == peer_.port_no) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (queue_id == peer_.queue_id) &&\
            (tx_bytes == peer_.tx_bytes) &&\
            (tx_packets == peer_.tx_packets) &&\
            (tx_errors == peer_.tx_errors);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_tp_port::of_action_tp_port()
    {
        type = 0;
        len = 0;
        tp_port = 0;
        pad[0] = 0;
        pad[1] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_tp_port::of_action_tp_port(uint16_t type_, uint16_t len_, uint16_t tp_port_, uint8_t pad_[])
    {
        type = type_;
        len = len_;
        tp_port = tp_port_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_tp_port::pack(ofp_action_tp_port* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->tp_port = htons(tp_port);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_tp_port::unpack(ofp_action_tp_port* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        tp_port = ntohs(buffer->tp_port);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_tp_port& of_action_tp_port::operator=(const of_action_tp_port& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        tp_port = peer_.tp_port;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_tp_port::operator==(const of_action_tp_port& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (tp_port == peer_.tp_port) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_port_stats_request::of_port_stats_request()
    {
        port_no = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_port_stats_request::of_port_stats_request(uint16_t port_no_, uint8_t pad_[])
    {
        port_no = port_no_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_port_stats_request::pack(ofp_port_stats_request* buffer)
    {
        buffer->port_no = htons(port_no);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_port_stats_request::unpack(ofp_port_stats_request* buffer)
    {
        port_no = ntohs(buffer->port_no);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_port_stats_request& of_port_stats_request::operator=(const of_port_stats_request& peer_)
    {
        port_no = peer_.port_no;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_port_stats_request::operator==(const of_port_stats_request& peer_) const
    {
        return \
            (port_no == peer_.port_no) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_stats_request::of_stats_request()
    {
        (*this).header.type = OFPT_STATS_REQUEST;
        type = 0;
        flags = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_stats_request::of_stats_request(of_header header_, uint16_t type_, uint16_t flags_)
    {
        header = header_;
        type = type_;
        flags = flags_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_stats_request::pack(ofp_stats_request* buffer)
    {
        header.pack(&buffer->header);
        buffer->type = htons(type);
        buffer->flags = htons(flags);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_stats_request::unpack(ofp_stats_request* buffer)
    {
        header.unpack(&buffer->header);
        type = ntohs(buffer->type);
        flags = ntohs(buffer->flags);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_stats_request& of_stats_request::operator=(const of_stats_request& peer_)
    {
        header = peer_.header;
        type = peer_.type;
        flags = peer_.flags;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_stats_request::operator==(const of_stats_request& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (type == peer_.type) &&\
            (flags == peer_.flags);
    }


        /** \brief Empty constructor with default assigned
         */
    of_hello::of_hello()
    {
        (*this).header.type = OFPT_HELLO;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_hello::of_hello(of_header header_)
    {
        header = header_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_hello::pack(ofp_hello* buffer)
    {
        header.pack(&buffer->header);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_hello::unpack(ofp_hello* buffer)
    {
        header.unpack(&buffer->header);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_hello& of_hello::operator=(const of_hello& peer_)
    {
        header = peer_.header;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_hello::operator==(const of_hello& peer_) const
    {
        return \
            (header == peer_.header);
    }


        /** \brief Empty constructor with default assigned
         */
    of_aggregate_stats_request::of_aggregate_stats_request()
    {
        table_id = 0;
        pad = 0;
        out_port = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_aggregate_stats_request::of_aggregate_stats_request(of_match match_, uint8_t table_id_, uint8_t pad_, uint16_t out_port_)
    {
        match = match_;
        table_id = table_id_;
        pad = pad_;
        out_port = out_port_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_aggregate_stats_request::pack(ofp_aggregate_stats_request* buffer)
    {
        match.pack(&buffer->match);
        buffer->table_id = (table_id);
        buffer->pad = (pad);
        buffer->out_port = htons(out_port);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_aggregate_stats_request::unpack(ofp_aggregate_stats_request* buffer)
    {
        match.unpack(&buffer->match);
        table_id = (buffer->table_id);
        pad = (buffer->pad);
        out_port = ntohs(buffer->out_port);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_aggregate_stats_request& of_aggregate_stats_request::operator=(const of_aggregate_stats_request& peer_)
    {
        match = peer_.match;
        table_id = peer_.table_id;
        pad = peer_.pad;
        out_port = peer_.out_port;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_aggregate_stats_request::operator==(const of_aggregate_stats_request& peer_) const
    {
        return \
            (match == peer_.match) &&\
            (table_id == peer_.table_id) &&\
            (pad == peer_.pad) &&\
            (out_port == peer_.out_port);
    }


        /** \brief Empty constructor with default assigned
         */
    of_port_status::of_port_status()
    {
        (*this).header.type = OFPT_PORT_STATUS;
        reason = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
        pad[6] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_port_status::of_port_status(of_header header_, uint8_t reason_, uint8_t pad_[], of_phy_port desc_)
    {
        header = header_;
        reason = reason_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
        pad[6] = pad_[6];
        desc = desc_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_port_status::pack(ofp_port_status* buffer)
    {
        header.pack(&buffer->header);
        buffer->reason = (reason);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
        buffer->pad[6] = (pad[6]);
        desc.pack(&buffer->desc);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_port_status::unpack(ofp_port_status* buffer)
    {
        header.unpack(&buffer->header);
        reason = (buffer->reason);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
        pad[6] = (buffer->pad[6]);
        desc.unpack(&buffer->desc);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_port_status& of_port_status::operator=(const of_port_status& peer_)
    {
        header = peer_.header;
        reason = peer_.reason;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        pad[6] = peer_.pad[6];
        desc = peer_.desc;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_port_status::operator==(const of_port_status& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (reason == peer_.reason) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]) &&\
            (pad[6] == peer_.pad[6]) &&\
            (desc == peer_.desc);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_header::of_action_header()
    {
        type = 0;
        len = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_header::of_action_header(uint16_t type_, uint16_t len_, uint8_t pad_[])
    {
        type = type_;
        len = len_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_header::pack(ofp_action_header* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_header::unpack(ofp_action_header* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_header& of_action_header::operator=(const of_action_header& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_header::operator==(const of_action_header& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_port_mod::of_port_mod()
    {
        (*this).header.type = OFPT_PORT_MOD;
        port_no = 0;
        hw_addr[0] = 0;
        hw_addr[1] = 0;
        hw_addr[2] = 0;
        hw_addr[3] = 0;
        hw_addr[4] = 0;
        hw_addr[5] = 0;
        config = 0;
        mask = 0;
        advertise = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_port_mod::of_port_mod(of_header header_, uint16_t port_no_, uint8_t hw_addr_[], uint32_t config_, uint32_t mask_, uint32_t advertise_, uint8_t pad_[])
    {
        header = header_;
        port_no = port_no_;
        hw_addr[0] = hw_addr_[0];
        hw_addr[1] = hw_addr_[1];
        hw_addr[2] = hw_addr_[2];
        hw_addr[3] = hw_addr_[3];
        hw_addr[4] = hw_addr_[4];
        hw_addr[5] = hw_addr_[5];
        config = config_;
        mask = mask_;
        advertise = advertise_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_port_mod::pack(ofp_port_mod* buffer)
    {
        header.pack(&buffer->header);
        buffer->port_no = htons(port_no);
        buffer->hw_addr[0] = (hw_addr[0]);
        buffer->hw_addr[1] = (hw_addr[1]);
        buffer->hw_addr[2] = (hw_addr[2]);
        buffer->hw_addr[3] = (hw_addr[3]);
        buffer->hw_addr[4] = (hw_addr[4]);
        buffer->hw_addr[5] = (hw_addr[5]);
        buffer->config = htonl(config);
        buffer->mask = htonl(mask);
        buffer->advertise = htonl(advertise);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_port_mod::unpack(ofp_port_mod* buffer)
    {
        header.unpack(&buffer->header);
        port_no = ntohs(buffer->port_no);
        hw_addr[0] = (buffer->hw_addr[0]);
        hw_addr[1] = (buffer->hw_addr[1]);
        hw_addr[2] = (buffer->hw_addr[2]);
        hw_addr[3] = (buffer->hw_addr[3]);
        hw_addr[4] = (buffer->hw_addr[4]);
        hw_addr[5] = (buffer->hw_addr[5]);
        config = ntohl(buffer->config);
        mask = ntohl(buffer->mask);
        advertise = ntohl(buffer->advertise);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_port_mod& of_port_mod::operator=(const of_port_mod& peer_)
    {
        header = peer_.header;
        port_no = peer_.port_no;
        hw_addr[0] = peer_.hw_addr[0];
        hw_addr[1] = peer_.hw_addr[1];
        hw_addr[2] = peer_.hw_addr[2];
        hw_addr[3] = peer_.hw_addr[3];
        hw_addr[4] = peer_.hw_addr[4];
        hw_addr[5] = peer_.hw_addr[5];
        config = peer_.config;
        mask = peer_.mask;
        advertise = peer_.advertise;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_port_mod::operator==(const of_port_mod& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (port_no == peer_.port_no) &&\
            (hw_addr[0] == peer_.hw_addr[0]) &&\
            (hw_addr[1] == peer_.hw_addr[1]) &&\
            (hw_addr[2] == peer_.hw_addr[2]) &&\
            (hw_addr[3] == peer_.hw_addr[3]) &&\
            (hw_addr[4] == peer_.hw_addr[4]) &&\
            (hw_addr[5] == peer_.hw_addr[5]) &&\
            (config == peer_.config) &&\
            (mask == peer_.mask) &&\
            (advertise == peer_.advertise) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_vlan_vid::of_action_vlan_vid()
    {
        type = OFPAT_SET_VLAN_VID;
        len = 0;
        vlan_vid = 0;
        pad[0] = 0;
        pad[1] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_vlan_vid::of_action_vlan_vid(uint16_t type_, uint16_t len_, uint16_t vlan_vid_, uint8_t pad_[])
    {
        type = type_;
        len = len_;
        vlan_vid = vlan_vid_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_vlan_vid::pack(ofp_action_vlan_vid* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->vlan_vid = htons(vlan_vid);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_vlan_vid::unpack(ofp_action_vlan_vid* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        vlan_vid = ntohs(buffer->vlan_vid);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_vlan_vid& of_action_vlan_vid::operator=(const of_action_vlan_vid& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        vlan_vid = peer_.vlan_vid;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_vlan_vid::operator==(const of_action_vlan_vid& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (vlan_vid == peer_.vlan_vid) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_output::of_action_output()
    {
        type = OFPAT_OUTPUT;
        len = 0;
        port = 0;
        max_len = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_output::of_action_output(uint16_t type_, uint16_t len_, uint16_t port_, uint16_t max_len_)
    {
        type = type_;
        len = len_;
        port = port_;
        max_len = max_len_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_output::pack(ofp_action_output* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->port = htons(port);
        buffer->max_len = htons(max_len);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_output::unpack(ofp_action_output* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        port = ntohs(buffer->port);
        max_len = ntohs(buffer->max_len);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_output& of_action_output::operator=(const of_action_output& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        port = peer_.port;
        max_len = peer_.max_len;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_output::operator==(const of_action_output& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (port == peer_.port) &&\
            (max_len == peer_.max_len);
    }


        /** \brief Empty constructor with default assigned
         */
    of_switch_config::of_switch_config()
    {
        flags = 0;
        miss_send_len = OFP_DEFAULT_MISS_SEND_LEN;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_switch_config::of_switch_config(of_header header_, uint16_t flags_, uint16_t miss_send_len_)
    {
        header = header_;
        flags = flags_;
        miss_send_len = miss_send_len_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_switch_config::pack(ofp_switch_config* buffer)
    {
        header.pack(&buffer->header);
        buffer->flags = htons(flags);
        buffer->miss_send_len = htons(miss_send_len);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_switch_config::unpack(ofp_switch_config* buffer)
    {
        header.unpack(&buffer->header);
        flags = ntohs(buffer->flags);
        miss_send_len = ntohs(buffer->miss_send_len);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_switch_config& of_switch_config::operator=(const of_switch_config& peer_)
    {
        header = peer_.header;
        flags = peer_.flags;
        miss_send_len = peer_.miss_send_len;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_switch_config::operator==(const of_switch_config& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (flags == peer_.flags) &&\
            (miss_send_len == peer_.miss_send_len);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_nw_tos::of_action_nw_tos()
    {
        type = 0;
        len = 0;
        nw_tos = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_nw_tos::of_action_nw_tos(uint16_t type_, uint16_t len_, uint8_t nw_tos_, uint8_t pad_[])
    {
        type = type_;
        len = len_;
        nw_tos = nw_tos_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_nw_tos::pack(ofp_action_nw_tos* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->nw_tos = (nw_tos);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_nw_tos::unpack(ofp_action_nw_tos* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        nw_tos = (buffer->nw_tos);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_nw_tos& of_action_nw_tos::operator=(const of_action_nw_tos& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        nw_tos = peer_.nw_tos;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_nw_tos::operator==(const of_action_nw_tos& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (nw_tos == peer_.nw_tos) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_get_config_reply::of_queue_get_config_reply()
    {
        port = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_get_config_reply::of_queue_get_config_reply(of_header header_, uint16_t port_, uint8_t pad_[])
    {
        header = header_;
        port = port_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_get_config_reply::pack(ofp_queue_get_config_reply* buffer)
    {
        header.pack(&buffer->header);
        buffer->port = htons(port);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_get_config_reply::unpack(ofp_queue_get_config_reply* buffer)
    {
        header.unpack(&buffer->header);
        port = ntohs(buffer->port);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_get_config_reply& of_queue_get_config_reply::operator=(const of_queue_get_config_reply& peer_)
    {
        header = peer_.header;
        port = peer_.port;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_get_config_reply::operator==(const of_queue_get_config_reply& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (port == peer_.port) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_packet_in::of_packet_in()
    {
        (*this).header.type = OFPT_PACKET_IN;
        buffer_id = 0;
        total_len = 0;
        in_port = 0;
        reason = 0;
        pad = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_packet_in::of_packet_in(of_header header_, uint32_t buffer_id_, uint16_t total_len_, uint16_t in_port_, uint8_t reason_, uint8_t pad_)
    {
        header = header_;
        buffer_id = buffer_id_;
        total_len = total_len_;
        in_port = in_port_;
        reason = reason_;
        pad = pad_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_packet_in::pack(ofp_packet_in* buffer)
    {
        header.pack(&buffer->header);
        buffer->buffer_id = htonl(buffer_id);
        buffer->total_len = htons(total_len);
        buffer->in_port = htons(in_port);
        buffer->reason = (reason);
        buffer->pad = (pad);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_packet_in::unpack(ofp_packet_in* buffer)
    {
        header.unpack(&buffer->header);
        buffer_id = ntohl(buffer->buffer_id);
        total_len = ntohs(buffer->total_len);
        in_port = ntohs(buffer->in_port);
        reason = (buffer->reason);
        pad = (buffer->pad);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_packet_in& of_packet_in::operator=(const of_packet_in& peer_)
    {
        header = peer_.header;
        buffer_id = peer_.buffer_id;
        total_len = peer_.total_len;
        in_port = peer_.in_port;
        reason = peer_.reason;
        pad = peer_.pad;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_packet_in::operator==(const of_packet_in& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (buffer_id == peer_.buffer_id) &&\
            (total_len == peer_.total_len) &&\
            (in_port == peer_.in_port) &&\
            (reason == peer_.reason) &&\
            (pad == peer_.pad);
    }


        /** \brief Empty constructor with default assigned
         */
    of_flow_stats::of_flow_stats()
    {
        length = 0;
        table_id = 0;
        pad = 0;
        duration_sec = 0;
        duration_nsec = 0;
        priority = OFP_DEFAULT_PRIORITY;
        idle_timeout = 0;
        hard_timeout = 0;
        pad2[0] = 0;
        pad2[1] = 0;
        pad2[2] = 0;
        pad2[3] = 0;
        pad2[4] = 0;
        pad2[5] = 0;
        cookie = 0;
        packet_count = 0;
        byte_count = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_flow_stats::of_flow_stats(uint16_t length_, uint8_t table_id_, uint8_t pad_, of_match match_, uint32_t duration_sec_, uint32_t duration_nsec_, uint16_t priority_, uint16_t idle_timeout_, uint16_t hard_timeout_, uint8_t pad2_[], uint64_t cookie_, uint64_t packet_count_, uint64_t byte_count_)
    {
        length = length_;
        table_id = table_id_;
        pad = pad_;
        match = match_;
        duration_sec = duration_sec_;
        duration_nsec = duration_nsec_;
        priority = priority_;
        idle_timeout = idle_timeout_;
        hard_timeout = hard_timeout_;
        pad2[0] = pad2_[0];
        pad2[1] = pad2_[1];
        pad2[2] = pad2_[2];
        pad2[3] = pad2_[3];
        pad2[4] = pad2_[4];
        pad2[5] = pad2_[5];
        cookie = cookie_;
        packet_count = packet_count_;
        byte_count = byte_count_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_flow_stats::pack(ofp_flow_stats* buffer)
    {
        buffer->length = htons(length);
        buffer->table_id = (table_id);
        buffer->pad = (pad);
        match.pack(&buffer->match);
        buffer->duration_sec = htonl(duration_sec);
        buffer->duration_nsec = htonl(duration_nsec);
        buffer->priority = htons(priority);
        buffer->idle_timeout = htons(idle_timeout);
        buffer->hard_timeout = htons(hard_timeout);
        buffer->pad2[0] = (pad2[0]);
        buffer->pad2[1] = (pad2[1]);
        buffer->pad2[2] = (pad2[2]);
        buffer->pad2[3] = (pad2[3]);
        buffer->pad2[4] = (pad2[4]);
        buffer->pad2[5] = (pad2[5]);
        buffer->cookie = htonll(cookie);
        buffer->packet_count = htonll(packet_count);
        buffer->byte_count = htonll(byte_count);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_flow_stats::unpack(ofp_flow_stats* buffer)
    {
        length = ntohs(buffer->length);
        table_id = (buffer->table_id);
        pad = (buffer->pad);
        match.unpack(&buffer->match);
        duration_sec = ntohl(buffer->duration_sec);
        duration_nsec = ntohl(buffer->duration_nsec);
        priority = ntohs(buffer->priority);
        idle_timeout = ntohs(buffer->idle_timeout);
        hard_timeout = ntohs(buffer->hard_timeout);
        pad2[0] = (buffer->pad2[0]);
        pad2[1] = (buffer->pad2[1]);
        pad2[2] = (buffer->pad2[2]);
        pad2[3] = (buffer->pad2[3]);
        pad2[4] = (buffer->pad2[4]);
        pad2[5] = (buffer->pad2[5]);
        cookie = ntohll(buffer->cookie);
        packet_count = ntohll(buffer->packet_count);
        byte_count = ntohll(buffer->byte_count);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_flow_stats& of_flow_stats::operator=(const of_flow_stats& peer_)
    {
        length = peer_.length;
        table_id = peer_.table_id;
        pad = peer_.pad;
        match = peer_.match;
        duration_sec = peer_.duration_sec;
        duration_nsec = peer_.duration_nsec;
        priority = peer_.priority;
        idle_timeout = peer_.idle_timeout;
        hard_timeout = peer_.hard_timeout;
        pad2[0] = peer_.pad2[0];
        pad2[1] = peer_.pad2[1];
        pad2[2] = peer_.pad2[2];
        pad2[3] = peer_.pad2[3];
        pad2[4] = peer_.pad2[4];
        pad2[5] = peer_.pad2[5];
        cookie = peer_.cookie;
        packet_count = peer_.packet_count;
        byte_count = peer_.byte_count;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_flow_stats::operator==(const of_flow_stats& peer_) const
    {
        return \
            (length == peer_.length) &&\
            (table_id == peer_.table_id) &&\
            (pad == peer_.pad) &&\
            (match == peer_.match) &&\
            (duration_sec == peer_.duration_sec) &&\
            (duration_nsec == peer_.duration_nsec) &&\
            (priority == peer_.priority) &&\
            (idle_timeout == peer_.idle_timeout) &&\
            (hard_timeout == peer_.hard_timeout) &&\
            (pad2[0] == peer_.pad2[0]) &&\
            (pad2[1] == peer_.pad2[1]) &&\
            (pad2[2] == peer_.pad2[2]) &&\
            (pad2[3] == peer_.pad2[3]) &&\
            (pad2[4] == peer_.pad2[4]) &&\
            (pad2[5] == peer_.pad2[5]) &&\
            (cookie == peer_.cookie) &&\
            (packet_count == peer_.packet_count) &&\
            (byte_count == peer_.byte_count);
    }


        /** \brief Empty constructor with default assigned
         */
    of_flow_stats_request::of_flow_stats_request()
    {
        table_id = 0;
        pad = 0;
        out_port = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_flow_stats_request::of_flow_stats_request(of_match match_, uint8_t table_id_, uint8_t pad_, uint16_t out_port_)
    {
        match = match_;
        table_id = table_id_;
        pad = pad_;
        out_port = out_port_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_flow_stats_request::pack(ofp_flow_stats_request* buffer)
    {
        match.pack(&buffer->match);
        buffer->table_id = (table_id);
        buffer->pad = (pad);
        buffer->out_port = htons(out_port);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_flow_stats_request::unpack(ofp_flow_stats_request* buffer)
    {
        match.unpack(&buffer->match);
        table_id = (buffer->table_id);
        pad = (buffer->pad);
        out_port = ntohs(buffer->out_port);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_flow_stats_request& of_flow_stats_request::operator=(const of_flow_stats_request& peer_)
    {
        match = peer_.match;
        table_id = peer_.table_id;
        pad = peer_.pad;
        out_port = peer_.out_port;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_flow_stats_request::operator==(const of_flow_stats_request& peer_) const
    {
        return \
            (match == peer_.match) &&\
            (table_id == peer_.table_id) &&\
            (pad == peer_.pad) &&\
            (out_port == peer_.out_port);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_vendor_header::of_action_vendor_header()
    {
        type = OFPAT_VENDOR;
        len = 0;
        vendor = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_vendor_header::of_action_vendor_header(uint16_t type_, uint16_t len_, uint32_t vendor_)
    {
        type = type_;
        len = len_;
        vendor = vendor_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_vendor_header::pack(ofp_action_vendor_header* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->vendor = htonl(vendor);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_vendor_header::unpack(ofp_action_vendor_header* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        vendor = ntohl(buffer->vendor);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_vendor_header& of_action_vendor_header::operator=(const of_action_vendor_header& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        vendor = peer_.vendor;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_vendor_header::operator==(const of_action_vendor_header& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (vendor == peer_.vendor);
    }


        /** \brief Empty constructor with default assigned
         */
    of_stats_reply::of_stats_reply()
    {
        (*this).header.type = OFPT_STATS_REPLY;
        type = 0;
        flags = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_stats_reply::of_stats_reply(of_header header_, uint16_t type_, uint16_t flags_)
    {
        header = header_;
        type = type_;
        flags = flags_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_stats_reply::pack(ofp_stats_reply* buffer)
    {
        header.pack(&buffer->header);
        buffer->type = htons(type);
        buffer->flags = htons(flags);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_stats_reply::unpack(ofp_stats_reply* buffer)
    {
        header.unpack(&buffer->header);
        type = ntohs(buffer->type);
        flags = ntohs(buffer->flags);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_stats_reply& of_stats_reply::operator=(const of_stats_reply& peer_)
    {
        header = peer_.header;
        type = peer_.type;
        flags = peer_.flags;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_stats_reply::operator==(const of_stats_reply& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (type == peer_.type) &&\
            (flags == peer_.flags);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_stats_request::of_queue_stats_request()
    {
        port_no = 0;
        pad[0] = 0;
        pad[1] = 0;
        queue_id = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_stats_request::of_queue_stats_request(uint16_t port_no_, uint8_t pad_[], uint32_t queue_id_)
    {
        port_no = port_no_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        queue_id = queue_id_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_stats_request::pack(ofp_queue_stats_request* buffer)
    {
        buffer->port_no = htons(port_no);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->queue_id = htonl(queue_id);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_stats_request::unpack(ofp_queue_stats_request* buffer)
    {
        port_no = ntohs(buffer->port_no);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        queue_id = ntohl(buffer->queue_id);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_stats_request& of_queue_stats_request::operator=(const of_queue_stats_request& peer_)
    {
        port_no = peer_.port_no;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        queue_id = peer_.queue_id;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_stats_request::operator==(const of_queue_stats_request& peer_) const
    {
        return \
            (port_no == peer_.port_no) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (queue_id == peer_.queue_id);
    }


        /** \brief Empty constructor with default assigned
         */
    of_desc_stats::of_desc_stats()
    {
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_desc_stats::of_desc_stats(std::string mfr_desc_, std::string hw_desc_, std::string sw_desc_, std::string serial_num_, std::string dp_desc_)
    {
        mfr_desc = mfr_desc_;
        hw_desc = hw_desc_;
        sw_desc = sw_desc_;
        serial_num = serial_num_;
        dp_desc = dp_desc_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_desc_stats::pack(ofp_desc_stats* buffer)
    {
        strncpy(buffer->mfr_desc,mfr_desc.c_str(),255);
        buffer->mfr_desc[255] = '\0';
        strncpy(buffer->hw_desc,hw_desc.c_str(),255);
        buffer->hw_desc[255] = '\0';
        strncpy(buffer->sw_desc,sw_desc.c_str(),255);
        buffer->sw_desc[255] = '\0';
        strncpy(buffer->serial_num,serial_num.c_str(),31);
        buffer->serial_num[31] = '\0';
        strncpy(buffer->dp_desc,dp_desc.c_str(),255);
        buffer->dp_desc[255] = '\0';
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_desc_stats::unpack(ofp_desc_stats* buffer)
    {
        mfr_desc.assign(buffer->mfr_desc);
        hw_desc.assign(buffer->hw_desc);
        sw_desc.assign(buffer->sw_desc);
        serial_num.assign(buffer->serial_num);
        dp_desc.assign(buffer->dp_desc);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_desc_stats& of_desc_stats::operator=(const of_desc_stats& peer_)
    {
        mfr_desc = peer_.mfr_desc;
        hw_desc = peer_.hw_desc;
        sw_desc = peer_.sw_desc;
        serial_num = peer_.serial_num;
        dp_desc = peer_.dp_desc;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_desc_stats::operator==(const of_desc_stats& peer_) const
    {
        return \
            (mfr_desc == peer_.mfr_desc) &&\
            (hw_desc == peer_.hw_desc) &&\
            (sw_desc == peer_.sw_desc) &&\
            (serial_num == peer_.serial_num) &&\
            (dp_desc == peer_.dp_desc);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_get_config_request::of_queue_get_config_request()
    {
        port = 0;
        pad[0] = 0;
        pad[1] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_get_config_request::of_queue_get_config_request(of_header header_, uint16_t port_, uint8_t pad_[])
    {
        header = header_;
        port = port_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_get_config_request::pack(ofp_queue_get_config_request* buffer)
    {
        header.pack(&buffer->header);
        buffer->port = htons(port);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_get_config_request::unpack(ofp_queue_get_config_request* buffer)
    {
        header.unpack(&buffer->header);
        port = ntohs(buffer->port);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_get_config_request& of_queue_get_config_request::operator=(const of_queue_get_config_request& peer_)
    {
        header = peer_.header;
        port = peer_.port;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_get_config_request::operator==(const of_queue_get_config_request& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (port == peer_.port) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_packet_queue::of_packet_queue()
    {
        queue_id = 0;
        len = 0;
        pad[0] = 0;
        pad[1] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_packet_queue::of_packet_queue(uint32_t queue_id_, uint16_t len_, uint8_t pad_[])
    {
        queue_id = queue_id_;
        len = len_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_packet_queue::pack(ofp_packet_queue* buffer)
    {
        buffer->queue_id = htonl(queue_id);
        buffer->len = htons(len);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_packet_queue::unpack(ofp_packet_queue* buffer)
    {
        queue_id = ntohl(buffer->queue_id);
        len = ntohs(buffer->len);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_packet_queue& of_packet_queue::operator=(const of_packet_queue& peer_)
    {
        queue_id = peer_.queue_id;
        len = peer_.len;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_packet_queue::operator==(const of_packet_queue& peer_) const
    {
        return \
            (queue_id == peer_.queue_id) &&\
            (len == peer_.len) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_dl_addr::of_action_dl_addr()
    {
        type = 0;
        len = 0;
        dl_addr[0] = 0;
        dl_addr[1] = 0;
        dl_addr[2] = 0;
        dl_addr[3] = 0;
        dl_addr[4] = 0;
        dl_addr[5] = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_dl_addr::of_action_dl_addr(uint16_t type_, uint16_t len_, uint8_t dl_addr_[], uint8_t pad_[])
    {
        type = type_;
        len = len_;
        dl_addr[0] = dl_addr_[0];
        dl_addr[1] = dl_addr_[1];
        dl_addr[2] = dl_addr_[2];
        dl_addr[3] = dl_addr_[3];
        dl_addr[4] = dl_addr_[4];
        dl_addr[5] = dl_addr_[5];
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_dl_addr::pack(ofp_action_dl_addr* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->dl_addr[0] = (dl_addr[0]);
        buffer->dl_addr[1] = (dl_addr[1]);
        buffer->dl_addr[2] = (dl_addr[2]);
        buffer->dl_addr[3] = (dl_addr[3]);
        buffer->dl_addr[4] = (dl_addr[4]);
        buffer->dl_addr[5] = (dl_addr[5]);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_dl_addr::unpack(ofp_action_dl_addr* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        dl_addr[0] = (buffer->dl_addr[0]);
        dl_addr[1] = (buffer->dl_addr[1]);
        dl_addr[2] = (buffer->dl_addr[2]);
        dl_addr[3] = (buffer->dl_addr[3]);
        dl_addr[4] = (buffer->dl_addr[4]);
        dl_addr[5] = (buffer->dl_addr[5]);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_dl_addr& of_action_dl_addr::operator=(const of_action_dl_addr& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        dl_addr[0] = peer_.dl_addr[0];
        dl_addr[1] = peer_.dl_addr[1];
        dl_addr[2] = peer_.dl_addr[2];
        dl_addr[3] = peer_.dl_addr[3];
        dl_addr[4] = peer_.dl_addr[4];
        dl_addr[5] = peer_.dl_addr[5];
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_dl_addr::operator==(const of_action_dl_addr& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (dl_addr[0] == peer_.dl_addr[0]) &&\
            (dl_addr[1] == peer_.dl_addr[1]) &&\
            (dl_addr[2] == peer_.dl_addr[2]) &&\
            (dl_addr[3] == peer_.dl_addr[3]) &&\
            (dl_addr[4] == peer_.dl_addr[4]) &&\
            (dl_addr[5] == peer_.dl_addr[5]) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_prop_header::of_queue_prop_header()
    {
        property = 0;
        len = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_prop_header::of_queue_prop_header(uint16_t property_, uint16_t len_, uint8_t pad_[])
    {
        property = property_;
        len = len_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_prop_header::pack(ofp_queue_prop_header* buffer)
    {
        buffer->property = htons(property);
        buffer->len = htons(len);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_prop_header::unpack(ofp_queue_prop_header* buffer)
    {
        property = ntohs(buffer->property);
        len = ntohs(buffer->len);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_prop_header& of_queue_prop_header::operator=(const of_queue_prop_header& peer_)
    {
        property = peer_.property;
        len = peer_.len;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_prop_header::operator==(const of_queue_prop_header& peer_) const
    {
        return \
            (property == peer_.property) &&\
            (len == peer_.len) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_queue_prop_min_rate::of_queue_prop_min_rate()
    {
        rate = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_queue_prop_min_rate::of_queue_prop_min_rate(of_queue_prop_header prop_header_, uint16_t rate_, uint8_t pad_[])
    {
        prop_header = prop_header_;
        rate = rate_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_queue_prop_min_rate::pack(ofp_queue_prop_min_rate* buffer)
    {
        prop_header.pack(&buffer->prop_header);
        buffer->rate = htons(rate);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_queue_prop_min_rate::unpack(ofp_queue_prop_min_rate* buffer)
    {
        prop_header.unpack(&buffer->prop_header);
        rate = ntohs(buffer->rate);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_queue_prop_min_rate& of_queue_prop_min_rate::operator=(const of_queue_prop_min_rate& peer_)
    {
        prop_header = peer_.prop_header;
        rate = peer_.rate;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_queue_prop_min_rate::operator==(const of_queue_prop_min_rate& peer_) const
    {
        return \
            (prop_header == peer_.prop_header) &&\
            (rate == peer_.rate) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_enqueue::of_action_enqueue()
    {
        type = 0;
        len = 0;
        port = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        pad[3] = 0;
        pad[4] = 0;
        pad[5] = 0;
        queue_id = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_enqueue::of_action_enqueue(uint16_t type_, uint16_t len_, uint16_t port_, uint8_t pad_[], uint32_t queue_id_)
    {
        type = type_;
        len = len_;
        port = port_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        pad[3] = pad_[3];
        pad[4] = pad_[4];
        pad[5] = pad_[5];
        queue_id = queue_id_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_enqueue::pack(ofp_action_enqueue* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->port = htons(port);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->pad[3] = (pad[3]);
        buffer->pad[4] = (pad[4]);
        buffer->pad[5] = (pad[5]);
        buffer->queue_id = htonl(queue_id);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_enqueue::unpack(ofp_action_enqueue* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        port = ntohs(buffer->port);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        pad[3] = (buffer->pad[3]);
        pad[4] = (buffer->pad[4]);
        pad[5] = (buffer->pad[5]);
        queue_id = ntohl(buffer->queue_id);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_enqueue& of_action_enqueue::operator=(const of_action_enqueue& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        port = peer_.port;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        pad[3] = peer_.pad[3];
        pad[4] = peer_.pad[4];
        pad[5] = peer_.pad[5];
        queue_id = peer_.queue_id;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_enqueue::operator==(const of_action_enqueue& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (port == peer_.port) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (pad[3] == peer_.pad[3]) &&\
            (pad[4] == peer_.pad[4]) &&\
            (pad[5] == peer_.pad[5]) &&\
            (queue_id == peer_.queue_id);
    }


        /** \brief Empty constructor with default assigned
         */
    of_switch_features::of_switch_features()
    {
        (*this).header.type = OFPT_FEATURES_REPLY;
        datapath_id = 0;
        n_buffers = 0;
        n_tables = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        capabilities = 0;
        actions = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_switch_features::of_switch_features(of_header header_, uint64_t datapath_id_, uint32_t n_buffers_, uint8_t n_tables_, uint8_t pad_[], uint32_t capabilities_, uint32_t actions_)
    {
        header = header_;
        datapath_id = datapath_id_;
        n_buffers = n_buffers_;
        n_tables = n_tables_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
        capabilities = capabilities_;
        actions = actions_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_switch_features::pack(ofp_switch_features* buffer)
    {
        header.pack(&buffer->header);
        buffer->datapath_id = htonll(datapath_id);
        buffer->n_buffers = htonl(n_buffers);
        buffer->n_tables = (n_tables);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
        buffer->capabilities = htonl(capabilities);
        buffer->actions = htonl(actions);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_switch_features::unpack(ofp_switch_features* buffer)
    {
        header.unpack(&buffer->header);
        datapath_id = ntohll(buffer->datapath_id);
        n_buffers = ntohl(buffer->n_buffers);
        n_tables = (buffer->n_tables);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
        capabilities = ntohl(buffer->capabilities);
        actions = ntohl(buffer->actions);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_switch_features& of_switch_features::operator=(const of_switch_features& peer_)
    {
        header = peer_.header;
        datapath_id = peer_.datapath_id;
        n_buffers = peer_.n_buffers;
        n_tables = peer_.n_tables;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        capabilities = peer_.capabilities;
        actions = peer_.actions;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_switch_features::operator==(const of_switch_features& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (datapath_id == peer_.datapath_id) &&\
            (n_buffers == peer_.n_buffers) &&\
            (n_tables == peer_.n_tables) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]) &&\
            (capabilities == peer_.capabilities) &&\
            (actions == peer_.actions);
    }


        /** \brief Empty constructor with default assigned
         */
    of_match::of_match()
    {
        wildcards = 0;
        in_port = 0;
        dl_src[0] = 0;
        dl_src[1] = 0;
        dl_src[2] = 0;
        dl_src[3] = 0;
        dl_src[4] = 0;
        dl_src[5] = 0;
        dl_dst[0] = 0;
        dl_dst[1] = 0;
        dl_dst[2] = 0;
        dl_dst[3] = 0;
        dl_dst[4] = 0;
        dl_dst[5] = 0;
        dl_vlan = 0;
        dl_vlan_pcp = 0;
        pad1[0] = 0;
        dl_type = 0;
        nw_tos = 0;
        nw_proto = 0;
        pad2[0] = 0;
        pad2[1] = 0;
        nw_src = 0;
        nw_dst = 0;
        tp_src = 0;
        tp_dst = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_match::of_match(uint32_t wildcards_, uint16_t in_port_, uint8_t dl_src_[], uint8_t dl_dst_[], uint16_t dl_vlan_, uint8_t dl_vlan_pcp_, uint8_t pad1_[], uint16_t dl_type_, uint8_t nw_tos_, uint8_t nw_proto_, uint8_t pad2_[], uint32_t nw_src_, uint32_t nw_dst_, uint16_t tp_src_, uint16_t tp_dst_)
    {
        wildcards = wildcards_;
        in_port = in_port_;
        dl_src[0] = dl_src_[0];
        dl_src[1] = dl_src_[1];
        dl_src[2] = dl_src_[2];
        dl_src[3] = dl_src_[3];
        dl_src[4] = dl_src_[4];
        dl_src[5] = dl_src_[5];
        dl_dst[0] = dl_dst_[0];
        dl_dst[1] = dl_dst_[1];
        dl_dst[2] = dl_dst_[2];
        dl_dst[3] = dl_dst_[3];
        dl_dst[4] = dl_dst_[4];
        dl_dst[5] = dl_dst_[5];
        dl_vlan = dl_vlan_;
        dl_vlan_pcp = dl_vlan_pcp_;
        pad1[0] = pad1_[0];
        dl_type = dl_type_;
        nw_tos = nw_tos_;
        nw_proto = nw_proto_;
        pad2[0] = pad2_[0];
        pad2[1] = pad2_[1];
        nw_src = nw_src_;
        nw_dst = nw_dst_;
        tp_src = tp_src_;
        tp_dst = tp_dst_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_match::pack(ofp_match* buffer)
    {
        buffer->wildcards = htonl(wildcards);
        buffer->in_port = htons(in_port);
        buffer->dl_src[0] = (dl_src[0]);
        buffer->dl_src[1] = (dl_src[1]);
        buffer->dl_src[2] = (dl_src[2]);
        buffer->dl_src[3] = (dl_src[3]);
        buffer->dl_src[4] = (dl_src[4]);
        buffer->dl_src[5] = (dl_src[5]);
        buffer->dl_dst[0] = (dl_dst[0]);
        buffer->dl_dst[1] = (dl_dst[1]);
        buffer->dl_dst[2] = (dl_dst[2]);
        buffer->dl_dst[3] = (dl_dst[3]);
        buffer->dl_dst[4] = (dl_dst[4]);
        buffer->dl_dst[5] = (dl_dst[5]);
        buffer->dl_vlan = htons(dl_vlan);
        buffer->dl_vlan_pcp = (dl_vlan_pcp);
        buffer->pad1[0] = (pad1[0]);
        buffer->dl_type = htons(dl_type);
        buffer->nw_tos = (nw_tos);
        buffer->nw_proto = (nw_proto);
        buffer->pad2[0] = (pad2[0]);
        buffer->pad2[1] = (pad2[1]);
        buffer->nw_src = htonl(nw_src);
        buffer->nw_dst = htonl(nw_dst);
        buffer->tp_src = htons(tp_src);
        buffer->tp_dst = htons(tp_dst);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_match::unpack(ofp_match* buffer)
    {
        wildcards = ntohl(buffer->wildcards);
        in_port = ntohs(buffer->in_port);
        dl_src[0] = (buffer->dl_src[0]);
        dl_src[1] = (buffer->dl_src[1]);
        dl_src[2] = (buffer->dl_src[2]);
        dl_src[3] = (buffer->dl_src[3]);
        dl_src[4] = (buffer->dl_src[4]);
        dl_src[5] = (buffer->dl_src[5]);
        dl_dst[0] = (buffer->dl_dst[0]);
        dl_dst[1] = (buffer->dl_dst[1]);
        dl_dst[2] = (buffer->dl_dst[2]);
        dl_dst[3] = (buffer->dl_dst[3]);
        dl_dst[4] = (buffer->dl_dst[4]);
        dl_dst[5] = (buffer->dl_dst[5]);
        dl_vlan = ntohs(buffer->dl_vlan);
        dl_vlan_pcp = (buffer->dl_vlan_pcp);
        pad1[0] = (buffer->pad1[0]);
        dl_type = ntohs(buffer->dl_type);
        nw_tos = (buffer->nw_tos);
        nw_proto = (buffer->nw_proto);
        pad2[0] = (buffer->pad2[0]);
        pad2[1] = (buffer->pad2[1]);
        nw_src = ntohl(buffer->nw_src);
        nw_dst = ntohl(buffer->nw_dst);
        tp_src = ntohs(buffer->tp_src);
        tp_dst = ntohs(buffer->tp_dst);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_match& of_match::operator=(const of_match& peer_)
    {
        wildcards = peer_.wildcards;
        in_port = peer_.in_port;
        dl_src[0] = peer_.dl_src[0];
        dl_src[1] = peer_.dl_src[1];
        dl_src[2] = peer_.dl_src[2];
        dl_src[3] = peer_.dl_src[3];
        dl_src[4] = peer_.dl_src[4];
        dl_src[5] = peer_.dl_src[5];
        dl_dst[0] = peer_.dl_dst[0];
        dl_dst[1] = peer_.dl_dst[1];
        dl_dst[2] = peer_.dl_dst[2];
        dl_dst[3] = peer_.dl_dst[3];
        dl_dst[4] = peer_.dl_dst[4];
        dl_dst[5] = peer_.dl_dst[5];
        dl_vlan = peer_.dl_vlan;
        dl_vlan_pcp = peer_.dl_vlan_pcp;
        pad1[0] = peer_.pad1[0];
        dl_type = peer_.dl_type;
        nw_tos = peer_.nw_tos;
        nw_proto = peer_.nw_proto;
        pad2[0] = peer_.pad2[0];
        pad2[1] = peer_.pad2[1];
        nw_src = peer_.nw_src;
        nw_dst = peer_.nw_dst;
        tp_src = peer_.tp_src;
        tp_dst = peer_.tp_dst;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_match::operator==(const of_match& peer_) const
    {
        return \
            (wildcards == peer_.wildcards) &&\
            (in_port == peer_.in_port) &&\
            (dl_src[0] == peer_.dl_src[0]) &&\
            (dl_src[1] == peer_.dl_src[1]) &&\
            (dl_src[2] == peer_.dl_src[2]) &&\
            (dl_src[3] == peer_.dl_src[3]) &&\
            (dl_src[4] == peer_.dl_src[4]) &&\
            (dl_src[5] == peer_.dl_src[5]) &&\
            (dl_dst[0] == peer_.dl_dst[0]) &&\
            (dl_dst[1] == peer_.dl_dst[1]) &&\
            (dl_dst[2] == peer_.dl_dst[2]) &&\
            (dl_dst[3] == peer_.dl_dst[3]) &&\
            (dl_dst[4] == peer_.dl_dst[4]) &&\
            (dl_dst[5] == peer_.dl_dst[5]) &&\
            (dl_vlan == peer_.dl_vlan) &&\
            (dl_vlan_pcp == peer_.dl_vlan_pcp) &&\
            (pad1[0] == peer_.pad1[0]) &&\
            (dl_type == peer_.dl_type) &&\
            (nw_tos == peer_.nw_tos) &&\
            (nw_proto == peer_.nw_proto) &&\
            (pad2[0] == peer_.pad2[0]) &&\
            (pad2[1] == peer_.pad2[1]) &&\
            (nw_src == peer_.nw_src) &&\
            (nw_dst == peer_.nw_dst) &&\
            (tp_src == peer_.tp_src) &&\
            (tp_dst == peer_.tp_dst);
    }


        /** \brief Empty constructor with default assigned
         */
    of_header::of_header()
    {
        version = OFP_VERSION;
        type = 0;
        length = 0;
        xid = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_header::of_header(uint8_t version_, uint8_t type_, uint16_t length_, uint32_t xid_)
    {
        version = version_;
        type = type_;
        length = length_;
        xid = xid_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_header::pack(ofp_header* buffer)
    {
        buffer->version = (version);
        buffer->type = (type);
        buffer->length = htons(length);
        buffer->xid = htonl(xid);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_header::unpack(ofp_header* buffer)
    {
        version = (buffer->version);
        type = (buffer->type);
        length = ntohs(buffer->length);
        xid = ntohl(buffer->xid);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_header& of_header::operator=(const of_header& peer_)
    {
        version = peer_.version;
        type = peer_.type;
        length = peer_.length;
        xid = peer_.xid;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_header::operator==(const of_header& peer_) const
    {
        return \
            (version == peer_.version) &&\
            (type == peer_.type) &&\
            (length == peer_.length) &&\
            (xid == peer_.xid);
    }


        /** \brief Empty constructor with default assigned
         */
    of_vendor_header::of_vendor_header()
    {
        (*this).header.type = OFPT_VENDOR;
        vendor = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_vendor_header::of_vendor_header(of_header header_, uint32_t vendor_)
    {
        header = header_;
        vendor = vendor_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_vendor_header::pack(ofp_vendor_header* buffer)
    {
        header.pack(&buffer->header);
        buffer->vendor = htonl(vendor);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_vendor_header::unpack(ofp_vendor_header* buffer)
    {
        header.unpack(&buffer->header);
        vendor = ntohl(buffer->vendor);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_vendor_header& of_vendor_header::operator=(const of_vendor_header& peer_)
    {
        header = peer_.header;
        vendor = peer_.vendor;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_vendor_header::operator==(const of_vendor_header& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (vendor == peer_.vendor);
    }


        /** \brief Empty constructor with default assigned
         */
    of_packet_out::of_packet_out()
    {
        (*this).header.type = OFPT_PACKET_OUT;
        buffer_id = 4294967295;
        in_port = 0;
        actions_len = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_packet_out::of_packet_out(of_header header_, uint32_t buffer_id_, uint16_t in_port_, uint16_t actions_len_)
    {
        header = header_;
        buffer_id = buffer_id_;
        in_port = in_port_;
        actions_len = actions_len_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_packet_out::pack(ofp_packet_out* buffer)
    {
        header.pack(&buffer->header);
        buffer->buffer_id = htonl(buffer_id);
        buffer->in_port = htons(in_port);
        buffer->actions_len = htons(actions_len);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_packet_out::unpack(ofp_packet_out* buffer)
    {
        header.unpack(&buffer->header);
        buffer_id = ntohl(buffer->buffer_id);
        in_port = ntohs(buffer->in_port);
        actions_len = ntohs(buffer->actions_len);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_packet_out& of_packet_out::operator=(const of_packet_out& peer_)
    {
        header = peer_.header;
        buffer_id = peer_.buffer_id;
        in_port = peer_.in_port;
        actions_len = peer_.actions_len;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_packet_out::operator==(const of_packet_out& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (buffer_id == peer_.buffer_id) &&\
            (in_port == peer_.in_port) &&\
            (actions_len == peer_.actions_len);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_nw_addr::of_action_nw_addr()
    {
        type = 0;
        len = 0;
        nw_addr = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_nw_addr::of_action_nw_addr(uint16_t type_, uint16_t len_, uint32_t nw_addr_)
    {
        type = type_;
        len = len_;
        nw_addr = nw_addr_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_nw_addr::pack(ofp_action_nw_addr* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->nw_addr = htonl(nw_addr);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_nw_addr::unpack(ofp_action_nw_addr* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        nw_addr = ntohl(buffer->nw_addr);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_nw_addr& of_action_nw_addr::operator=(const of_action_nw_addr& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        nw_addr = peer_.nw_addr;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_nw_addr::operator==(const of_action_nw_addr& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (nw_addr == peer_.nw_addr);
    }


        /** \brief Empty constructor with default assigned
         */
    of_action_vlan_pcp::of_action_vlan_pcp()
    {
        type = OFPAT_SET_VLAN_PCP;
        len = 0;
        vlan_pcp = 0;
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_action_vlan_pcp::of_action_vlan_pcp(uint16_t type_, uint16_t len_, uint8_t vlan_pcp_, uint8_t pad_[])
    {
        type = type_;
        len = len_;
        vlan_pcp = vlan_pcp_;
        pad[0] = pad_[0];
        pad[1] = pad_[1];
        pad[2] = pad_[2];
    }

        /** \brief Pack OpenFlow messages
         */
    void of_action_vlan_pcp::pack(ofp_action_vlan_pcp* buffer)
    {
        buffer->type = htons(type);
        buffer->len = htons(len);
        buffer->vlan_pcp = (vlan_pcp);
        buffer->pad[0] = (pad[0]);
        buffer->pad[1] = (pad[1]);
        buffer->pad[2] = (pad[2]);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_action_vlan_pcp::unpack(ofp_action_vlan_pcp* buffer)
    {
        type = ntohs(buffer->type);
        len = ntohs(buffer->len);
        vlan_pcp = (buffer->vlan_pcp);
        pad[0] = (buffer->pad[0]);
        pad[1] = (buffer->pad[1]);
        pad[2] = (buffer->pad[2]);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_action_vlan_pcp& of_action_vlan_pcp::operator=(const of_action_vlan_pcp& peer_)
    {
        type = peer_.type;
        len = peer_.len;
        vlan_pcp = peer_.vlan_pcp;
        pad[0] = peer_.pad[0];
        pad[1] = peer_.pad[1];
        pad[2] = peer_.pad[2];
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_action_vlan_pcp::operator==(const of_action_vlan_pcp& peer_) const
    {
        return \
            (type == peer_.type) &&\
            (len == peer_.len) &&\
            (vlan_pcp == peer_.vlan_pcp) &&\
            (pad[0] == peer_.pad[0]) &&\
            (pad[1] == peer_.pad[1]) &&\
            (pad[2] == peer_.pad[2]);
    }


        /** \brief Empty constructor with default assigned
         */
    of_flow_mod::of_flow_mod()
    {
        (*this).header.type = OFPT_FLOW_MOD;
        cookie = 0;
        command = 0;
        idle_timeout = 0;
        hard_timeout = 0;
        priority = OFP_DEFAULT_PRIORITY;
        buffer_id = 0;
        out_port = 0;
        flags = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_flow_mod::of_flow_mod(of_header header_, of_match match_, uint64_t cookie_, uint16_t command_, uint16_t idle_timeout_, uint16_t hard_timeout_, uint16_t priority_, uint32_t buffer_id_, uint16_t out_port_, uint16_t flags_)
    {
        header = header_;
        match = match_;
        cookie = cookie_;
        command = command_;
        idle_timeout = idle_timeout_;
        hard_timeout = hard_timeout_;
        priority = priority_;
        buffer_id = buffer_id_;
        out_port = out_port_;
        flags = flags_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_flow_mod::pack(ofp_flow_mod* buffer)
    {
        header.pack(&buffer->header);
        match.pack(&buffer->match);
        buffer->cookie = htonll(cookie);
        buffer->command = htons(command);
        buffer->idle_timeout = htons(idle_timeout);
        buffer->hard_timeout = htons(hard_timeout);
        buffer->priority = htons(priority);
        buffer->buffer_id = htonl(buffer_id);
        buffer->out_port = htons(out_port);
        buffer->flags = htons(flags);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_flow_mod::unpack(ofp_flow_mod* buffer)
    {
        header.unpack(&buffer->header);
        match.unpack(&buffer->match);
        cookie = ntohll(buffer->cookie);
        command = ntohs(buffer->command);
        idle_timeout = ntohs(buffer->idle_timeout);
        hard_timeout = ntohs(buffer->hard_timeout);
        priority = ntohs(buffer->priority);
        buffer_id = ntohl(buffer->buffer_id);
        out_port = ntohs(buffer->out_port);
        flags = ntohs(buffer->flags);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_flow_mod& of_flow_mod::operator=(const of_flow_mod& peer_)
    {
        header = peer_.header;
        match = peer_.match;
        cookie = peer_.cookie;
        command = peer_.command;
        idle_timeout = peer_.idle_timeout;
        hard_timeout = peer_.hard_timeout;
        priority = peer_.priority;
        buffer_id = peer_.buffer_id;
        out_port = peer_.out_port;
        flags = peer_.flags;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_flow_mod::operator==(const of_flow_mod& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (match == peer_.match) &&\
            (cookie == peer_.cookie) &&\
            (command == peer_.command) &&\
            (idle_timeout == peer_.idle_timeout) &&\
            (hard_timeout == peer_.hard_timeout) &&\
            (priority == peer_.priority) &&\
            (buffer_id == peer_.buffer_id) &&\
            (out_port == peer_.out_port) &&\
            (flags == peer_.flags);
    }


        /** \brief Empty constructor with default assigned
         */
    of_error_msg::of_error_msg()
    {
        (*this).header.type = OFPT_ERROR;
        type = 0;
        code = 0;
    }

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
    of_error_msg::of_error_msg(of_header header_, uint16_t type_, uint16_t code_)
    {
        header = header_;
        type = type_;
        code = code_;
    }

        /** \brief Pack OpenFlow messages
         */
    void of_error_msg::pack(ofp_error_msg* buffer)
    {
        header.pack(&buffer->header);
        buffer->type = htons(type);
        buffer->code = htons(code);
    }

        /** \brief Unpack OpenFlow messages
         */
    void of_error_msg::unpack(ofp_error_msg* buffer)
    {
        header.unpack(&buffer->header);
        type = ntohs(buffer->type);
        code = ntohs(buffer->code);
    }

        /** \brief Overload assignment (for cloning)
         */
    of_error_msg& of_error_msg::operator=(const of_error_msg& peer_)
    {
        header = peer_.header;
        type = peer_.type;
        code = peer_.code;
        return *this;
    }

        /** \brief Overload equivalent (for comparison)
         */
    bool of_error_msg::operator==(const of_error_msg& peer_) const
    {
        return \
            (header == peer_.header) &&\
            (type == peer_.type) &&\
            (code == peer_.code);
    }

}

