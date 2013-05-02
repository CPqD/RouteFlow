#include "RFProtocol.h"

#include <mongo/client/dbclient.h>

PortRegister::PortRegister() {
    set_vm_id(0);
    set_vm_port(0);
    set_hwaddress(MACAddress());
}

PortRegister::PortRegister(uint64_t vm_id, uint32_t vm_port, MACAddress hwaddress) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_hwaddress(hwaddress);
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

MACAddress PortRegister::get_hwaddress() {
    return this->hwaddress;
}

void PortRegister::set_hwaddress(MACAddress hwaddress) {
    this->hwaddress = hwaddress;
}

void PortRegister::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_hwaddress(MACAddress(obj["hwaddress"].String()));
}

const char* PortRegister::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("vm_id", to_string<uint64_t>(get_vm_id()));
    _b.append("vm_port", to_string<uint32_t>(get_vm_port()));
    _b.append("hwaddress", get_hwaddress().toString());
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
    ss << "  hwaddress: " << get_hwaddress().toString() << endl;
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

DatapathPortRegister::DatapathPortRegister() {
    set_ct_id(0);
    set_dp_id(0);
    set_dp_port(0);
}

DatapathPortRegister::DatapathPortRegister(uint64_t ct_id, uint64_t dp_id, uint32_t dp_port) {
    set_ct_id(ct_id);
    set_dp_id(dp_id);
    set_dp_port(dp_port);
}

int DatapathPortRegister::get_type() {
    return DATAPATH_PORT_REGISTER;
}

uint64_t DatapathPortRegister::get_ct_id() {
    return this->ct_id;
}

void DatapathPortRegister::set_ct_id(uint64_t ct_id) {
    this->ct_id = ct_id;
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
    set_ct_id(string_to<uint64_t>(obj["ct_id"].String()));
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_dp_port(string_to<uint32_t>(obj["dp_port"].String()));
}

const char* DatapathPortRegister::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("ct_id", to_string<uint64_t>(get_ct_id()));
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
    ss << "  ct_id: " << to_string<uint64_t>(get_ct_id()) << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  dp_port: " << to_string<uint32_t>(get_dp_port()) << endl;
    return ss.str();
}

DatapathDown::DatapathDown() {
    set_ct_id(0);
    set_dp_id(0);
}

DatapathDown::DatapathDown(uint64_t ct_id, uint64_t dp_id) {
    set_ct_id(ct_id);
    set_dp_id(dp_id);
}

int DatapathDown::get_type() {
    return DATAPATH_DOWN;
}

uint64_t DatapathDown::get_ct_id() {
    return this->ct_id;
}

void DatapathDown::set_ct_id(uint64_t ct_id) {
    this->ct_id = ct_id;
}

uint64_t DatapathDown::get_dp_id() {
    return this->dp_id;
}

void DatapathDown::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

void DatapathDown::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_ct_id(string_to<uint64_t>(obj["ct_id"].String()));
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
}

const char* DatapathDown::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("ct_id", to_string<uint64_t>(get_ct_id()));
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DatapathDown::str() {
    stringstream ss;
    ss << "DatapathDown" << endl;
    ss << "  ct_id: " << to_string<uint64_t>(get_ct_id()) << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    return ss.str();
}

VirtualPlaneMap::VirtualPlaneMap() {
    set_vm_id(0);
    set_vm_port(0);
    set_vs_id(0);
    set_vs_port(0);
}

VirtualPlaneMap::VirtualPlaneMap(uint64_t vm_id, uint32_t vm_port, uint64_t vs_id, uint32_t vs_port) {
    set_vm_id(vm_id);
    set_vm_port(vm_port);
    set_vs_id(vs_id);
    set_vs_port(vs_port);
}

int VirtualPlaneMap::get_type() {
    return VIRTUAL_PLANE_MAP;
}

uint64_t VirtualPlaneMap::get_vm_id() {
    return this->vm_id;
}

void VirtualPlaneMap::set_vm_id(uint64_t vm_id) {
    this->vm_id = vm_id;
}

uint32_t VirtualPlaneMap::get_vm_port() {
    return this->vm_port;
}

void VirtualPlaneMap::set_vm_port(uint32_t vm_port) {
    this->vm_port = vm_port;
}

uint64_t VirtualPlaneMap::get_vs_id() {
    return this->vs_id;
}

void VirtualPlaneMap::set_vs_id(uint64_t vs_id) {
    this->vs_id = vs_id;
}

uint32_t VirtualPlaneMap::get_vs_port() {
    return this->vs_port;
}

void VirtualPlaneMap::set_vs_port(uint32_t vs_port) {
    this->vs_port = vs_port;
}

void VirtualPlaneMap::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_vm_id(string_to<uint64_t>(obj["vm_id"].String()));
    set_vm_port(string_to<uint32_t>(obj["vm_port"].String()));
    set_vs_id(string_to<uint64_t>(obj["vs_id"].String()));
    set_vs_port(string_to<uint32_t>(obj["vs_port"].String()));
}

const char* VirtualPlaneMap::to_BSON() {
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

string VirtualPlaneMap::str() {
    stringstream ss;
    ss << "VirtualPlaneMap" << endl;
    ss << "  vm_id: " << to_string<uint64_t>(get_vm_id()) << endl;
    ss << "  vm_port: " << to_string<uint32_t>(get_vm_port()) << endl;
    ss << "  vs_id: " << to_string<uint64_t>(get_vs_id()) << endl;
    ss << "  vs_port: " << to_string<uint32_t>(get_vs_port()) << endl;
    return ss.str();
}

DataPlaneMap::DataPlaneMap() {
    set_ct_id(0);
    set_dp_id(0);
    set_dp_port(0);
    set_vs_id(0);
    set_vs_port(0);
}

DataPlaneMap::DataPlaneMap(uint64_t ct_id, uint64_t dp_id, uint32_t dp_port, uint64_t vs_id, uint32_t vs_port) {
    set_ct_id(ct_id);
    set_dp_id(dp_id);
    set_dp_port(dp_port);
    set_vs_id(vs_id);
    set_vs_port(vs_port);
}

int DataPlaneMap::get_type() {
    return DATA_PLANE_MAP;
}

uint64_t DataPlaneMap::get_ct_id() {
    return this->ct_id;
}

void DataPlaneMap::set_ct_id(uint64_t ct_id) {
    this->ct_id = ct_id;
}

uint64_t DataPlaneMap::get_dp_id() {
    return this->dp_id;
}

void DataPlaneMap::set_dp_id(uint64_t dp_id) {
    this->dp_id = dp_id;
}

uint32_t DataPlaneMap::get_dp_port() {
    return this->dp_port;
}

void DataPlaneMap::set_dp_port(uint32_t dp_port) {
    this->dp_port = dp_port;
}

uint64_t DataPlaneMap::get_vs_id() {
    return this->vs_id;
}

void DataPlaneMap::set_vs_id(uint64_t vs_id) {
    this->vs_id = vs_id;
}

uint32_t DataPlaneMap::get_vs_port() {
    return this->vs_port;
}

void DataPlaneMap::set_vs_port(uint32_t vs_port) {
    this->vs_port = vs_port;
}

void DataPlaneMap::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_ct_id(string_to<uint64_t>(obj["ct_id"].String()));
    set_dp_id(string_to<uint64_t>(obj["dp_id"].String()));
    set_dp_port(string_to<uint32_t>(obj["dp_port"].String()));
    set_vs_id(string_to<uint64_t>(obj["vs_id"].String()));
    set_vs_port(string_to<uint32_t>(obj["vs_port"].String()));
}

const char* DataPlaneMap::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("ct_id", to_string<uint64_t>(get_ct_id()));
    _b.append("dp_id", to_string<uint64_t>(get_dp_id()));
    _b.append("dp_port", to_string<uint32_t>(get_dp_port()));
    _b.append("vs_id", to_string<uint64_t>(get_vs_id()));
    _b.append("vs_port", to_string<uint32_t>(get_vs_port()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string DataPlaneMap::str() {
    stringstream ss;
    ss << "DataPlaneMap" << endl;
    ss << "  ct_id: " << to_string<uint64_t>(get_ct_id()) << endl;
    ss << "  dp_id: " << to_string<uint64_t>(get_dp_id()) << endl;
    ss << "  dp_port: " << to_string<uint32_t>(get_dp_port()) << endl;
    ss << "  vs_id: " << to_string<uint64_t>(get_vs_id()) << endl;
    ss << "  vs_port: " << to_string<uint32_t>(get_vs_port()) << endl;
    return ss.str();
}

RouteMod::RouteMod() {
    set_mod(0);
    set_id(0);
    set_matches(std::vector<Match>());
    set_actions(std::vector<Action>());
    set_options(std::vector<Option>());
}

RouteMod::RouteMod(uint8_t mod, uint64_t id, std::vector<Match> matches, std::vector<Action> actions, std::vector<Option> options) {
    set_mod(mod);
    set_id(id);
    set_matches(matches);
    set_actions(actions);
    set_options(options);
}

int RouteMod::get_type() {
    return ROUTE_MOD;
}

uint8_t RouteMod::get_mod() {
    return this->mod;
}

void RouteMod::set_mod(uint8_t mod) {
    this->mod = mod;
}

uint64_t RouteMod::get_id() {
    return this->id;
}

void RouteMod::set_id(uint64_t id) {
    this->id = id;
}

std::vector<Match> RouteMod::get_matches() {
    return this->matches;
}

void RouteMod::set_matches(std::vector<Match> matches) {
    this->matches = matches;
}

void RouteMod::add_match(const Match& match) {
    this->matches.push_back(match);
}

std::vector<Action> RouteMod::get_actions() {
    return this->actions;
}

void RouteMod::set_actions(std::vector<Action> actions) {
    this->actions = actions;
}

void RouteMod::add_action(const Action& action) {
    this->actions.push_back(action);
}

std::vector<Option> RouteMod::get_options() {
    return this->options;
}

void RouteMod::set_options(std::vector<Option> options) {
    this->options = options;
}

void RouteMod::add_option(const Option& option) {
    this->options.push_back(option);
}

void RouteMod::from_BSON(const char* data) {
    mongo::BSONObj obj(data);
    set_mod(string_to<uint8_t>(obj["mod"].String()));
    set_id(string_to<uint64_t>(obj["id"].String()));
    set_matches(MatchList::to_vector(obj["matches"].Array()));
    set_actions(ActionList::to_vector(obj["actions"].Array()));
    set_options(OptionList::to_vector(obj["options"].Array()));
}

const char* RouteMod::to_BSON() {
    mongo::BSONObjBuilder _b;
    _b.append("mod", to_string<uint16_t>(get_mod()));
    _b.append("id", to_string<uint64_t>(get_id()));
    _b.appendArray("matches", MatchList::to_BSON(get_matches()));
    _b.appendArray("actions", ActionList::to_BSON(get_actions()));
    _b.appendArray("options", OptionList::to_BSON(get_options()));
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string RouteMod::str() {
    stringstream ss;
    ss << "RouteMod" << endl;
    ss << "  mod: " << to_string<uint16_t>(get_mod()) << endl;
    ss << "  id: " << to_string<uint64_t>(get_id()) << endl;
    ss << "  matches: " << MatchList::to_BSON(get_matches()) << endl;
    ss << "  actions: " << ActionList::to_BSON(get_actions()) << endl;
    ss << "  options: " << OptionList::to_BSON(get_options()) << endl;
    return ss.str();
}
