#include "RFProtocol.h"

#include <mongo/client/dbclient.h>

VMRegisterRequest::VMRegisterRequest() {
    set_vm_id(0);
}

VMRegisterRequest::VMRegisterRequest(uint64_t vm_id) {
    set_vm_id(vm_id);
}

int VMRegisterRequest::get_type() {
    return VM_REGISTER_REQUEST;
}

uint64_t VMRegisterRequest::get_vm_id() {
    return this->vm_id;
}

void VMRegisterRequest::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

void VMRegisterRequest::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
}

const char* VMRegisterRequest::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string VMRegisterRequest::str() {
    stringstream ss;
    ss << "VMRegisterRequest" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    return ss.str();
}

VMRegisterResponse::VMRegisterResponse() {
    set_accept(false);
}

VMRegisterResponse::VMRegisterResponse(bool accept) {
    set_accept(accept);
}

int VMRegisterResponse::get_type() {
    return VM_REGISTER_RESPONSE;
}

bool VMRegisterResponse::get_accept() {
    return this->accept;
}

void VMRegisterResponse::set_accept(bool accept) {
    this->accept = accept;
}

void VMRegisterResponse::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_accept(obj["accept"].Bool());
}

const char* VMRegisterResponse::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("accept", get_accept());
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string VMRegisterResponse::str() {
    stringstream ss;
    ss << "VMRegisterResponse" << endl;
    ss << "  accept: " << get_accept() << endl;
    return ss.str();
}

VMConfig::VMConfig() {
    set_n_ports(0);
}

VMConfig::VMConfig(uint32_t n_ports) {
    set_n_ports(n_ports);
}

int VMConfig::get_type() {
    return VM_CONFIG;
}

uint32_t VMConfig::get_n_ports() {
    return this->n_ports;
}

void VMConfig::set_n_ports(uint32_t n_ports) {
    this->n_ports = n_ports;
}

void VMConfig::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_n_ports(string_to<uint32_t>(obj["n_ports"].String()));
}

const char* VMConfig::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("n_ports", to_string<uint32_t>(get_n_ports()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string VMConfig::str() {
    stringstream ss;
    ss << "VMConfig" << endl;
    ss << "  n_ports: " << to_string<uint32_t>(get_n_ports()) << endl;
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
    set_address(IPAddress(IPV4));
    set_netmask(IPAddress(IPV4));
    set_dst_port(0);
    set_src_hwaddress(MACAddress());
    set_dst_hwaddress(MACAddress());
    set_is_removal(false);
}

RouteInfo::RouteInfo(uint64_t vm_id, IPAddress address, IPAddress netmask, uint32_t dst_port, MACAddress src_hwaddress, MACAddress dst_hwaddress, bool is_removal) {
    set_vm_id(vm_id);
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

DatapathJoin::DatapathJoin() {
    set_dp_id(0);
    set_n_ports(0);
    set_is_rfvs(false);
}

DatapathJoin::DatapathJoin(uint64_t dp_id, uint32_t n_ports, bool is_rfvs) {
    set_dp_id(dp_id);
    set_n_ports(n_ports);
    set_is_rfvs(is_rfvs);
}

int DatapathJoin::get_type() {
    return DATAPATH_JOIN;
}

uint64_t DatapathJoin::get_dp_id() {
    return this->dp_id;
}

void DatapathJoin::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

uint32_t DatapathJoin::get_n_ports() {
    return this->n_ports;
}

void DatapathJoin::set_n_ports(uint32_t n_ports) {
    this->n_ports = n_ports;
}

bool DatapathJoin::get_is_rfvs() {
    return this->is_rfvs;
}

void DatapathJoin::set_is_rfvs(bool is_rfvs) {
    this->is_rfvs = is_rfvs;
}

void DatapathJoin::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_n_ports(string_to<uint32_t>(obj["n_ports"].String()));
    set_is_rfvs(obj["is_rfvs"].Bool());
}

const char* DatapathJoin::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    _b.append("n_ports", to_string<uint32_t>(get_n_ports()));
    _b.append("is_rfvs", get_is_rfvs());
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathJoin::str() {
    stringstream ss;
    ss << "DatapathJoin" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  n_ports: " << to_string<uint32_t>(get_n_ports()) << endl;
    ss << "  is_rfvs: " << get_is_rfvs() << endl;
    return ss.str();
}

DatapathLeave::DatapathLeave() {
    set_dp_id(0);
}

DatapathLeave::DatapathLeave(uint64_t dp_id) {
    set_dp_id(dp_id);
}

int DatapathLeave::get_type() {
    return DATAPATH_LEAVE;
}

uint64_t DatapathLeave::get_dp_id() {
    return this->dp_id;
}

void DatapathLeave::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

void DatapathLeave::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
}

const char* DatapathLeave::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathLeave::str() {
    stringstream ss;
    ss << "DatapathLeave" << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    return ss.str();
}

VMMap::VMMap() {
    set_vm_id(0);
    set_vm_port(0);
    set_vs_id(0);
    set_vs_port(0);
}

VMMap::VMMap(uint64_t vm_id, uint32_t vm_port, uint64_t vs_id, uint32_t vs_port) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_vs_id(vs_id);
    set_vs_port(vs_port);
}

int VMMap::get_type() {
    return VM_MAP;
}

uint64_t VMMap::get_vm_id() {
    return this->vm_id;
}

void VMMap::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t VMMap::get_vm_port() {
    return this->vm_port;
}

void VMMap::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

uint64_t VMMap::get_vs_id() {
    return this->vs_id;
}

void VMMap::set_vs_id(uint64_t vs_id) {
    this->vs_id = vs_id;
}

uint32_t VMMap::get_vs_port() {
    return this->vs_port;
}

void VMMap::set_vs_port(uint32_t vs_port) {
    this->vs_port = vs_port;
}

void VMMap::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_vs_id(string_to<uint64_t>(obj["vs_id"].String()));
    set_vs_port(string_to<uint32_t>(obj["vs_port"].String()));
}

const char* VMMap::to_BSON() {
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

string VMMap::str() {
    stringstream ss;
    ss << "VMMap" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    ss << "  vs_id: " << to_string<uint64_t>(get_vs_id()) << endl;
    ss << "  vs_port: " << to_string<uint32_t>(get_vs_port()) << endl;
    return ss.str();
}
