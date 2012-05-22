#ifndef __TABLE_H__
#define __TABLE_H__

#include <string>
#include <iostream>
#include <mongo/client/dbclient.h>
#include <vector>
#include <map>

// Constants mapping application and DB fields
#define ID "_id"
#define VM_ID "vm_id"
#define VM_PORT "vm_port"
#define VS_ID "vs_id"
#define VS_PORT "vs_port"
#define DP_ID "dp_id"
#define DP_PORT "dp_port"
        
using namespace std;

// TODO: stop using strings and use proper typing

class RFEntry {
    public:
        RFEntry() : id("") {};
        
        string get_id();
        string set_id(string id);

        string get_vm_id();
        void set_vm_id(string vm_id);
        
        string get_vm_port();
        void set_vm_port(string vm_port);
        
        string get_vs_id();
        void set_vs_id(string vs_id);
        
        string get_vs_port();
        void set_vs_port(string vs_port);
        
        string get_dp_id();
        void set_dp_id(string dp_id);
        
        string get_dp_port();
        void set_dp_port(string dp_port);

        bool is_mapped();
        
        void fromBSON(const char* data);
        const char* toBSON();

        string toString();
        
    private:
        string id;
            
        string vm_id;
        string vm_port;
        
        string vs_id;
        string vs_port;

        string dp_id;
        string dp_port;
};

class RFTable {
    public:
        RFTable(const string server_address, const string db, const string table);
        vector<RFEntry> get_entries(map<string, string> parameters);
        void set_entry(RFEntry &entry);
        void remove_entry(RFEntry &entry);
        void clear();

    private:
        mongo::DBClientConnection connection;
        string ns;
};

#endif /* __TABLE_H__ */
