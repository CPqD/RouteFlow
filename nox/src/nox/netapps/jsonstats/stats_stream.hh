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
#ifndef STATS_STREAM_HH
#define STATS_STREAM_HH

#include "messenger_core.hh"

enum stats_pkt_request_type {
  SPRT_FEATURES,
  SPRT_PORT_STATS,
  SPRT_MAX
};

class Stats_stream {
  std::vector<vigil::Msg_stream*> clients_;
public:
  int datapaths_left;
  std::ostringstream msg;
  int appending;

  Stats_stream(int num_datapaths, stats_pkt_request_type type);
  void send_to_clients();
  void add(vigil::Msg_stream *client);
};

#endif /* STATS_STREAM_HH */
