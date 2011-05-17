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
#include "openflow/openflow.h"
#include <string>

#ifndef OPENFLOW_PACK_RAW_H
#define OPENFLOW_PACK_RAW_H

namespace vigil
{
    /** \brief Object wrapper for struct ofp_phy_port
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_phy_port
        : public ofp_phy_port
    {
    public:
        std::string name;

        /** \brief Empty constructor with default assigned
         */
        of_phy_port();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_phy_port(uint16_t port_no_, uint8_t hw_addr_[], std::string name_, uint32_t config_, uint32_t state_, uint32_t curr_, uint32_t advertised_, uint32_t supported_, uint32_t peer_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_phy_port* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_phy_port* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_phy_port& operator=(const of_phy_port& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_phy_port& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_aggregate_stats_reply
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_aggregate_stats_reply
        : public ofp_aggregate_stats_reply
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_aggregate_stats_reply();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_aggregate_stats_reply(uint64_t packet_count_, uint64_t byte_count_, uint32_t flow_count_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_aggregate_stats_reply* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_aggregate_stats_reply* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_aggregate_stats_reply& operator=(const of_aggregate_stats_reply& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_aggregate_stats_reply& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_table_stats
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_table_stats
        : public ofp_table_stats
    {
    public:
        std::string name;

        /** \brief Empty constructor with default assigned
         */
        of_table_stats();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_table_stats(uint8_t table_id_, uint8_t pad_[], std::string name_, uint32_t wildcards_, uint32_t max_entries_, uint32_t active_count_, uint64_t lookup_count_, uint64_t matched_count_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_table_stats* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_table_stats* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_table_stats& operator=(const of_table_stats& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_table_stats& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_port_stats
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_port_stats
        : public ofp_port_stats
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_port_stats();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_port_stats(uint16_t port_no_, uint8_t pad_[], uint64_t rx_packets_, uint64_t tx_packets_, uint64_t rx_bytes_, uint64_t tx_bytes_, uint64_t rx_dropped_, uint64_t tx_dropped_, uint64_t rx_errors_, uint64_t tx_errors_, uint64_t rx_frame_err_, uint64_t rx_over_err_, uint64_t rx_crc_err_, uint64_t collisions_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_port_stats* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_port_stats* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_port_stats& operator=(const of_port_stats& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_port_stats& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_queue_stats
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_stats
        : public ofp_queue_stats
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_queue_stats();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_stats(uint16_t port_no_, uint8_t pad_[], uint32_t queue_id_, uint64_t tx_bytes_, uint64_t tx_packets_, uint64_t tx_errors_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_stats* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_stats* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_stats& operator=(const of_queue_stats& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_stats& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_tp_port
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_tp_port
        : public ofp_action_tp_port
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_tp_port();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_tp_port(uint16_t type_, uint16_t len_, uint16_t tp_port_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_tp_port* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_tp_port* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_tp_port& operator=(const of_action_tp_port& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_tp_port& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_port_stats_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_port_stats_request
        : public ofp_port_stats_request
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_port_stats_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_port_stats_request(uint16_t port_no_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_port_stats_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_port_stats_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_port_stats_request& operator=(const of_port_stats_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_port_stats_request& peer_) const;

    private:
    };





    /** \brief Object wrapper for struct ofp_action_header
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_header
        : public ofp_action_header
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_header();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_header(uint16_t type_, uint16_t len_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_header* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_header* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_header& operator=(const of_action_header& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_header& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_action_vlan_vid
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_vlan_vid
        : public ofp_action_vlan_vid
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_vlan_vid();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_vlan_vid(uint16_t type_, uint16_t len_, uint16_t vlan_vid_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_vlan_vid* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_vlan_vid* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_vlan_vid& operator=(const of_action_vlan_vid& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_vlan_vid& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_output
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_output
        : public ofp_action_output
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_output();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_output(uint16_t type_, uint16_t len_, uint16_t port_, uint16_t max_len_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_output* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_output* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_output& operator=(const of_action_output& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_output& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_action_nw_tos
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_nw_tos
        : public ofp_action_nw_tos
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_nw_tos();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_nw_tos(uint16_t type_, uint16_t len_, uint8_t nw_tos_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_nw_tos* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_nw_tos* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_nw_tos& operator=(const of_action_nw_tos& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_nw_tos& peer_) const;

    private:
    };





    /** \brief Object wrapper for struct ofp_action_vendor_header
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_vendor_header
        : public ofp_action_vendor_header
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_vendor_header();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_vendor_header(uint16_t type_, uint16_t len_, uint32_t vendor_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_vendor_header* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_vendor_header* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_vendor_header& operator=(const of_action_vendor_header& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_vendor_header& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_queue_stats_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_stats_request
        : public ofp_queue_stats_request
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_queue_stats_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_stats_request(uint16_t port_no_, uint8_t pad_[], uint32_t queue_id_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_stats_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_stats_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_stats_request& operator=(const of_queue_stats_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_stats_request& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_desc_stats
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_desc_stats
        : public ofp_desc_stats
    {
    public:
        std::string mfr_desc;
        std::string hw_desc;
        std::string sw_desc;
        std::string serial_num;
        std::string dp_desc;

        /** \brief Empty constructor with default assigned
         */
        of_desc_stats();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_desc_stats(std::string mfr_desc_, std::string hw_desc_, std::string sw_desc_, std::string serial_num_, std::string dp_desc_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_desc_stats* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_desc_stats* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_desc_stats& operator=(const of_desc_stats& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_desc_stats& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_packet_queue
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_packet_queue
        : public ofp_packet_queue
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_packet_queue();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_packet_queue(uint32_t queue_id_, uint16_t len_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_packet_queue* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_packet_queue* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_packet_queue& operator=(const of_packet_queue& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_packet_queue& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_dl_addr
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_dl_addr
        : public ofp_action_dl_addr
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_dl_addr();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_dl_addr(uint16_t type_, uint16_t len_, uint8_t dl_addr_[], uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_dl_addr* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_dl_addr* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_dl_addr& operator=(const of_action_dl_addr& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_dl_addr& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_queue_prop_header
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_prop_header
        : public ofp_queue_prop_header
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_queue_prop_header();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_prop_header(uint16_t property_, uint16_t len_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_prop_header* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_prop_header* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_prop_header& operator=(const of_queue_prop_header& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_prop_header& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_queue_prop_min_rate
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_prop_min_rate
        : public ofp_queue_prop_min_rate
    {
    public:
        of_queue_prop_header prop_header;

        /** \brief Empty constructor with default assigned
         */
        of_queue_prop_min_rate();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_prop_min_rate(of_queue_prop_header prop_header_, uint16_t rate_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_prop_min_rate* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_prop_min_rate* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_prop_min_rate& operator=(const of_queue_prop_min_rate& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_prop_min_rate& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_enqueue
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_enqueue
        : public ofp_action_enqueue
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_enqueue();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_enqueue(uint16_t type_, uint16_t len_, uint16_t port_, uint8_t pad_[], uint32_t queue_id_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_enqueue* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_enqueue* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_enqueue& operator=(const of_action_enqueue& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_enqueue& peer_) const;

    private:
    };


    /** \brief Object wrapper for struct ofp_match
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_match
        : public ofp_match
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_match();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_match(uint32_t wildcards_, uint16_t in_port_, uint8_t dl_src_[], uint8_t dl_dst_[], uint16_t dl_vlan_, uint8_t dl_vlan_pcp_, uint8_t pad1_[], uint16_t dl_type_, uint8_t nw_tos_, uint8_t nw_proto_, uint8_t pad2_[], uint32_t nw_src_, uint32_t nw_dst_, uint16_t tp_src_, uint16_t tp_dst_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_match* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_match* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_match& operator=(const of_match& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_match& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_header
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_header
        : public ofp_header
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_header();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_header(uint8_t version_, uint8_t type_, uint16_t length_, uint32_t xid_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_header* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_header* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_header& operator=(const of_header& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_header& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_vendor_header
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_vendor_header
        : public ofp_vendor_header
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_vendor_header();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_vendor_header(of_header header_, uint32_t vendor_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_vendor_header* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_vendor_header* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_vendor_header& operator=(const of_vendor_header& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_vendor_header& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_packet_out
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_packet_out
        : public ofp_packet_out
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_packet_out();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_packet_out(of_header header_, uint32_t buffer_id_, uint16_t in_port_, uint16_t actions_len_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_packet_out* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_packet_out* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_packet_out& operator=(const of_packet_out& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_packet_out& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_nw_addr
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_nw_addr
        : public ofp_action_nw_addr
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_nw_addr();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_nw_addr(uint16_t type_, uint16_t len_, uint32_t nw_addr_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_nw_addr* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_nw_addr* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_nw_addr& operator=(const of_action_nw_addr& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_nw_addr& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_action_vlan_pcp
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_action_vlan_pcp
        : public ofp_action_vlan_pcp
    {
    public:

        /** \brief Empty constructor with default assigned
         */
        of_action_vlan_pcp();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_action_vlan_pcp(uint16_t type_, uint16_t len_, uint8_t vlan_pcp_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_action_vlan_pcp* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_action_vlan_pcp* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_action_vlan_pcp& operator=(const of_action_vlan_pcp& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_action_vlan_pcp& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_flow_mod
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_flow_mod
        : public ofp_flow_mod
    {
    public:
        of_header header;
        of_match match;

        /** \brief Empty constructor with default assigned
         */
        of_flow_mod();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_flow_mod(of_header header_, of_match match_, uint64_t cookie_, uint16_t command_, uint16_t idle_timeout_, uint16_t hard_timeout_, uint16_t priority_, uint32_t buffer_id_, uint16_t out_port_, uint16_t flags_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_flow_mod* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_flow_mod* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_flow_mod& operator=(const of_flow_mod& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_flow_mod& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_error_msg
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_error_msg
        : public ofp_error_msg
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_error_msg();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_error_msg(of_header header_, uint16_t type_, uint16_t code_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_error_msg* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_error_msg* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_error_msg& operator=(const of_error_msg& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_error_msg& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_flow_removed
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_flow_removed
        : public ofp_flow_removed
    {
    public:
        of_header header;
        of_match match;

        /** \brief Empty constructor with default assigned
         */
        of_flow_removed();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_flow_removed(of_header header_, of_match match_, uint64_t cookie_, uint16_t priority_, uint8_t reason_, uint8_t pad_[], uint32_t duration_sec_, uint32_t duration_nsec_, uint16_t idle_timeout_, uint8_t pad2_[], uint64_t packet_count_, uint64_t byte_count_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_flow_removed* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_flow_removed* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_flow_removed& operator=(const of_flow_removed& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_flow_removed& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_stats_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_stats_request
        : public ofp_stats_request
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_stats_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_stats_request(of_header header_, uint16_t type_, uint16_t flags_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_stats_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_stats_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_stats_request& operator=(const of_stats_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_stats_request& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_hello
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_hello
        : public ofp_hello
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_hello();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_hello(of_header header_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_hello* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_hello* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_hello& operator=(const of_hello& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_hello& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_aggregate_stats_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_aggregate_stats_request
        : public ofp_aggregate_stats_request
    {
    public:
        of_match match;

        /** \brief Empty constructor with default assigned
         */
        of_aggregate_stats_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_aggregate_stats_request(of_match match_, uint8_t table_id_, uint8_t pad_, uint16_t out_port_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_aggregate_stats_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_aggregate_stats_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_aggregate_stats_request& operator=(const of_aggregate_stats_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_aggregate_stats_request& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_port_status
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_port_status
        : public ofp_port_status
    {
    public:
        of_header header;
        of_phy_port desc;

        /** \brief Empty constructor with default assigned
         */
        of_port_status();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_port_status(of_header header_, uint8_t reason_, uint8_t pad_[], of_phy_port desc_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_port_status* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_port_status* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_port_status& operator=(const of_port_status& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_port_status& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_port_mod
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_port_mod
        : public ofp_port_mod
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_port_mod();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_port_mod(of_header header_, uint16_t port_no_, uint8_t hw_addr_[], uint32_t config_, uint32_t mask_, uint32_t advertise_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_port_mod* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_port_mod* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_port_mod& operator=(const of_port_mod& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_port_mod& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_switch_config
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_switch_config
        : public ofp_switch_config
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_switch_config();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_switch_config(of_header header_, uint16_t flags_, uint16_t miss_send_len_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_switch_config* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_switch_config* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_switch_config& operator=(const of_switch_config& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_switch_config& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_queue_get_config_reply
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_get_config_reply
        : public ofp_queue_get_config_reply
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_queue_get_config_reply();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_get_config_reply(of_header header_, uint16_t port_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_get_config_reply* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_get_config_reply* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_get_config_reply& operator=(const of_queue_get_config_reply& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_get_config_reply& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_packet_in
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_packet_in
        : public ofp_packet_in
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_packet_in();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_packet_in(of_header header_, uint32_t buffer_id_, uint16_t total_len_, uint16_t in_port_, uint8_t reason_, uint8_t pad_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_packet_in* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_packet_in* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_packet_in& operator=(const of_packet_in& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_packet_in& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_flow_stats
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_flow_stats
        : public ofp_flow_stats
    {
    public:
        of_match match;

        /** \brief Empty constructor with default assigned
         */
        of_flow_stats();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_flow_stats(uint16_t length_, uint8_t table_id_, uint8_t pad_, of_match match_, uint32_t duration_sec_, uint32_t duration_nsec_, uint16_t priority_, uint16_t idle_timeout_, uint16_t hard_timeout_, uint8_t pad2_[], uint64_t cookie_, uint64_t packet_count_, uint64_t byte_count_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_flow_stats* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_flow_stats* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_flow_stats& operator=(const of_flow_stats& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_flow_stats& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_flow_stats_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_flow_stats_request
        : public ofp_flow_stats_request
    {
    public:
        of_match match;

        /** \brief Empty constructor with default assigned
         */
        of_flow_stats_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_flow_stats_request(of_match match_, uint8_t table_id_, uint8_t pad_, uint16_t out_port_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_flow_stats_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_flow_stats_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_flow_stats_request& operator=(const of_flow_stats_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_flow_stats_request& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_stats_reply
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_stats_reply
        : public ofp_stats_reply
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_stats_reply();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_stats_reply(of_header header_, uint16_t type_, uint16_t flags_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_stats_reply* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_stats_reply* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_stats_reply& operator=(const of_stats_reply& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_stats_reply& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_queue_get_config_request
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_queue_get_config_request
        : public ofp_queue_get_config_request
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_queue_get_config_request();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_queue_get_config_request(of_header header_, uint16_t port_, uint8_t pad_[]);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_queue_get_config_request* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_queue_get_config_request* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_queue_get_config_request& operator=(const of_queue_get_config_request& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_queue_get_config_request& peer_) const;

    private:
    };

    /** \brief Object wrapper for struct ofp_switch_features
     *
     * Everything should be in host order.  Only when the packet
     * is packed, it will be done in network order.  So, all byte
     * order translation is done by this library and no one else.
     * 
     * @author pylibopenflow [by KK Yap (ykk),Dan Talayco]
     * @date Fri May 21 00:00:00 2010
     */
    struct of_switch_features
        : public ofp_switch_features
    {
    public:
        of_header header;

        /** \brief Empty constructor with default assigned
         */
        of_switch_features();

        /** \brief Constructor with every field given
         * 
         * Might be good to assign defaults too?
         */
        of_switch_features(of_header header_, uint64_t datapath_id_, uint32_t n_buffers_, uint8_t n_tables_, uint8_t pad_[], uint32_t capabilities_, uint32_t actions_);

        /** \brief Pack OpenFlow messages
         */
        void pack(ofp_switch_features* buffer);

        /** \brief Unpack OpenFlow messages
         */
        void unpack(ofp_switch_features* buffer);

        /** \brief Overload assignment (for cloning)
         */
        of_switch_features& operator=(const of_switch_features& peer_);

        /** \brief Overload equivalent (for comparison)
         */
        bool operator==(const of_switch_features& peer_) const;

    private:
    };

}
#endif

