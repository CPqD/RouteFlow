#include <syslog.h>

#include "ipc/IPC.h"
#include "ipc/MongoIPC.h"
#include "ipc/RFProtocol.h"
#include "ipc/RFProtocolFactory.h"
#include "rftable/RFTable.h"
#include "converter.h"
#include "defs.h"

class RFServer : private RFProtocolFactory, private IPCMessageProcessor {
    public:
        RFServer() {
            this->ipc = (IPCMessageService*) new MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, RFSERVER_ID);
            this->rftable = new RFTable(MONGO_ADDRESS, MONGO_DB_NAME, RF_TABLE_NAME);

            syslog(LOG_INFO, "Creating server id=%s", RFSERVER_ID);
        }

        void start(bool clear) {
            if (clear) {
                this->rftable->clear();
                syslog(LOG_INFO, "Clearing RFTable...");
            }

            syslog(LOG_INFO, "Listening for messages from slaves and controller...");
            ipc->listen(RFCLIENT_RFSERVER_CHANNEL, this, this, false);
            ipc->listen(RFSERVER_RFPROXY_CHANNEL, this, this);
        }

    private:
        IPCMessageService* ipc;
        RFTable* rftable;

        void register_vm(uint64_t vm_id) {
            // TODO: tmp fix for rftable
            string _vm_id = to_string<uint64_t>(vm_id);

            VMRegisterResponse response;
            bool accept = false;
            bool idle = false;

            // First, we query for the VM.
            map<string, string> query;
            vector<RFEntry> results;
            query[VM_ID] = _vm_id;
            results = this->rftable->get_entries(query);

            // In case the VM is new we...
            if (results.empty()) {
                // check if there is an idle datapath to connect...
                query.clear();
                query[DP_ID] = "*";
                query[VM_ID] = "";
                vector<RFEntry> dpresults;
                dpresults = this->rftable->get_entries(query);
                if(not dpresults.empty()) {
                    query.clear();
                    query[DP_ID] = string_to<uint64_t>(dpresults.front().get_dp_id());
                    for (vector<RFEntry>::iterator it = dpresults.begin(); it != dpresults.end(); it++) {
                        it->set_vm_id(_vm_id);
                        rftable->set_entry(*it);
                    }
                    accept = true;
                    idle = false;
                }
                // if we can't find a datapath, the VM stays idle.
                else {
                    RFEntry entry;
                    entry.set_vm_id(_vm_id);
                    rftable->set_entry(entry);

                    accept = true;
                    idle = true;
                }
            }
            // If the VM was already registered and the entry was inactive, we restore its datapath association.
            else {
                set<uint64_t> dps;
                for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++) {
                    it->set_vm_port("");
                    it->set_vs_id("");
                    it->set_vs_port("");
                    it->set_dp_port("");
                    dps.insert(string_to<uint64_t>(it->get_dp_id()));
                    rftable->set_entry(*it);
                }
                // Reset datapath config
                for (set<uint64_t>::iterator it = dps.begin(); it != dps.end(); it++) {
                    configure_dp(*it);
                }
                syslog(LOG_INFO, "vm=0x%llx already registered, resetting config but keeping datapath", vm_id);

                accept = true;
                idle = false;
            }

            // Finally, we say that we accept the VM and...
            if (accept) {
                syslog(LOG_INFO, "Accepting vm=0x%llx", vm_id);
                response.set_accept(true);
                this->ipc->send(RFCLIENT_RFSERVER_CHANNEL, _vm_id, response);

                // if it's not idle, we configure it.
                if (not idle)
                    configure_vm(vm_id);
            }
        }

        void register_datapath(uint64_t dp_id, int n_ports) {
            // TODO: tmp fix for rftable
            string _dp_id = to_string<uint64_t>(dp_id);

            map<string, string> query;
            vector<RFEntry> results;

            // Configure datapath to forward routing flows
            configure_dp(dp_id);
            
            // Check whether the datapath isn't already registered and was previously associated. If it was, restore the association.
            query[DP_ID] = _dp_id;
            query[VM_ID] = "*";
            results = this->rftable->get_entries(query);
            if (not results.empty()) {
                // Ask all VMs associated with this datapath for a mapping
                set<uint64_t> vms;
                for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++)
                    if (not it->is_mapped())
                        vms.insert(string_to<uint64_t>(it->get_vm_id()));
                for (set<uint64_t>::iterator it = vms.begin(); it != vms.end(); it++)
                    configure_vm(*it);
                return;
            }

            // Check if there is an idle vm to connect...
            query.clear();
            query[VM_ID] = "*";
            query[DP_ID] = "";
            results = this->rftable->get_entries(query);
            // if there's, we associate VM and datapath.
            if (not results.empty()){
                string _vm_id = results.front().get_vm_id();
                uint64_t vm_id = string_to<uint64_t>(_vm_id);
                for(int i = 0; i < n_ports; i++) {
                    RFEntry e;
                    e.set_dp_id(_dp_id);
                    e.set_vm_id(_vm_id);
                    rftable->set_entry(e);
                }

                rftable->remove_entry(results.front());
                configure_vm(vm_id);
                syslog(LOG_INFO, "dp=0x%llx registered and associated with vm=%llx", dp_id, vm_id);
            }
            else {
                // Otherwise, register the datapath as idle.
                RFEntry entry;
                entry.set_dp_id(_dp_id);
                for (int i = 0; i < n_ports; i++)
                    rftable->set_entry(entry);
                syslog(LOG_INFO, "Couldn't find an idle VM, registering dp=0x%llx as idle", dp_id);
            }
        }
        
        void configure_vm(uint64_t vm_id) {
            // TODO: tmp fix for rftable
            string _vm_id = to_string<uint64_t>(vm_id);
            
            // TODO: specify number of ports as parameter
            VMConfig msg(0);
            this->ipc->send(RFCLIENT_RFSERVER_CHANNEL, _vm_id, msg);
            syslog(LOG_INFO, "Configure message sent to vm=0x%llx", vm_id);
        }

        void configure_dp(uint64_t dp_id) {
            // Clear datapath flows and install flows to receive essential traffic
            send_datapath_config_message(dp_id, DC_CLEAR_FLOW_TABLE);
            // TODO: enforce order: clear should always be executed first
            send_datapath_config_message(dp_id, DC_OSPF);
            send_datapath_config_message(dp_id, DC_BGP);
            send_datapath_config_message(dp_id, DC_RIPV2);
            send_datapath_config_message(dp_id, DC_ARP);
            send_datapath_config_message(dp_id, DC_ICMP);
        }

        void send_datapath_config_message(uint64_t dp_id, DATAPATH_CONFIG_OPERATION operation) {
            DatapathConfig msg(dp_id, (uint32_t) operation);
            ipc->send(RFSERVER_RFPROXY_CHANNEL, RFPROXY_ID, msg);
        }

        bool process(const string &from, const string &to, const string &channel, IPCMessage& msg) {
            int type = msg.get_type();
            // TODO: move as much of these to functions to make it more readable
            if (type == VM_REGISTER_REQUEST) {
                VMRegisterRequest *request = dynamic_cast<VMRegisterRequest*>(&msg);
                syslog(LOG_INFO, "Register request from vm=0x%llx", request->get_vm_id());
                this->register_vm(request->get_vm_id());
            }
            else if (type == ROUTE_INFO) {
                RouteInfo *info = dynamic_cast<RouteInfo*>(&msg);
                // Find the datapath associated to the VM
                // TODO: add support for multiple datapaths associated to one VM
                map<string, string> query;
                vector<RFEntry> results;
                query[VM_ID] = to_string<uint64_t>(info->get_vm_id());
                results = this->rftable->get_entries(query);
                if (results.empty() || results.front().get_dp_id().empty())
                    return true;

                FlowMod flow;
                flow.set_dp_id(string_to<uint64_t>(results.front().get_dp_id()));
                flow.set_address(info->get_address());
                flow.set_netmask(info->get_netmask());
                flow.set_dst_port(info->get_dst_port());
                flow.set_src_hwaddress(info->get_src_hwaddress());
                flow.set_dst_hwaddress(info->get_dst_hwaddress());
                flow.set_is_removal(info->get_is_removal());

                ipc->send(RFSERVER_RFPROXY_CHANNEL, RFPROXY_ID, flow);
            }
            else if (type == DATAPATH_LEAVE) {
                DatapathLeave* dpleave = dynamic_cast<DatapathLeave*>(&msg);
                // Find the entries associated with the datapath
                map<string, string> query;
                vector<RFEntry> results;
                query[DP_ID] = to_string<uint64_t>(dpleave->get_dp_id());
                results = this->rftable->get_entries(query);
                // Keep the VM-datapath association, but discard mapping info
                for (vector<RFEntry>::iterator it = results.begin(); it != results.end(); it++) {
                    it->set_vm_port("");
                    it->set_vs_id("");
                    it->set_vs_port("");
                    it->set_dp_port("");
                    rftable->set_entry(*it);
                }
            }
            else if (type == DATAPATH_JOIN) {
                DatapathJoin* dpjoin = dynamic_cast<DatapathJoin*>(&msg);
                // Configure normal datapaths (all except the control plane switch)
                if (not dpjoin->get_is_rfvs()) {
                    this->register_datapath(dpjoin->get_dp_id(), dpjoin->get_n_ports());
                }
                // Configure the the control plane switch datapath
                // Redirect all the traffic in the control plane switch to the controller
                else {
                    syslog(LOG_INFO, "Configuring control plane switch");
            		send_datapath_config_message(dpjoin->get_dp_id(), DC_ALL);
                }
            }
            else if (type == VM_MAP) {
                VMMap *mapmsg = dynamic_cast<VMMap*>(&msg);
                syslog(LOG_INFO, "Mapping message arrived from vm=0x%llx", mapmsg->get_vm_id());

                // Search for VM's with no mapping
                map<string, string> query;
                vector<RFEntry> results;
                query[VM_ID] = to_string<uint64_t>(mapmsg->get_vm_id());
                query[VS_ID] = "";
                // Querying for VS_ID is enough, but we could play it safe and query all mapping attributes
                results = this->rftable->get_entries(query);
                if (not results.empty()) {
                    RFEntry entry = results.front();
                    entry.set_vs_id(to_string<uint64_t>(mapmsg->get_vs_id()));
                    entry.set_vs_port(to_string<uint32_t>(mapmsg->get_vs_port()));
                    entry.set_vm_port(to_string<uint32_t>(mapmsg->get_vm_port()));
                    // For now we assume the VM port is the same as the datapath port
                    entry.set_dp_port(to_string<uint32_t>(mapmsg->get_vm_port()));
                    rftable->set_entry(entry);
                }
            }
            else
                // This message is not processed by this processor
                return false;

            // The message was successfully processed
            return true;
        }
};

int main() {
    openlog("rf-server", LOG_NDELAY | LOG_NOWAIT | LOG_PID, SYSLOGFACILITY);
    RFServer s;
    s.start(true);
    return 0;
}
