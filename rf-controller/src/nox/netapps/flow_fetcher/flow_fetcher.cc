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
#include "flow_fetcher.hh"
#include <boost/bind.hpp>
#include <tr1/unordered_map>
#include "assert.hh"
#include "component.hh"
#include "datapath-leave.hh"
#include "error-event.hh"
#include "flow-stats-in.hh"
#include "nox.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

namespace vigil {
namespace applications {

static Vlog_module lg("flow_fetcher");

struct Flow_fetcher_app
    : public container::Component
{
    /* Application interface. */
    Flow_fetcher_app(const container::Context* c,
                     const json_object*)
        : Component(c) { }

    static void getInstance(const container::Context*, Flow_fetcher_app*&);

    void configure(const container::Configuration*);
    void install() { }

    /* Flow-fetcher specifics. */
    typedef std::tr1::unordered_map<uint32_t, boost::shared_ptr<Flow_fetcher> >
        Fetcher_map;
    Fetcher_map active_fetchers;

    boost::shared_ptr<Flow_fetcher>
    start_fetch(datapathid dpid, const ofp_flow_stats_request& request,
                const boost::function<void()>& cb);

    Fetcher_map::iterator lookup_xid(uint32_t xid);
    Disposition handle_flow_stats_in(const Event&);
    Disposition handle_datapath_leave(const Event&);
    Disposition handle_error(const Event&);

    /* Expiration.
     *
     * We time out any query that hasn't had a response in the last 5
     * seconds. */
    static const int EXPIRATION_SECS = 5;
    Co_fsm fsm;
    void expire();
};

void
Flow_fetcher_app::getInstance(const container::Context* ctxt,
                              Flow_fetcher_app*& app)
{
    app = dynamic_cast<Flow_fetcher_app*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Flow_fetcher_app).name())));
}

Flow_fetcher_app::Fetcher_map::iterator
Flow_fetcher_app::lookup_xid(uint32_t xid)
{
    Fetcher_map::iterator i = active_fetchers.end();
    if (xid) {
        i = active_fetchers.find(xid);
    }
    return i;
}

Disposition
Flow_fetcher_app::handle_flow_stats_in(const Event& e)
{
    /* Find matching Flow_fetcher. */
    const Flow_stats_in_event& fsie
        = assert_cast<const Flow_stats_in_event&>(e);
    Fetcher_map::iterator i(lookup_xid(fsie.xid()));
    if (i == active_fetchers.end()) {
        lg.dbg("ignoring Flow_stats_in_event with unknown xid %"PRIu32,
               fsie.xid());
        return CONTINUE;
    }

    /* Append the new flows.  Unless there are more flows to come, declare
     * it a success. */
    boost::shared_ptr<Flow_fetcher> ff(i->second);
    ff->flows.insert(ff->flows.end(), fsie.flows.begin(), fsie.flows.end());
    if (fsie.more) {
        ff->touch();
    } else {
        ff->complete(0);
        active_fetchers.erase(i);
    }
    return STOP;
}

Disposition
Flow_fetcher_app::handle_datapath_leave(const Event& e)
{
    /* Find any Flow_fetchers that match the datapath that's leaving and fail
     * them.  */
    const Datapath_leave_event& dle
        = assert_cast<const Datapath_leave_event&>(e);

    Fetcher_map::iterator i(active_fetchers.begin());
    while (i != active_fetchers.end()) {
        const boost::shared_ptr<Flow_fetcher>& ff(i->second);
        if (ff->datapath_id == dle.datapath_id) {
            lg.warn("flow fetching canceled due to datapath connection drop");
            ff->complete(ENOTCONN);
            i = active_fetchers.erase(i);
        } else {
            ++i;
        }
    }
    return CONTINUE;
}

Disposition
Flow_fetcher_app::handle_error(const Event& e)
{
    const Error_event& ee = assert_cast<const Error_event&>(e);
    Fetcher_map::iterator i(lookup_xid(ee.xid()));
    if (i == active_fetchers.end()) {
        lg.dbg("ignoring Error_event with unknown xid %"PRIu32, ee.xid());
        return CONTINUE;
    }
    boost::shared_ptr<Flow_fetcher> ff(i->second);

    lg.warn("flow fetching canceled due to received error type=%"PRIu16" "
            "code=%"PRIu16, ee.type, ee.code);
    ff->complete(EREMOTE);
    active_fetchers.erase(i);
    return STOP;
}

void
Flow_fetcher_app::expire()
{
    Fetcher_map::iterator i(active_fetchers.begin());
    while (i != active_fetchers.end()) {
        const boost::shared_ptr<Flow_fetcher>& ff(i->second);
        if (time_msec() >= ff->expires) {
            lg.warn("flow fetching canceled due to timeout");
            ff->complete(ETIMEDOUT);
            i = active_fetchers.erase(i);
        } else {
            ++i;
        }
    }
    co_timer_wait(do_gettimeofday() + timeval_from_ms(5000), NULL);
    co_fsm_block();
}

boost::shared_ptr<Flow_fetcher>
Flow_fetcher_app::start_fetch(datapathid dpid,
                              const ofp_flow_stats_request& request,
                              const boost::function<void()>& cb)
{
    uint32_t xid = nox::allocate_openflow_xid();
    boost::shared_ptr<Flow_fetcher> ff(new Flow_fetcher(this, dpid, xid, cb));

    /* Compose the ofp_stats_request header. */
    Array_buffer b(0);
    ofp_stats_request osr;
    osr.header.version = OFP_VERSION;
    osr.header.type = OFPT_STATS_REQUEST;
    osr.header.xid = ff->xid;
    osr.type = htons(OFPST_FLOW);
    osr.flags = htons(0);
    size_t body_ofs = offsetof(ofp_stats_request, body);
    memcpy(b.put(body_ofs), &osr, body_ofs);

    /* Copy the user-provided ofp_flow_stats_request, zeroing the pad
     * bytes just in case the caller failed to do so. */
    ofp_flow_stats_request fsr = request;
    memset(&fsr.match.pad1, 0, sizeof fsr.match.pad1);
    memset(&fsr.match.pad2, 0, sizeof fsr.match.pad2);
    memset(&fsr.pad, 0, sizeof fsr.pad);
    memcpy(b.put(sizeof request), &request, sizeof request);

    /* Send. */
    ofp_header& oh = b.at<ofp_header>(0);
    oh.length = htons(b.size());
    int error = send_openflow_command(dpid, &oh, false);
    if (!error) {
        active_fetchers.insert(std::make_pair(ff->xid, ff)); 
    } else {
        nox::post_timer(boost::bind(&Flow_fetcher::complete, ff, error));
    }
    return ff;
}

void
Flow_fetcher_app::configure(const container::Configuration*)
{
    register_handler<Flow_stats_in_event>
        (boost::bind(&Flow_fetcher_app::handle_flow_stats_in, this, _1));
    register_handler<Datapath_leave_event>
        (boost::bind(&Flow_fetcher_app::handle_datapath_leave, this, _1));
    register_handler<Error_event>
        (boost::bind(&Flow_fetcher_app::handle_error, this, _1));
    fsm.start(boost::bind(&Flow_fetcher_app::expire, this));
}

/* This is the function to call to start fetching a set of flows.  Flows
 * satisfying the predicate in 'request' will be fetched from the switch with
 * datapath id 'dpid'.  When the fetch completes (either successfully or with
 * an error), 'cb' will be invoked.  (But 'cb' will not be invoked before
 * fetch() returns.)
 *
 * Returns a flow fetcher object whose state may be queried to find out the
 * results of the fetch operation or to cancel the operation.  A call to the
 * callback indicates that the flow fetch operation is complete and that the
 * flow fetcher's state will no longer change. */
boost::shared_ptr<Flow_fetcher>
Flow_fetcher::fetch(const container::Context* ctx,
                    datapathid dpid, const ofp_flow_stats_request& request,
                    const boost::function<void()>& cb)
{
    Flow_fetcher_app* app;
    Flow_fetcher_app::getInstance(ctx, app);
    return app->start_fetch(dpid, request, cb);
}

Flow_fetcher::Flow_fetcher(Flow_fetcher_app* app_,
                           datapathid datapath_id_,
                           uint32_t xid_,
                           const boost::function<void()>& cb_)
    : app(app_),
      status(-1),
      datapath_id(datapath_id_),
      xid(xid_),
      cb(cb_)
{
    touch();
}

/* Cancels fetching the flows, by preventing the callback from being
 * invoked. */
void
Flow_fetcher::cancel()
{
    cb = boost::function<void()>();
}

/* Returns the flow fetch status, which is -1 if flow fetching is not
 * complete, 0 if flow fetching succeeded, and otherwise a positive Unix errno
 * value that hints at the error that occurred.  If the return value is 0,
 * then get_flows() may be used to retrieve the fetched flows. */
int
Flow_fetcher::get_status() const
{
    return status;
}

/* Returns any flows that were received.  Ordinarily, you would only call this
 * after get_status() returns 0.  But sometimes there might be some flows
 * returned even if the query is not yet finished (i.e. some flows have been
 * received but there are more to come) or if the query ultimately failed
 * (e.g. if some flows were received and then the connection dropped). */
const std::vector<Flow_stats>&
Flow_fetcher::get_flows() const
{
    return flows;
}

void
Flow_fetcher::complete(int status_)
{
    assert(status < 0);
    assert(status_ >= 0);
    status = status_;
    if (!cb.empty()) {
        boost::function<void()> save_cb = cb;
        cancel();
        save_cb();
    }
}

void
Flow_fetcher::touch()
{
    expires = time_msec() + Flow_fetcher_app::EXPIRATION_SECS * 1000;
}

REGISTER_COMPONENT(container::Simple_component_factory<Flow_fetcher_app>,
                   Flow_fetcher_app);

} // namespace applications
} // namespace vigil
