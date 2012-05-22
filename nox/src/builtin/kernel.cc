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
#include "kernel.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "deployer.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;

static Vlog_module lg("kernel");

/* Global kernel singleton */
static Kernel* kernel;

state_change_error::state_change_error(const std::string& msg)
    : runtime_error(msg) {
}

Dependency::~Dependency() {
    
}

Name_dependency::Name_dependency(const Component_name& name_)
    : name(name_) {
    
}

bool
Name_dependency::resolve(Kernel* kernel, const Component_state to_state) {
    Component_context* ctxt = kernel->get(name);
    if (!ctxt) {
        kernel->install(name, INSTALLED);
        return false;
    } else {
        return ctxt->get_state() == INSTALLED;
    }
}

string
Name_dependency::get_status(Kernel* kernel) const {
    Component_context* ctxt = kernel->get(name);
    if (!ctxt) {
        return "'" + name + "' not found";
    }

    return ctxt->get_state() == INSTALLED ? 
        "'" + name + "' OK" :
        "'" + name + "' not installed";
}

static string indent(string s) {
    static const string CRLF("\n");
    for (size_t pos = s.find(CRLF); pos != -1; pos = s.find(CRLF, pos + 1)) {
        s.replace(pos, CRLF.length(), "\n\t");
    }
    
    return s;
}

string
Name_dependency::get_error_message(Kernel* kernel,
                                   hash_set<Component_context*> ctxts_visited) 
    const {
    Component_context* ctxt = kernel->get(name);
    if (!ctxt) {
        return "unmet dependency to '" + name + "': component not found!";
    }

    if (ctxt->get_state() == INSTALLED) {
        return "";
    }

    return indent("\n" + ctxt->get_error_message(ctxts_visited));
}

Component_context::Component_context(Kernel* kernel_) 
    : current_state(NOT_INSTALLED), 
      required_state(NOT_INSTALLED), 
      component(0),
      kernel(kernel_) {
    
}

Component_context::~Component_context() {
    
}

void
Component_context::install(const Component_state to_state) {
    
    if (install_actions[to_state] == 0) {
        throw state_change_error("no action to move to state " + 
                                 boost::lexical_cast<string>(to_state));
    }

    install_actions[to_state]();
}

void
Component_context::uninstall(const Component_state to_state) {
    throw state_change_error("uninstall not implemented");
}

bool
Component_context::resolve(Kernel* kernel, Component_state state) {
    BOOST_FOREACH(Dependency* d, dependencies) {
        if (!d->resolve(kernel, state)) { 
             return false; 
        }
    }
 
    return true;
}

Component_name 
Component_context::get_name() const { 
    return name; 
}

Interface_description 
Component_context::get_interface() const { 
    return interface; 
}

Component* 
Component_context::get_instance() const { 
    return component; 
}

Component_state 
Component_context::get_state() const { 
    return current_state; 
}

Component_state 
Component_context::get_required_state() const { 
    return required_state; 
}

void
Component_context::set_required_state(Component_state to) { 
    required_state = to;
}

static string get_state_string(const Component_state state) {
    switch (state) {    
    case NOT_INSTALLED: return "NOT_INSTALLED";
    case DESCRIBED: return "DESCRIBED";
    case LOADED: return "LOADED";
    case FACTORY_INSTANTIATED: return "FACTORY_INSTANTIATED";
    case INSTANTIATED: return "INSTANTIATED";
    case CONFIGURED: return "CONFIGURED";
    case INSTALLED: return "INSTALLED";
    case ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

string
Component_context::get_status() const {
    string status;
    
    status += "\tCurrent state: " + get_state_string(current_state) + "\n"; 
    status += "\tRequired state: " + get_state_string(required_state) + "\n";
    status += "\tDependencies: ";
    int i = 0;
    BOOST_FOREACH(Dependency* d, dependencies) {
        status += i++ == 0 ? "" : ", ";
        status += d->get_status(kernel);
    }
    status += "\n";

    if (current_state == ERROR) {        
        status += indent(indent("\tError:\n" + error_message)) +  
            "\n";
    }
    return status;
}

container::Component* 
Component_context::get_by_name(const Component_name& name) const {
    Component_context* context = kernel->get(name, INSTALLED);
    return context ? context->get_instance() : 0;
}

container::Component* 
Component_context::get_by_interface(const Interface_description& i) const {
    BOOST_FOREACH(Component_context* ctxt, kernel->get_all()) {
        if (ctxt->get_state() == INSTALLED && i == ctxt->get_interface()) {
            return ctxt->get_instance();
        }
    }

    return 0;
}

Kernel* 
Component_context::get_kernel() const {
    return kernel;
}

string
Component_context::get_error_message(hash_set<Component_context*> ctxts_visited)
    const
{
    if (current_state == ERROR) {
        return "'" + name + "' ran into an error: " + 
            indent("\n" + error_message);
    }

    if (ctxts_visited.find(const_cast<Component_context*>(this)) !=
        ctxts_visited.end()) {
        return "A circular component dependency detected.";
    }

    ctxts_visited.insert(const_cast<Component_context*>(this));

    BOOST_FOREACH(Dependency* d, dependencies) {
        string err = d->get_error_message(kernel, ctxts_visited);
        if (err != "") {
            return "'" + name + "' has an unmet dependency: " + err;
        }
    }

    return "";
}
Kernel::Kernel(const std::string& info_file, int argc_, char** argv_) 
    : argc(argc_), argv(argv_), state_requirements_changed(false),
      installing(false), start_time(::time(0)) 
{
    /* Create a new container info file, if necessary. */
    int fd = ::open(info_file.c_str(), O_RDWR | O_CREAT, S_IRWXU);
    if (fd == -1) {
        throw runtime_error("Unable to open info file '" + info_file + "'.");
    }

    /* Try to read the file, just in case it happened to exist
       before. */
    int c = ::read(fd, &this->info, sizeof(struct container_info));
    if (c != sizeof(struct container_info)) {
        info.restart_counter = 0;

        /* Use the non-blocking random generator, since this is not
           that security critical identifier. */
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 ||
            ::read(fd, &info.uuid, sizeof(UUID)) != sizeof(UUID)) {
            throw runtime_error("Unable to generate a random UUID.");
        }
        info.uuid &= 0x7fffffffffffffffULL;
        ::close(fd);

        lg.dbg("Assigned a new UUID for the container: %"PRIu64, info.uuid);
    }

    /* Increment the counter and save the changes. */
    ++info.restart_counter;

    ::lseek(fd, 0L, SEEK_SET);
    ::write(fd, &info, sizeof(struct container_info));
    ::close(fd);
}

time_t
Kernel::uptime() const {
    return ::time(0) - start_time;
}

Kernel::UUID
Kernel::uuid() const {
    return info.uuid;
}

uint64_t
Kernel::restarts() const {
    return info.restart_counter;
}

void
Kernel::set_arguments(const Component_name& name, 
                      const Component_argument_list& args) {
    arguments[name] = args;
}

const Component_argument_list
Kernel::get_arguments(const Component_name& name) const {
    Component_arguments_map::const_iterator i = arguments.find(name);
    return i == arguments.end() ? Component_argument_list() : i->second;
}

void 
Kernel::attach_deployer(Deployer* deployer) {
    deployers.push_back(deployer);
}

Deployer_list
Kernel::get_deployers() const {
    return deployers;
}

Component_context* 
Kernel::get(const Component_name& name) const {
    Component_name_context_map::const_iterator i = contexts.find(name);
    return i == contexts.end() ? 0 : i->second;
}

Component_context* 
Kernel::get(const Component_name& name, const Component_state state) const {
    Component_name_context_map::const_iterator i = contexts.find(name);
    if (i == contexts.end()) { 
        return 0; 
    } else {
        Component_context* context = i->second;
        return context->get_state() == state ? context : 0;
    }
}

Component_context_list 
Kernel::get_all() const {
    Component_context_list l;
    
    BOOST_FOREACH(Component_name_context_map::value_type v, contexts) {
        l.push_back(v.second);
    }

    return l;
}

void 
Kernel::install(const Component_name& name, const Component_state to) 
    throw(state_change_error) {
    if (contexts.count(name) != 0) {
        throw state_change_error("Application '" + name + 
                                 "' was tried to be installed multiple times.");
    }

    BOOST_FOREACH(Deployer* d, deployers) {
        if (d->deploy(this, name)) {
            change(contexts[name], to);
            return;
        }
    }
    
    throw state_change_error("Application '" + name + 
                             "' description not found.");
}

void 
Kernel::install(Component_context* ctxt, const Component_state to) {
    if (contexts.count(ctxt->get_name()) != 0) {
        throw state_change_error("component already installed");
    }

    contexts[ctxt->get_name()] = ctxt;
    
    if (per_state.find(NOT_INSTALLED) == per_state.end()) {
        per_state[NOT_INSTALLED] = new Component_context_vector();
    }
    
    per_state[NOT_INSTALLED]->push_back(ctxt);
    
    change(ctxt, to);
}

void
Kernel::change(Component_context* ctxt, const Component_state to) 
    throw(state_change_error) {
    const Component_state from = ctxt->get_state();
    
    if (from == to) {
        return;
    }
    
    if (from > to) {
        throw state_change_error("uninstalling not implemented");
    }

    if (contexts.count(ctxt->get_name()) != 1) {
        throw state_change_error("unknown context");
    }

    ctxt->set_required_state(to);
    
    if (installing) {
        state_requirements_changed = true;
    } else {
        resolve();

        if (!get(ctxt->get_name(), to)) {
            hash_set<Component_context*> c;
            throw state_change_error("Cannot change the state of '" + 
                                     ctxt->get_name() + "' to " + 
                                     get_state_string(to) + ":\n" + 
                                     ctxt->get_error_message(c));
        }
    }
}

void
Kernel::resolve() {
    bool progress = installing = true;

    while (progress || state_requirements_changed) {
        state_requirements_changed = progress = false;

        for (int i = NOT_INSTALLED; i < INSTALLED && !progress; ++i) {
            Component_state from = Component_state(i);
            Component_state to = Component_state(i + 1);
            progress = resolve(from, to);
        }
    }

    installing = false;
}

bool
Kernel::resolve(const Component_state from, const Component_state to) {
    Component_context_vector* to_install = per_state[from];
    Component_context_vector dependencies_resolved = resolve(*to_install, to);
    
    BOOST_FOREACH(Component_context* ctxt, dependencies_resolved) {
        ctxt->install(to);
        for (Component_context_vector::iterator i = to_install->begin(); 
             i != to_install->end(); ++i) {
            if (*i != ctxt) continue;
                
            to_install->erase(i);
            break;
        }

        if (per_state.find(ctxt->get_state()) == per_state.end()) {
            per_state[ctxt->get_state()] = new Component_context_vector();
        }

        BOOST_FOREACH(Component_context* c, *to_install) {
            assert(c != ctxt);
        }
        per_state[ctxt->get_state()]->push_back(ctxt);
    }

    return !dependencies_resolved.empty();
}

Component_context_vector
Kernel::resolve(const Component_context_vector& contexts,
                const Component_state to) {
    Component_context_vector resolved;

    for (int i = 0; i < contexts.size(); ++i) {
        Component_context* ctxt = contexts[i];

        if (ctxt->resolve(this, to)) { 
            resolved.push_back(ctxt); 
        }
    }

    return resolved;
}

void
Kernel::init(const std::string& info, int argc, char** argv) {
    kernel = new Kernel(info, argc, argv);
}

Kernel*
Kernel::get_instance() {
    return kernel;
}
