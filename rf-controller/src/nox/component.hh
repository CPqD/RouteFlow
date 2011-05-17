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
// ------------------------------------------------------------
// Container API
//
// ------------------------------------------------------------

#ifndef PUBLIC_CONTAINER_HH
#define PUBLIC_CONTAINER_HH 1

#include <list>
#include <vector>
#include <string>
#include <typeinfo>
#include <utility>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_array.hpp>

#include "json_object.hh"

#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"
#include "buffer.hh"
#include "event-dispatcher.hh"
#include "hash_map.hh"
#include "packet-classifier.hh"
#include "timer-dispatcher.hh"
#include "timeval.hh"

namespace vigil { 

class Kernel;
class Switch_Auth; 

namespace container {

typedef std::string Interface_description;
typedef std::string Component_name;

class Configuration;
class Context;

/** \defgroup noxcomponents NOX Components
 *
 * A Component encapsulates specific functionality made available to NOX.
 *
 * NOX applications are generally componsed of cooperating components
 * that provide the required functionality.
 */

/** \ingroup noxcomponents
 *
 * Base class for all components.
 *
 * Component see the component state transitions as instantiation and
 * method calls by the container:
 *
 * - NOT_INSTALLED           - state of every component in the beginning.
 * - DESCRIBED               - component description has been parsed.
 * - LOADED                  - dynamic library loaded (if any).
 * - FACTORY_INSTANTIATED    - component factory constructed
 * - INSTANTIATED            - component constructed.
 * - CONFIGURED              - component configure()'d.
 * - INSTALLED               - component install()'ed.
 * - ERROR
 */
class Component
    : boost::noncopyable {
public:
    Component(const Context*);
    virtual ~Component();

    /** 
     * Configure the component.
     *
     * The method should block until the configuration completes.
     */
    virtual void configure(const Configuration*) = 0;

    /**
     * Start the component.
     *
     * The method should block until the component runs.  Container
     * will not pass any method invocations before starting completes.
     */
    virtual void install() = 0;

    /*
     * Resolve a component interface to a component instance.
     */
    template <typename T>
    inline
    void resolve(T*& c) const {
        T::getInstance(ctxt, c);
    }

    /* Event management methods */

    typedef Disposition Handler_signature(const Event&);
    typedef boost::function<Handler_signature> Event_handler;

    /* Register an event */
    template <typename T>
    inline 
    void register_event() const {
        register_event(T::static_get_name());
    }

    /* Not preferred mechanism for registering an event. */
    void register_event(const Event_name&) const;
    
    /* Register an event handler */
    template <typename T>
    inline 
    void register_handler(const Event_handler& h) const {
        register_handler(T::static_get_name(), h);
    }

    /* Register an event handler */
    void register_handler(const Event_name&, const Event_handler&) const;

    /* Post an event */
    void post(Event*) const;

    /* Timer management methods */
    
    // TODO: move Timer* declaration to be a part of component
    // interface as well

    typedef boost::function<void()> Timer_Callback;

    /* Post a timer to be executed immediately.  */
    Timer post(const Timer_Callback&) const;

    /* Post a timer to be executed after the given duration. */
    Timer post(const Timer_Callback&, const timeval& duration) const;

    void register_switch_auth(Switch_Auth *auth) const; 

    /* Packet receiving methods */
    
    typedef uint32_t Rule_id;

    /**
     * Register a packet match handler.
     *
     * Returns a rule id.
     */ 
    Rule_id register_handler_on_match(uint32_t priority, 
                                      const Packet_expr &,
                                      Pexpr_action) const;

    /* Delete a rule */
    bool unregister_handler(Rule_id rule_id) const;

    /* OpenFlow interaction methods */

    int send_openflow_command(const datapathid&, const ofp_header*, 
                              bool block) const;

    int send_openflow_command(const datapathid&, 
			      boost::shared_array<uint8_t>& of_raw, 
                              bool block) const;
    
    int send_openflow_packet(const datapathid&, uint32_t buffer_id, 
                             const ofp_action_header actions[], 
                             uint16_t actions_len,
                             uint16_t in_port, bool block) const;

    int send_openflow_packet(const datapathid&, uint32_t buffer_id, 
                             uint16_t out_port, uint16_t in_port, 
                             bool block) const;
    
    int send_openflow_packet(const datapathid&, const Buffer&, 
                             uint16_t out_port, uint16_t in_port, 
                             bool block) const;
    
    int send_openflow_packet(const datapathid&, const Buffer&, 
                             const ofp_action_header actions[], 
                             uint16_t actions_len,
                             uint16_t in_port, bool block) const;

    int close_openflow_connection(const datapathid&);


    int send_switch_command(datapathid dpid, const std::string&, 
                            const std::vector<std::string>);
    int switch_reset(const datapathid&);
    int switch_update(const datapathid&);

    int send_add_snat(const datapathid &dpid, uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, 
                    uint16_t mac_timeout=0);
    int send_del_snat(const datapathid &dpid, uint16_t port);
    uint32_t get_switch_controller_ip(const datapathid &dpid);
    uint32_t get_switch_ip(const datapathid &dpid);


    /* Context to access the container */
    const Context* ctxt;
};

/* Components can not interact through static variables.  For
 * accessing the container and to discover other components, container
 * passes a context instance for them.
 */
class Context {
public:
    virtual ~Context();

    /* Return the component's configured human-readable name */
    virtual Component_name get_name() const = 0;

    /* Find a component and return a pointer to its instance.  Returns
       0 if not found. */
    virtual Component* get_by_name(const Component_name&) const = 0;

    /* Find a component and return a pointer to its instance.  Returns
       0 if not found. */
    virtual Component* get_by_interface(const Interface_description&) const = 0;

    /* Get a pointer to the kernel the component is installed into. */
    virtual Kernel* get_kernel() const = 0;
};

/* Applications provide a component factory for the container.
 *
 * While loading the component in, the container asks for a factory
 * instance by calling get_factory() and then constructs (and destroys)
 * all the component instances using the factory.
 */
class Component_factory {
public:
    Component_factory();
    virtual ~Component_factory();
    virtual Component* instance(const Context*, 
                                    const json_object*) const = 0;
    virtual Interface_description get_interface() const = 0;
    virtual void destroy(Component*) const = 0;
};
    
typedef std::string Component_argument;
typedef std::list<Component_argument> Component_argument_list;

/* Container passes an application configuration object while the
   component is being configured.  The object contains both parsed XML
   configuration (TBD) and any command line arguments defined for the
   component by the user. */
class Configuration {
public:
    virtual ~Configuration();
    
    /* Retrieve a value of an XML configuration key. */
    virtual const std::string get(const std::string& key) const = 0;

    /* Return true if the XML key exists. */
    virtual const bool has(const std::string& key) const = 0;

    /* Return all XML keys. */
    virtual const std::list<std::string> keys() const = 0;
    
    /* Return all command line arguments of the component. */
    virtual const Component_argument_list get_arguments() const = 0;

    /* Return list of arguments. */
    virtual const hash_map<std::string, std::string> 
    get_arguments_list(char d1 = ',', char d2 = '=') const = 0;
};

/* Basic component factory template for simple application needs. */
template <typename T>
class Simple_component_factory
    : public Component_factory {
public:
    Simple_component_factory(const Interface_description& i_) : i(i_) { }
    Simple_component_factory() : i(typeid(T).name()) { }

    Component* instance(const Context* c, 
                        const json_object* conf) const { 
        return new T(c, conf);
    }

    Interface_description get_interface() const { return i; }

    void destroy(Component*) const { 
        throw std::runtime_error("not implemented"); 
    }

private:
    const Interface_description i;
};

/* If the compiler is not provided with a factory function name, use empty
   component name. */
#ifndef __COMPONENT_FACTORY_FUNCTION__
#define __COMPONENT_FACTORY_FUNCTION__ get_factory
#endif

/* Register components implemented as dynamic libraries */
#define REGISTER_COMPONENT(COMPONENT_FACTORY, COMPONENT_CLASS)         \
    extern "C"                                                         \
    vigil::container::Component_factory*                               \
    __COMPONENT_FACTORY_FUNCTION__() {                                 \
        static vigil::container::Interface_description                 \
            i(typeid(COMPONENT_CLASS).name());                         \
        static COMPONENT_FACTORY f(i);                                 \
        return &f;                                                     \
    }

} // namespace container
} // namespace vigil

#endif
