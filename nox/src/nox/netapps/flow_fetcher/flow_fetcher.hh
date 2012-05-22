/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FLOW_FETCHER_HH
#define FLOW_FETCHER_HH 1

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <vector>
#include "component.hh"
#include "flow-stats-in.hh"
#include "netinet++/datapathid.hh"

struct ofp_flow_stats_request;
struct ofp_flow_stats;

namespace vigil {
namespace applications {

class Flow_fetcher_app;

class Flow_fetcher {
public:
    static boost::shared_ptr<Flow_fetcher>
    fetch(const container::Context*, datapathid, const ofp_flow_stats_request&,
          const boost::function<void()>& cb);

    void cancel();
    int get_status() const;
    const std::vector<Flow_stats>& get_flows() const;
private:
    friend class Flow_fetcher_app;

    Flow_fetcher(Flow_fetcher_app* app, 
                 datapathid datapath_id_,
                 uint32_t xid_,
                 const boost::function<void()>& cb);
    void complete(int status);
    void touch();

    Flow_fetcher_app* app;
    int status;
    datapathid datapath_id;
    uint32_t xid;
    std::vector<Flow_stats> flows;
    boost::function<void()> cb;
    long long int expires;
};

} // namespace applications
} // namespace vigil


#endif  //  FLOW_FETCHER_HH
