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
#ifndef KERNEL_HH
#define KERNEL_HH 1

#include <list>
#include <string>
#include <vector>

#include <boost/function.hpp>

#include "component.hh"
#include "hash_map.hh"
#include "hash_set.hh"

#include <ctime>

namespace vigil {

/* Valid component states. */
enum Component_state { 
    NOT_INSTALLED = 0,        /* Component has been discovered. */
    DESCRIBED = 1,            /* Component (XML) description has been
                                 processed. */
    LOADED = 2,               /* Component library has been loaded, if
                                 necessary. */
    FACTORY_INSTANTIATED = 3, /* Component factory has been
                                 instantiated from the library. */
    INSTANTIATED = 4,         /* Component itself has been
                                 instantiated. */
    CONFIGURED = 5,           /* Component's configure() has been
                                 called. */
    INSTALLED = 6,            /* Component's install() has been
                                 called. */
    ERROR = 7                 /* Any state transition can lead to
                                 error state, if an unrecovarable
                                 error is occurs. */
};

class Component_context;
class Deployer;

typedef std::list<Component_context*> Component_context_list;
typedef std::list<Deployer*> Deployer_list;
typedef hash_map<container::Component_name, 
                 Component_context*> Component_name_context_map;
typedef std::vector<Component_context*> Component_context_vector;
typedef hash_map<int, Component_context_vector*> 
Component_state_context_set_map;

class state_change_error 
    : public std::runtime_error {
public:
    explicit state_change_error(const std::string&);
};

/* NOX kernel is a dependency management engine.  It does nothing else
   but maintains the inter-component dependencies and takes care of
   installing any necessary components other components depend on. 
   
   TODO: make the kernel monitor. not strictly necessary due to the
   co-threads, but still...
*/
class Kernel {
public:
    /* Time in seconds since boot. */
    time_t uptime() const;

    /* Get the universally unique identifier of this NOX instance. */
    typedef uint64_t UUID;
    UUID uuid() const;

    /* How many times NOX container has been rebooted since its
       installation. */
    uint64_t restarts() const;

    /* Attach a deployer to be used if an unknown component is asked
       to be installed. */
    void attach_deployer(Deployer*);

    /* Get deployers */
    Deployer_list get_deployers() const;

    /* Retrieve a component context. Return 0 if no such component
       context known. Note this guarantees nothing about the state of
       the given component. */
    Component_context* get(const container::Component_name&) const;

    /* Retrieve a component context. Return 0 if no such component
       context not known or not in the required state.*/
    Component_context* get(const container::Component_name&,
                           const Component_state) const;

    /* Retrieve all component contexts, regardless of their state. */
    Component_context_list get_all() const;

    /* Install a component by delegating the installation process to
       attached deployers. Throws an exception if no component can be
       found. */
    void install(const container::Component_name&, const Component_state)
        throw(state_change_error);

    /* Install a component to a given state. */
    void install(Component_context*, const Component_state to_state);
    
    /* Change the state of a component. Throws an exception if a fatal
       error occurs during the state transition(s). */
    void change(Component_context*, const Component_state to_state)
        throw(state_change_error);

    void set_arguments(const container::Component_name&, 
                       const container::Component_argument_list&);

    const container::Component_argument_list 
    get_arguments(const container::Component_name&) const;

    /* Get global kernel singleton.  Note, this is *not* for general
       use, but for unit test framework integration and NOX platform
       internal use only. */
    static Kernel* get_instance();

    /* Initialize the global kernel singleton.  Note, this is for the
       NOX platform to use only. */
    static void init(const std::string&, int argc, char** argv);

    /* Keep argc and argv around (primarily tro pass to python */
    int argc;
    char** argv;

private:
    /* Currently, kernel is a singleton and . */
    Kernel(const std::string&, int argc, char** argv);

    /* Attempt to resolve dependencies. */
    void resolve();

    /* Attempt to resolve dependencies from a state to next one. */
    bool resolve(const Component_state from, const Component_state to);

    /* Attempt to resolve dependencies of a given set of contexts to
       next state. */
    Component_context_vector resolve(const Component_context_vector&,
                                     const Component_state to);

    /* Deployers to try if a new component context needs to be
       created. */
    Deployer_list deployers;

    /* Every context */
    Component_name_context_map contexts;

    /* Per state context */
    Component_state_context_set_map per_state;

    /* Application properties */
    typedef hash_map<container::Component_name, 
                     container::Component_argument_list> 
    Component_arguments_map;
    Component_arguments_map arguments;

    /* Set if a context required state has been changed while
       resolving. */
    bool state_requirements_changed;

    /* Set if the kernel is installing a component context right
       now. */
    bool installing;

    /* start time. used for uptime */
    time_t start_time;

    /* Persistent container information unique to this container instance.
     * Retrieved and modified from/in a persistent storage in the
     * beginning of the boot sequence.  If nothing exists on the storage,
     * the container creates a new info block with random UUID. */
    struct container_info {
        /* Universally Unique Identifier (UUID).  Not 128-bits as defined
         * in the RFC 4122, but 64-bits for the convenience. */
        uint64_t uuid;
        
        /* The number of times the container has been restarted. */
        uint64_t restart_counter;
    };
    container_info info;
};

/* An abstract dependency. */
class Dependency {
public:
    virtual ~Dependency();

    /* Attempts to resolve the dependency, and executes any necessary
       actions. Returns true if the dependency has been met.*/
    virtual bool resolve(Kernel*, const Component_state to_state) = 0;

    /* Get a human readable status report. */
    virtual std::string get_status(Kernel*) const = 0;

    /* Get a human readable error message about the dependency failure
       (if any). */
    virtual std::string get_error_message(Kernel*,
                                          hash_set<Component_context*>)
        const = 0;
};

/* A basic name dependency is met only when the given component has
   been installed. */
class Name_dependency 
    : public Dependency {
public:
    Name_dependency(const container::Component_name&);
    bool resolve(Kernel*, const Component_state);
    std::string get_status(Kernel*) const;
    std::string get_error_message(Kernel*, hash_set<Component_context*>) const;

private:
    const container::Component_name name;
};

/* Kernel does not directly operate on components, but on component
   contexts, which contain all per component information - including
   the component instance itself. */
class Component_context
    : public container::Context {
public:
    Component_context(Kernel*);
    virtual ~Component_context();

    /* State transition management methods */
    virtual void install(const Component_state to_state);
    virtual void uninstall(const Component_state to_state);

    /* Attempts to resolve the dependencies for the given state.
       Returns true if all the dependencies are met. */
    bool resolve(Kernel*, const Component_state);
    
    /* Accessors for member variables */
    container::Interface_description get_interface() const;
    container::Component* get_instance() const;
    Component_state get_state() const;
    Component_state get_required_state() const; 
    void set_required_state(const Component_state); 

    /* Return a human-readable status report. */
    std::string get_status() const;
    std::string get_error_message(hash_set<Component_context*>) const;

    /* Public container::Context methods to be provided for components. */
    container::Component_name get_name() const;
    container::Component* get_by_name(const container::Component_name&) const;
    container::Component* 
    get_by_interface (const container::Interface_description&) const;
    Kernel* get_kernel() const;

protected:
    /* Human readable component name */
    container::Component_name name;

    /* Interface identifier */
    container::Interface_description interface;

    /* Component home directory */
    std::string home_path;

    /* Current and required component states */
    Component_state current_state;
    Component_state required_state;
    std::string error_message;

    /* If has been instantianted, the actual component instance */
    container::Component* component;

    /* Component description, typically loaded from disk */
    json_object* json_description;

    /* Configuration merging the static XML component description and
       dynamic runtime-configuration */
    container::Configuration* configuration;

    /* Any component dependencies */
    typedef std::list<Dependency*> Dependency_list; 
    Dependency_list dependencies;
    
    /* Action implementations */
    typedef boost::function<void()> Action_callback;
    hash_map<int, Action_callback> install_actions;

    /* Kernel hosting the context */
    Kernel* kernel;
};

}

#endif 
