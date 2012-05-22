#include "RFTable.h"

string RFEntry::get_id() {
    return this->id;
}

string RFEntry::set_id(string id) {
    return this->id = id;
}

string RFEntry::get_vm_id() {
    return this->vm_id;
}

void RFEntry::set_vm_id(string vm_id) {
    this->vm_id = vm_id;
}

string RFEntry::get_vm_port() {
    return this->vm_port;
}

void RFEntry::set_vm_port(string vm_port) {
    this->vm_port = vm_port;
}

string RFEntry::get_vs_id() {
    return this->vs_id;
}

void RFEntry::set_vs_id(string vs_id) {
    this->vs_id = vs_id;
}

string RFEntry::get_vs_port() {
    return this->vs_port;
}

void RFEntry::set_vs_port(string vs_port) {
    this->vs_port = vs_port;
}

string RFEntry::get_dp_id() {
    return this->dp_id;
}

void RFEntry::set_dp_id(string dp_id) {
    this->dp_id = dp_id;
}

string RFEntry::get_dp_port() {
    return this->dp_port;
}

void RFEntry::set_dp_port(string dp_port) {
    this->dp_port = dp_port;
}

bool RFEntry::is_mapped() {
    return not (this->vm_port.empty() and
        this->vs_id.empty() and
        this->vs_port.empty() and
        this->dp_port.empty());
}

void RFEntry::fromBSON(const char* data) {
    mongo::BSONObj obj(data);

    set_id(obj[ID].OID().str());

    set_vm_id(obj[VM_ID].String());
    set_vm_port(obj[VM_PORT].String());

    set_vs_id(obj[VS_ID].String());
    set_vs_port(obj[VS_PORT].String());

    set_dp_id(obj[DP_ID].String());
    set_dp_port(obj[DP_PORT].String());
}

const char* RFEntry::toBSON() {
    mongo::BSONObjBuilder _b;
    get_id() == ""? _b.genOID() : _b.append(ID, mongo::OID(get_id()));
    _b.append(VM_ID, get_vm_id());
    _b.append(VM_PORT, get_vm_port());
    _b.append(VS_ID, get_vs_id());
    _b.append(VS_PORT, get_vs_port());
    _b.append(DP_ID, get_dp_id());
    _b.append(DP_PORT, get_dp_port());
    mongo::BSONObj o = _b.obj();
    char* data = new char[o.objsize()];
    memcpy(data, o.objdata(), o.objsize());
    return data;
}

string RFEntry::toString() {
    stringstream ss;
    ss << "RFEntry" << endl;
    ss << "  vm_id: " << get_vm_id() << endl;
    ss << "  vm_port: " << get_vm_port() << endl;
    ss << "  vs_id: " << get_vs_id() << endl;
    ss << "  vs_port: " << get_vs_port() << endl;
    ss << "  dp_id: " << get_dp_id() << endl;
    ss << "  dp_port: " << get_dp_port() << endl;
    return ss.str();
}

RFTable::RFTable(const string server_address, const string db, const string table) {
    try {
        this->connection.connect(server_address);
    }
    catch(mongo::DBException &e) {
        cout << "Exception: " << e.what() << endl;
        exit(1);
    }

    this->ns = db + "." + table;
    this->connection.createCollection(ns);
}

vector<RFEntry> RFTable::get_entries(map<string, string> parameters) {
    vector<RFEntry> results;
    
    mongo::BSONObjBuilder b;
    map<string, string>::iterator it;
    for (it=parameters.begin(); it != parameters.end(); it++) {
        if (it->second == "*")
            b.append(it->first, BSON("$ne" << ""));
        else
            b.append(it->first, it->second);
    }
    auto_ptr<mongo::DBClientCursor> cursor = this->connection.query(this->ns, mongo::Query(b.obj()));
    
    while (cursor->more()) {
        mongo::BSONObj result = cursor->next();
        if (result.isEmpty())
            continue;
            
        RFEntry entry;
        entry.fromBSON(result.objdata());
        results.push_back(entry);
    }

    return results;
}

void RFTable::set_entry(RFEntry &entry) {
    const char* data = entry.toBSON();
    mongo::BSONObj obj = mongo::BSONObj(data);
    
    if (entry.get_id() == "" or
        this->connection.findOne(this->ns, QUERY(ID << obj[ID])).isEmpty()) {
        this->connection.insert(this->ns, obj);
    }
    else {
        mongo::OID oid = obj[ID].OID();
        obj = obj.removeField(ID);
        this->connection.update(this->ns,
            BSON(ID << oid),
            BSON("$set" << obj));
    }
    
    delete data;
}

void RFTable::remove_entry(RFEntry &entry) {
    const char* data = entry.toBSON();
    mongo::BSONObj obj = mongo::BSONObj(data);

    this->connection.remove(this->ns, BSON(ID << obj[ID]));
                
    delete data;
}

void RFTable::clear() {
    this->connection.remove(this->ns, mongo::BSONObj());
}

/*
int main() {
    RFTable t("localhost", "db", "rfdata");
    
    RFEntry e;
    e.set_vm_id("vm1");
    e.set_vm_port("1");
    e.set_vs_id("vs1");
    e.set_vs_port("2");
    e.set_dp_id("dp1");
    e.set_dp_port("3");
    t.set_entry(e);
    
    RFEntry e2;
    e2.set_vm_id("vm2");
    e2.set_vm_port("1");
    e2.set_vs_id("vs3");
    e2.set_vs_port("100");
    e2.set_dp_id("dp1");
    e2.set_dp_port("76");
    t.set_entry(e2);
    
    RFEntry e3;
    e3.set_dp_id("dp1");
    e3.set_dp_port("3");
    t.set_entry(e3);
    
    RFEntry e4;
    e4.set_vm_id("2221476214413212");
    e4.set_vm_port("7");
    e4.set_vs_id("150876205513");
    e4.set_vs_port("15");
    t.set_entry(e4);
    
    map<string, string> query;
    vector<RFEntry> results;
    
    cout << "VMs with no associated datapath" << endl;
    query.clear();
    query[DP_ID] = "";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;

    cout << "Datapaths with no associated VM" << endl;
    query.clear();
    query[VM_ID] = "";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;

    cout << "VMs answering for VS1" << endl;
    query.clear();
    //query[DP_PORT] = "76";
    //query[DP_ID] = "dp1";
    //query[VM_ID] = "2221476214413212";
    query[VS_ID] = "150876205513";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;

    cout << "VMs answering for VS1" << endl;
    query.clear();
    //query[DP_PORT] = "76";
    //query[DP_ID] = "dp1";
    //query[VM_ID] = "2221476214413212";
    query[VS_ID] = "150876205513";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;
 
    cout << "Update all entries with vm_id=vm1 to vm_id=vm123" << endl;
    query.clear();
    query[VM_ID] = "vm1";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++) {
        it->set_vm_id("vm123");
        t.set_entry(*it);
    }

    cout << "Check that entries have changed to vm_id=vm123" << endl;
    query.clear();
    query[VM_ID] = "vm123";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;
      
    cout << "Check that there are no entries with vm_id=vm1" << endl;
    query.clear();
    query[VM_ID] = "vm1";
    results = t.get_entries(query);
    for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
        cout << it->toString() << endl;
                 
    return 0;
}*/
