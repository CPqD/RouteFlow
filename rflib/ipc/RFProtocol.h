#ifndef __RFPROTOCOL_H__
#define __RFPROTOCOL_H__

#include <stdint.h>

#include "IPC.h"
#include "IPAddress.h"
#include "MACAddress.h"
#include "converter.h"

enum {
	VM_REGISTER_REQUEST,
	VM_REGISTER_RESPONSE,
	VM_CONFIG,
	DATAPATH_CONFIG,
	ROUTE_INFO,
	FLOW_MOD,
	DATAPATH_JOIN,
	DATAPATH_LEAVE,
	VM_MAP
};

class VMRegisterRequest : public IPCMessage {
    public:
        VMRegisterRequest();
        VMRegisterRequest(uint64_t vm_id);

        uint64_t get_vm_id();
        void set_vm_id(uint64_t vm_id);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t vm_id;
};

class VMRegisterResponse : public IPCMessage {
    public:
        VMRegisterResponse();
        VMRegisterResponse(bool accept);

        bool get_accept();
        void set_accept(bool accept);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        bool accept;
};

class VMConfig : public IPCMessage {
    public:
        VMConfig();
        VMConfig(uint32_t n_ports);

        uint32_t get_n_ports();
        void set_n_ports(uint32_t n_ports);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint32_t n_ports;
};

class DatapathConfig : public IPCMessage {
    public:
        DatapathConfig();
        DatapathConfig(uint64_t dp_id, uint32_t operation_id);

        uint64_t get_dp_id();
        void set_dp_id(uint64_t dp_id);

        uint32_t get_operation_id();
        void set_operation_id(uint32_t operation_id);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t dp_id;
        uint32_t operation_id;
};

class RouteInfo : public IPCMessage {
    public:
        RouteInfo();
        RouteInfo(uint64_t vm_id, IPAddress address, IPAddress netmask, uint32_t dst_port, MACAddress src_hwaddress, MACAddress dst_hwaddress, bool is_removal);

        uint64_t get_vm_id();
        void set_vm_id(uint64_t vm_id);

        IPAddress get_address();
        void set_address(IPAddress address);

        IPAddress get_netmask();
        void set_netmask(IPAddress netmask);

        uint32_t get_dst_port();
        void set_dst_port(uint32_t dst_port);

        MACAddress get_src_hwaddress();
        void set_src_hwaddress(MACAddress src_hwaddress);

        MACAddress get_dst_hwaddress();
        void set_dst_hwaddress(MACAddress dst_hwaddress);

        bool get_is_removal();
        void set_is_removal(bool is_removal);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t vm_id;
        IPAddress address;
        IPAddress netmask;
        uint32_t dst_port;
        MACAddress src_hwaddress;
        MACAddress dst_hwaddress;
        bool is_removal;
};

class FlowMod : public IPCMessage {
    public:
        FlowMod();
        FlowMod(uint64_t dp_id, IPAddress address, IPAddress netmask, uint32_t dst_port, MACAddress src_hwaddress, MACAddress dst_hwaddress, bool is_removal);

        uint64_t get_dp_id();
        void set_dp_id(uint64_t dp_id);

        IPAddress get_address();
        void set_address(IPAddress address);

        IPAddress get_netmask();
        void set_netmask(IPAddress netmask);

        uint32_t get_dst_port();
        void set_dst_port(uint32_t dst_port);

        MACAddress get_src_hwaddress();
        void set_src_hwaddress(MACAddress src_hwaddress);

        MACAddress get_dst_hwaddress();
        void set_dst_hwaddress(MACAddress dst_hwaddress);

        bool get_is_removal();
        void set_is_removal(bool is_removal);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t dp_id;
        IPAddress address;
        IPAddress netmask;
        uint32_t dst_port;
        MACAddress src_hwaddress;
        MACAddress dst_hwaddress;
        bool is_removal;
};

class DatapathJoin : public IPCMessage {
    public:
        DatapathJoin();
        DatapathJoin(uint64_t dp_id, uint32_t n_ports, bool is_rfvs);

        uint64_t get_dp_id();
        void set_dp_id(uint64_t dp_id);

        uint32_t get_n_ports();
        void set_n_ports(uint32_t n_ports);

        bool get_is_rfvs();
        void set_is_rfvs(bool is_rfvs);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t dp_id;
        uint32_t n_ports;
        bool is_rfvs;
};

class DatapathLeave : public IPCMessage {
    public:
        DatapathLeave();
        DatapathLeave(uint64_t dp_id);

        uint64_t get_dp_id();
        void set_dp_id(uint64_t dp_id);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t dp_id;
};

class VMMap : public IPCMessage {
    public:
        VMMap();
        VMMap(uint64_t vm_id, uint32_t vm_port, uint64_t vs_id, uint32_t vs_port);

        uint64_t get_vm_id();
        void set_vm_id(uint64_t vm_id);

        uint32_t get_vm_port();
        void set_vm_port(uint32_t vm_port);

        uint64_t get_vs_id();
        void set_vs_id(uint64_t vs_id);

        uint32_t get_vs_port();
        void set_vs_port(uint32_t vs_port);

        virtual int get_type();
        virtual void from_BSON(const char* data);
        virtual const char* to_BSON();
        virtual string str();

    private:
        uint64_t vm_id;
        uint32_t vm_port;
        uint64_t vs_id;
        uint32_t vs_port;
};

#endif /* __RFPROTOCOL_H__ */