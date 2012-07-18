#include "RFProtocol.h"

#include <mongo/client/dbclient.h>

PortRegister::PortRegister() {
    set_vm_id(0);
    set_vm_port(0);
}

PortRegister::PortRegister(uint64_t vm_id, uint32_t vm_port) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
}

int PortRegister::get_type() {
    return PORT_REGISTER;
}

uint64_t PortRegister::get_vm_id() {
    return this->vm_id;
}

void PortRegister::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t PortRegister::get_vm_port() {
    return this->vm_port;
}

void PortRegister::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

void PortRegister::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
}

const char* PortRegister::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    _b.append("vm_port", to_string<uint32_t>(get_vm_port()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string PortRegister::str() {
    stringstream ss;
    ss << "PortRegister" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    return ss.str();
}

PortConfig::PortConfig() {
    set_vm_id(0);
    set_vm_port(0);
    set_operation_id(0);
}

PortConfig::PortConfig(uint64_t vm_id, uint32_t vm_port, uint32_t operation_id) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_operation_id(operation_id);
}

int PortConfig::get_type() {
    return PORT_CONFIG;
}

uint64_t PortConfig::get_vm_id() {
    return this->vm_id;
}

void PortConfig::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t PortConfig::get_vm_port() {
    return this->vm_port;
}

void PortConfig::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

uint32_t PortConfig::get_operation_id() {
    return this->operation_id;
}

void PortConfig::set_operation_id(uint32_t operation_id) {
    this->operation_id = operation_id;
}

void PortConfig::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_operation_id(string_to<uint32_t>(obj["operation_id"].String()));
}

const char* PortConfig::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    _b.append("vm_port", to_string<uint32_t>(get_vm_port()));
    _b.append("operation_id", to_string<uint32_t>(get_operation_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string PortConfig::str() {
    stringstream ss;
    ss << "PortConfig" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    ss << "  operation_id: " << to_string<uint32_t>(get_operation_id()) << endl;
    return ss.str();
}

DatapathConfig::DatapathConfig() {
    set_dp_id(0);
    set_operation_id(0);
}

DatapathConfig::DatapathConfig(uint64_t dp_id, uint32_t operation_id) {
    set_dp_id(dp_id);
    set_operation_id(operation_id);
}

int DatapathConfig::get_type() {
    return DATAPATH_CONFIG;
}

uint64_t DatapathConfig::get_dp_id() {
    return this->dp_id;
}

void DatapathConfig::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

uint32_t DatapathConfig::get_operation_id() {
    return this->operation_id;
}

void DatapathConfig::set_operation_id(uint32_t operation_id) {
    this->operation_id = operation_id;
}

void DatapathConfig::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_operation_id(string_to<uint32_t>(obj["operation_id"].String()));
}

const char* DatapathConfig::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    _b.append("operation_id", to_string<uint32_t>(get_operation_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathConfig::str() {
    stringstream ss;
    ss << "DatapathConfig" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  operation_id: " << to_string<uint32_t>(get_operation_id()) << endl;
    return ss.str();
}

RouteInfo::RouteInfo() {
    set_vm_id(0);
    set_vm_port(0);
    set_address(IPAddress(IPV4));
    set_netmask(IPAddress(IPV4));
    set_dst_port(0);
    set_src_hwaddress(MACAddress());
    set_dst_hwaddress(MACAddress());
    set_is_removal(false);
}

RouteInfo::RouteInfo(uint64_t vm_id, uint32_t vm_port, IPAddress address, IPAddress netmask, uint32_t dst_port, MACAddress src_hwaddress, MACAddress dst_hwaddress, bool is_removal) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_address(address);
    set_netmask(netmask);
    set_dst_port(dst_port);
    set_src_hwaddress(src_hwaddress);
    set_dst_hwaddress(dst_hwaddress);
    set_is_removal(is_removal);
}

int RouteInfo::get_type() {
    return ROUTE_INFO;
}

uint64_t RouteInfo::get_vm_id() {
    return this->vm_id;
}

void RouteInfo::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t RouteInfo::get_vm_port() {
    return this->vm_port;
}

void RouteInfo::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

IPAddress RouteInfo::get_address() {
    return this->address;
}

void RouteInfo::set_address(IPAddress address) {
    this->address = address;
}

IPAddress RouteInfo::get_netmask() {
    return this->netmask;
}

void RouteInfo::set_netmask(IPAddress netmask) {
    this->netmask = netmask;
}

uint32_t RouteInfo::get_dst_port() {
    return this->dst_port;
}

void RouteInfo::set_dst_port(uint32_t dst_port) {
    this->dst_port = dst_port;
}

MACAddress RouteInfo::get_src_hwaddress() {
    return this->src_hwaddress;
}

void RouteInfo::set_src_hwaddress(MACAddress src_hwaddress) {
    this->src_hwaddress = src_hwaddress;
}

MACAddress RouteInfo::get_dst_hwaddress() {
    return this->dst_hwaddress;
}

void RouteInfo::set_dst_hwaddress(MACAddress dst_hwaddress) {
    this->dst_hwaddress = dst_hwaddress;
}

bool RouteInfo::get_is_removal() {
    return this->is_removal;
}

void RouteInfo::set_is_removal(bool is_removal) {
    this->is_removal = is_removal;
}

void RouteInfo::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_address(IPAddress(IPV4, obj["address"].String()));
    set_netmask(IPAddress(IPV4, obj["netmask"].String()));
    set_dst_port(string_to<uint32_t>(obj["dst_port"].String()));
    set_src_hwaddress(MACAddress(obj["src_hwaddress"].String()));
    set_dst_hwaddress(MACAddress(obj["dst_hwaddress"].String()));
    set_is_removal(obj["is_removal"].Bool());
}

const char* RouteInfo::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    _b.append("vm_port", to_string<uint32_t>(get_vm_port()));
    _b.append("address", get_address().toString());
    _b.append("netmask", get_netmask().toString());
    _b.append("dst_port", to_string<uint32_t>(get_dst_port()));
    _b.append("src_hwaddress", get_src_hwaddress().toString());
    _b.append("dst_hwaddress", get_dst_hwaddress().toString());
    _b.append("is_removal", get_is_removal());
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string RouteInfo::str() {
    stringstream ss;
    ss << "RouteInfo" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    ss << "  address: " << get_address().toString() << endl;
    ss << "  netmask: " << get_netmask().toString() << endl;
    ss << "  dst_port: " << to_string<uint32_t>(get_dst_port()) << endl;
    ss << "  src_hwaddress: " << get_src_hwaddress().toString() << endl;
    ss << "  dst_hwaddress: " << get_dst_hwaddress().toString() << endl;
    ss << "  is_removal: " << get_is_removal() << endl;
    return ss.str();
}

FlowMod::FlowMod() {
    set_dp_id(0);
    set_address(IPAddress(IPV4));
    set_netmask(IPAddress(IPV4));
    set_dst_port(0);
    set_src_hwaddress(MACAddress());
    set_dst_hwaddress(MACAddress());
    set_is_removal(false);
}

FlowMod::FlowMod(uint64_t dp_id, IPAddress address, IPAddress netmask, uint32_t dst_port, MACAddress src_hwaddress, MACAddress dst_hwaddress, bool is_removal) {
    set_dp_id(dp_id);
    set_address(address);
    set_netmask(netmask);
    set_dst_port(dst_port);
    set_src_hwaddress(src_hwaddress);
    set_dst_hwaddress(dst_hwaddress);
    set_is_removal(is_removal);
}

int FlowMod::get_type() {
    return FLOW_MOD;
}

uint64_t FlowMod::get_dp_id() {
    return this->dp_id;
}

void FlowMod::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

IPAddress FlowMod::get_address() {
    return this->address;
}

void FlowMod::set_address(IPAddress address) {
    this->address = address;
}

IPAddress FlowMod::get_netmask() {
    return this->netmask;
}

void FlowMod::set_netmask(IPAddress netmask) {
    this->netmask = netmask;
}

uint32_t FlowMod::get_dst_port() {
    return this->dst_port;
}

void FlowMod::set_dst_port(uint32_t dst_port) {
    this->dst_port = dst_port;
}

MACAddress FlowMod::get_src_hwaddress() {
    return this->src_hwaddress;
}

void FlowMod::set_src_hwaddress(MACAddress src_hwaddress) {
    this->src_hwaddress = src_hwaddress;
}

MACAddress FlowMod::get_dst_hwaddress() {
    return this->dst_hwaddress;
}

void FlowMod::set_dst_hwaddress(MACAddress dst_hwaddress) {
    this->dst_hwaddress = dst_hwaddress;
}

bool FlowMod::get_is_removal() {
    return this->is_removal;
}

void FlowMod::set_is_removal(bool is_removal) {
    this->is_removal = is_removal;
}

void FlowMod::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_address(IPAddress(IPV4, obj["address"].String()));
    set_netmask(IPAddress(IPV4, obj["netmask"].String()));
    set_dst_port(string_to<uint32_t>(obj["dst_port"].String()));
    set_src_hwaddress(MACAddress(obj["src_hwaddress"].String()));
    set_dst_hwaddress(MACAddress(obj["dst_hwaddress"].String()));
    set_is_removal(obj["is_removal"].Bool());
}

const char* FlowMod::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    _b.append("address", get_address().toString());
    _b.append("netmask", get_netmask().toString());
    _b.append("dst_port", to_string<uint32_t>(get_dst_port()));
    _b.append("src_hwaddress", get_src_hwaddress().toString());
    _b.append("dst_hwaddress", get_dst_hwaddress().toString());
    _b.append("is_removal", get_is_removal());
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string FlowMod::str() {
    stringstream ss;
    ss << "FlowMod" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  address: " << get_address().toString() << endl;
    ss << "  netmask: " << get_netmask().toString() << endl;
    ss << "  dst_port: " << to_string<uint32_t>(get_dst_port()) << endl;
    ss << "  src_hwaddress: " << get_src_hwaddress().toString() << endl;
    ss << "  dst_hwaddress: " << get_dst_hwaddress().toString() << endl;
    ss << "  is_removal: " << get_is_removal() << endl;
    return ss.str();
}

DatapathPortRegister::DatapathPortRegister() {
    set_dp_id(0);
    set_dp_port(0);
}

DatapathPortRegister::DatapathPortRegister(uint64_t dp_id, uint32_t dp_port) {
    set_dp_id(dp_id);
    set_dp_port(dp_port);
}

int DatapathPortRegister::get_type() {
    return DATAPATH_PORT_REGISTER;
}

uint64_t DatapathPortRegister::get_dp_id() {
    return this->dp_id;
}

void DatapathPortRegister::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

uint32_t DatapathPortRegister::get_dp_port() {
    return this->dp_port;
}

void DatapathPortRegister::set_dp_port(uint32_t dp_port) {
    this->dp_port = dp_port;
}

void DatapathPortRegister::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_dp_port(string_to<uint32_t>(obj["dp_port"].String()));
}

const char* DatapathPortRegister::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    _b.append("dp_port", to_string<uint32_t>(get_dp_port()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathPortRegister::str() {
    stringstream ss;
    ss << "DatapathPortRegister" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  dp_port: " << to_string<uint32_t>(get_dp_port()) << endl;
    return ss.str();
}

DatapathDown::DatapathDown() {
    set_dp_id(0);
}

DatapathDown::DatapathDown(uint64_t dp_id) {
    set_dp_id(dp_id);
}

int DatapathDown::get_type() {
    return DATAPATH_DOWN;
}

uint64_t DatapathDown::get_dp_id() {
    return this->dp_id;
}

void DatapathDown::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

void DatapathDown::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
}

const char* DatapathDown::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathDown::str() {
    stringstream ss;
    ss << "DatapathDown" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    return ss.str();
}

PortMap::PortMap() {
    set_vm_id(0);
    set_vm_port(0);
    set_vs_id(0);
    set_vs_port(0);
}

PortMap::PortMap(uint64_t vm_id, uint32_t vm_port, uint64_t vs_id, uint32_t vs_port) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_vs_id(vs_id);
    set_vs_port(vs_port);
}

int PortMap::get_type() {
    return PORT_MAP;
}

uint64_t PortMap::get_vm_id() {
    return this->vm_id;
}

void PortMap::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t PortMap::get_vm_port() {
    return this->vm_port;
}

void PortMap::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

uint64_t PortMap::get_vs_id() {
    return this->vs_id;
}

void PortMap::set_vs_id(uint64_t vs_id) {
    this->vs_id = vs_id;
}

uint32_t PortMap::get_vs_port() {
    return this->vs_port;
}

void PortMap::set_vs_port(uint32_t vs_port) {
    this->vs_port = vs_port;
}

void PortMap::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_vs_id(string_to<uint64_t>(obj["vs_id"].String()));
    set_vs_port(string_to<uint32_t>(obj["vs_port"].String()));
}

const char* PortMap::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    _b.append("vm_port", to_string<uint32_t>(get_vm_port()));
    _b.append("vs_id", to_string<uint64_t>(get_vs_id()));
    _b.append("vs_port", to_string<uint32_t>(get_vs_port()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string PortMap::str() {
    stringstream ss;
    ss << "PortMap" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    ss << "  vs_id: " << to_string<uint64_t>(get_vs_id()) << endl;
    ss << "  vs_port: " << to_string<uint32_t>(get_vs_port()) << endl;
    return ss.str();
}
