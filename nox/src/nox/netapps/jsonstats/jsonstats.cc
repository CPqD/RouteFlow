/*
 *  Copyright 2012 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include <boost/bind.hpp>
#include <map>
#include "assert.hh"
#include "component.hh"
#include "vlog.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "port-stats-in.hh"
#include "json-util.hh"
#include "jsonmessenger.hh"
#include "timeval.hh"
#include "netinet++/datapathid.hh"
#include "stats_stream.hh"
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>

namespace {

using namespace vigil;
using namespace vigil::container;
using std::map;
using std::pair;
using std::vector;
using std::list;
using std::string;

Vlog_module lg("jsonstats");

const uint32_t DEFAULT_RESPONSE_TIMEOUT = 500; /* milliseconds */

/*
 * This NOX app recieves requests for statistics using jsonmessenger.
 * Statistics are retrieved, collated and sent back to interested clients in an
 * openflow-esque JSON format.
 */
class JSONStats: public Component {
  int response_timeout_;
  hash_map<datapathid,vector<Port> > datapaths_;
  Stats_stream *streams_[SPRT_MAX];
public:
  JSONStats(const Context *c, const json_object*) : Component(c) {
    int i;
    for (i = 0; i < SPRT_MAX; ++i)
      streams_[i] = NULL;

    response_timeout_ = DEFAULT_RESPONSE_TIMEOUT;
  }

  void configure(const Configuration*);
  void install();

  void request_features();
  void request_port_stats();
  void finalise_stream(int request);
  void add_client(stats_pkt_request_type request, Msg_stream *sock);
  void copy_port_information(struct vector<Port> &ports,
                             const Datapath_join_event& dj);

  Disposition handle_datapath_join(const Event& e);
  Disposition handle_datapath_leave(const Event& e);
  Disposition handle_ports_stats(const Event& e);
  Disposition handle_json_event(const Event& e);
};

void create_port_stats_request(ofp_stats_request *osr, size_t size) {
  osr->header.type = OFPT_STATS_REQUEST;
  osr->header.version = OFP_VERSION;
  osr->header.length = htons(size);
  osr->header.xid = htonl(0);
  osr->type = htons(OFPST_PORT);
  osr->flags = htons(0);

  struct ofp_port_stats_request *psr = (ofp_port_stats_request *)(osr->body);
  psr->port_no = htons(OFPP_NONE);
}

/**
 * TODO: Extract configuration items from config object
 *
 * "What configuration items?", you say?
 * - jsonmessenger listen port
 * - response timeout value
 */
void JSONStats::configure(const Configuration* config) {
}

void JSONStats::install() {
  /* Register with NOX for events we're interested in */
  register_handler<JSONMsg_event> (boost::bind(
      &JSONStats::handle_json_event, this, _1));
  register_handler<Datapath_join_event> (boost::bind(
      &JSONStats::handle_datapath_join, this, _1));
  register_handler<Datapath_leave_event> (boost::bind(
      &JSONStats::handle_datapath_leave, this, _1));
  register_handler<Port_stats_in_event> (boost::bind(
      &JSONStats::handle_ports_stats, this, _1));
}

/**
 * Note: We don't check to see if each datapath will support the stats request.
 */
void JSONStats::request_port_stats() {
  size_t msize = sizeof(ofp_stats_request) + sizeof(ofp_port_stats_request);
  boost::shared_array<uint8_t> raw_sr(new uint8_t[msize]);
  ofp_stats_request *osr = (ofp_stats_request *)raw_sr.get();
  create_port_stats_request(osr, msize);

  /* Send OpenFlow Stats Request to all registered datapaths. */
  hash_map<datapathid,vector<Port> >::iterator iter;
  for (iter = datapaths_.begin(); iter != datapaths_.end(); ++iter) {
    VLOG_DBG(lg, "Sent ofp_stats-request to 0x%lx", iter->first.as_host());
    send_openflow_command(iter->first, &osr->header, false);
  }
}

void JSONStats::copy_port_information(vector<Port> &ports,
                                      const Datapath_join_event& dj) {
  ports.reserve(dj.ports.size());
  vector<Port>::const_iterator iter = dj.ports.begin();
  for (; iter != dj.ports.end(); ++iter) {
    ports.push_back(*iter);
  }
}

Disposition JSONStats::handle_datapath_join(const Event& e) {
  const Datapath_join_event& dj = assert_cast<const Datapath_join_event&> (e);

  hash_map<datapathid,vector<Port> >::iterator iter =
      datapaths_.find(dj.datapath_id);
  if (iter != datapaths_.end()) {
    VLOG_INFO(lg, "Duplicate datapath join ignored: 0x%lx",
              dj.datapath_id.as_host());
    return CONTINUE;
  }

  /* Add the datapath and its set of Port features to the hash_map. */
  vector<Port> ports;
  copy_port_information(ports, dj);
  datapaths_.insert( pair<datapathid,vector<Port> >(dj.datapath_id, ports));

  VLOG_DBG(lg, "DP joined: 0x%lx", dj.datapath_id.as_host());

  return CONTINUE;
}

Disposition JSONStats::handle_datapath_leave(const Event& e) {
  const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&> (e);

  datapaths_.erase(dl.datapath_id);
  VLOG_DBG(lg, "DP left: 0x%lx", dl.datapath_id.as_host());

  return CONTINUE;
}

Disposition JSONStats::handle_ports_stats(const Event& e) {
  const Port_stats_in_event& ps = assert_cast<const Port_stats_in_event&> (e);
  datapathid dpid = ps.datapath_id;

  if (streams_[SPRT_PORT_STATS] == NULL) {
    VLOG_DBG(lg, "Dropping unsolicited ofp_port_stats");
    return CONTINUE;
  }

  std::ostringstream& oss = streams_[SPRT_PORT_STATS]->msg;
  VLOG_DBG(lg, "Handling ports_stats event from 0x%lx", dpid.as_host());

  /* Construct our JSON ofp_port_stats message */
  if (streams_[SPRT_PORT_STATS]->appending)
    oss << ",";
  else
    streams_[SPRT_PORT_STATS]->appending = 1;

  oss << "{\"datapath_id\":\"" << dpid.as_host() << "\",\"ports\":[";
  oss.flush();

  std::vector<Port_stats>::const_iterator iter;
  int appending_port = 0;
  for (iter = ps.ports.begin(); iter != ps.ports.end(); ++iter) {
    if (iter->port_no >= OFPP_MAX)
      continue;

    if (appending_port)
      oss << ",";
    else
      appending_port = 1;

    oss << "{\"port_no\":" << iter->port_no << ",";
    oss << "\"rx_packets\":\"" << iter->rx_packets << "\",";
    oss << "\"tx_packets\":\"" << iter->tx_packets << "\",";
    oss << "\"rx_bytes\":\"" << iter->rx_bytes << "\",";
    oss << "\"tx_bytes\":\"" << iter->tx_bytes << "\",";
    oss << "\"rx_dropped\":\"" << iter->rx_dropped << "\",";
    oss << "\"tx_dropped\":\"" << iter->tx_dropped << "\",";
    oss << "\"rx_errors\":\"" << iter->rx_errors << "\",";
    oss << "\"tx_errors\":\"" << iter->tx_errors << "\",";
    oss << "\"rx_frame_err\":\"" << iter->rx_frame_err << "\",";
    oss << "\"rx_over_err\":\"" << iter->rx_over_err << "\",";
    oss << "\"rx_crc_err\":\"" << iter->rx_crc_err << "\",";
    oss << "\"collisions\":\"" << iter->collisions << "\"}";

    oss.flush();
  }

  oss << "]}";
  oss.flush();

  streams_[SPRT_PORT_STATS]->datapaths_left -= 1;
  if (streams_[SPRT_PORT_STATS]->datapaths_left == 0) {
    VLOG_DBG(lg, "%s", oss.str().c_str());
    finalise_stream(SPRT_PORT_STATS);
  }

  return CONTINUE;
}

void JSONStats::finalise_stream(int request) {
  if (streams_[request] != NULL) {
    streams_[request]->send_to_clients();
    delete streams_[request];
    streams_[request] = NULL;
  }
}

/**
 * request_features():
 *
 * Fetch ofp_features information from our hashmap and prepare a response
 */
void JSONStats::request_features() {
  if (streams_[SPRT_FEATURES] == NULL ) {
    VLOG_DBG(lg, "Dropping unsolicited ofp_features_reply");
    return;
  }

  std::ostringstream& oss = streams_[SPRT_FEATURES]->msg;
  VLOG_DBG(lg, "Sending datapath info for %lu datapaths", datapaths_.size());

  int appending_dp = 0;
  hash_map<datapathid,vector<Port> >::iterator map_iter;
  for (map_iter = datapaths_.begin(); map_iter != datapaths_.end(); ++map_iter) {
    if (appending_dp)
      oss << ",";
    else
      appending_dp = 1;

    oss << "{\"datapath_id\":\"" << map_iter->first.as_host() << "\",";
    /* If we were to make a copy of the entire ofp_switch_features msg that we
     * recieve from datapaths, then we could encode the below information into
     * our json replies too. */
    /*oss << "\"n_buffers\":" << map_iter->second.n_buffers << ",";
    oss << "\"n_tables\":" << map_iter->second.n_tables << ",";
    oss << "\"capabilities\":" << map_iter->second.capabilities << ",";
    oss << "\"actions\":" << map_iter->second.actions << ",";*/
    oss << "\"ports\":[";

    int appending_port = 0;
    vector<Port>::iterator port_iter = map_iter->second.begin();
    for(; port_iter != map_iter->second.end(); ++port_iter) {
      if (port_iter->port_no >= OFPP_MAX)
        continue;

      if (appending_port)
        oss << ",";
      else
        appending_port = 1;

      oss << "{\"port_no\":" << port_iter->port_no << ",";
      oss << "\"hw_addr\":\""<< port_iter->hw_addr << "\",";
      oss << "\"name\":\""<< port_iter->name.c_str() << "\",";
      /* speed is made available by vigil::Port, but is not in ofp_phy_port */
      /*oss << "\"speed\":" << port_iter->speed << ",";*/
      oss << "\"config\":"<< port_iter->config << ",";
      oss << "\"state\":"<< port_iter->state << ",";
      oss << "\"curr\":"<< port_iter->curr << ",";
      oss << "\"advertised\":"<< port_iter->advertised << ",";
      oss << "\"supported\":"<< port_iter->supported << ",";
      oss << "\"peer\":"<< port_iter->peer << "}";

      oss.flush();
    }
    oss << "]}";
    oss.flush();

  }

  VLOG_DBG(lg, "%s", oss.str().c_str());

  /* Set this reply to be handled ASAP -- simplifies other code */
  timeval tv = { 0, 1 };
  post(boost::bind(&JSONStats::finalise_stream, this, SPRT_FEATURES), tv);
}

/**
 * add_client():
 *
 * Adds the given Msg_stream to the list of clients for the given stats
 * request type. If no request is currently underway, begin a new one.
 */
void JSONStats::add_client(stats_pkt_request_type request,
                               Msg_stream *sock) {
  /* If there isn't currently a request of this type underway */
  if (streams_[request] == NULL ) {
    streams_[request] = new Stats_stream(datapaths_.size(), request);

    switch (request) {
    case SPRT_FEATURES:
      request_features();
      break;
    case SPRT_PORT_STATS:
      request_port_stats();
      break;
    default:
      VLOG_ERR(lg, "Unexpected request value in add_client()");
      break;
    }

    /**
     * If we request information from x datapaths and recieve <x replies,
     * then our stats_stream would never finish its response and send back.
     * This could be caused by a leaving or unresponsive datapath. Here, we
     * post a timeout to ensure we will always reply to requests (even if only
     * with incomplete response data).
     */
    timeval tv = { response_timeout_ / 1000, response_timeout_ * 1000 };
    post(boost::bind(&JSONStats::finalise_stream, this, request), tv);
  }

  streams_[request]->add(sock);
}

/**
 * handle_json_event():
 *
 * {
 *   // We handle json messages with
 *   "type":"jsonstats",
 *   // and EITHER
 *   "command":"features_request"
 *   // OR
 *   "command":"port_stats_request"
 * }
 */
Disposition JSONStats::handle_json_event(const Event& e) {
  const JSONMsg_event& jme = assert_cast<const JSONMsg_event&> (e);
  json_object *jsonobj = (json_object *)jme.jsonobj.get();

  if (jsonobj->type != json_object::JSONT_DICT)
    return CONTINUE;

  json_dict *jdict = (json_dict *) jsonobj->object;
  json_dict::iterator i = jdict->find("type");
  if (i == jdict->end())
    return CONTINUE;

  if (i->second->type == json_object::JSONT_STRING &&
      strncmp(((string *)i->second->object)->c_str(),
              "jsonstats", strlen("jsonstats")) == 0) {
    VLOG_DBG(lg, "Handling JSON Event (type %d)\n%s", jsonobj->type,
             jsonobj->get_string(false).c_str());

    i = jdict->find("command");
    if (i == jdict->end())
      return CONTINUE;

    if (i->second->type == json_object::JSONT_STRING) {
      if (strncmp(((string *)i->second->object)->c_str(),
          "features_request", strlen("features_request")) == 0) {
        add_client(SPRT_FEATURES, jme.sock);
      } else if (strncmp(((string *)i->second->object)->c_str(),
                 "port_stats_request", strlen("port_stats_request")) == 0) {
        add_client(SPRT_PORT_STATS, jme.sock);
      }
    }
  }

  return CONTINUE;
}

REGISTER_COMPONENT(container::Simple_component_factory<JSONStats>, JSONStats);

} /* anonymous namespace */
