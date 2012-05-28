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
#include <vector>
#include <iostream>
#include <sstream>
#include "stats_stream.hh"

Stats_stream::Stats_stream(int num_datapaths, stats_pkt_request_type type) {
  std::string response_string;

  switch(type) {
    case SPRT_FEATURES:
      response_string = "features_reply";
      break;
    case SPRT_PORT_STATS:
      response_string = "port_stats";
      break;
    default:
      response_string = "unexpected_type";
      break;
  }

  msg << "{\"type\":\"" << response_string << "\",\"datapaths\":[";
  datapaths_left = num_datapaths;
  appending = 0;
}

void Stats_stream::send_to_clients() {
  msg << "]}\0";
  msg.flush();

  std::vector<vigil::Msg_stream*>::iterator iter;
  for(iter = clients_.begin(); iter != clients_.end(); ++iter) {
    (*iter)->send(msg.str());
  }
}

void Stats_stream::add(vigil::Msg_stream *client) {
  clients_.push_back(client);
}
