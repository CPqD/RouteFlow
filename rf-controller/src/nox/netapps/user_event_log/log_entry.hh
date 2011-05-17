#ifndef __LOG_ENTRY_HH__
#define __LOG_ENTRY_HH__

#include "bindings_storage/bindings_storage.hh" 
#include "data/datatypes.hh" 

namespace vigil { 
namespace applications { 

// This class represents a single log message, along with the 
// information that will be used to insert high-level names into
// the log message. 
//
//  The goal of the user_event_log is to let applications easily 
//  generate 'network event log' messages that include high-level 
//  network names.  Essentially, the application provides a format
//  string and one or more network identifiers, and the user_event_log fills
//  in the format string with high-level names associated with those
//  network identifiers.  For example, an application may call the
//  user_event_log with a format string: 
//  "%sh is scanning the local network" and and IP address
//  as a network identifer.  The user_event_log then finds out the 
//  host names associated with that IP address (e.g., 'host1') 
//  and creates the resulting message 'host1 is scanning the local network'.  
//  
//  The user_event_log deals with three types of names: hosts, users, and 
//  locations.  When logging, the application can provide network 
//  identifiers for both a source and destination, meaning that the following
//  format characters are valid:  %sh, %dh, %su, %du, %sl, and %dl.  
//
//  There are a few methods below for more "exotic" uses of the user_event_log
//  but they are mainly related to corner cases that should not be common for
//  most NOX app writers. 
//

class LogEntry { 

  public: 
  
    // Log levels to be used by applications generating log messages
    enum Level { INVALID = 0, CRITICAL, ALERT, INFO }; 
    enum Direction { SRC = 0, DST } ; 

    LogEntry(const std::string &_app, Level _level, const std::string &_msg):
      app(_app), msg(_msg), level(_level) {} 

    // This hard-codes the values used to resolve named format
    // string (currently, su,du,sh,dh,sl,dl)
    // If setName is used in conjunction with an addKey call for
    // the same direction, any names found during a key lookup 
    // with the same direction and type as a name provided to 
    // setName will be ignored.  For example, if one where to call
    // AddMacKey(1,SRC) and setName('bob',USER,SRC), even if there
    // were many other user names associated with mac = 1, the {su}
    // format string would be replaced only with the user name 'bob'
    void setName(int64_t uid, PrincipalType type,Direction dir) {
      PrincipalList &nlist = (dir == SRC) ? src_names : dst_names;
      Principal p; 
      p.id = uid; 
      p.type = type;  
      nlist.push_back(p);
    }

    // This is similar to a setName() call, but handles the situation when
    // the caller does not know the location name, only the DPID and port.  
    // This is really just a hack to handle the case a log entry contains dl/sl
    // but no corresponding bindings exist in Bindings_Storage (e.g., b/c they 
    // just created it themselves).  Unless you create/remove bindings, it is 
    // unlikely that you need to use this function. 
    // Note: if the same Log_Entry object has a location set by both setName()
    // and setNameByLocation(), the name given to setName() takes precedence. 
/*

    void setNameByLocation(const datapathid &dpid, uint16_t port, Direction dir) {
      storage::Query &q = (dir == SRC) ? src_locname_query : dst_locname_query;
      q.clear(); 
      q["principal_type"] = (int64_t) Name::LOCATION; 
      q["dpid"] = (int64_t) dpid.as_host(); 
      q["port"] = (int64_t) port; 
    } 
*/
    // add*Key functions
    // Only the last key supplied for each direction is kept and used.  

    void addLocationKey(const datapathid &dpid, uint16_t port,Direction dir) { 
      storage::Query &q = (dir == SRC) ? src_key_query : dst_key_query;
      q.clear(); 
      q["dpid"] = (int64_t) dpid.as_host(); 
      q["port"] = (int64_t) port; 
    }
    
    void addMacKey(const ethernetaddr &dladdr, Direction dir) { 
      storage::Query &q = (dir == SRC) ? src_key_query : dst_key_query;
      q.clear(); 
      q["dladdr"] = (int64_t) dladdr.hb_long(); 
    }
    
    void addIPKey(uint32_t nwaddr, Direction dir) { 
      storage::Query &q = (dir == SRC) ? src_key_query : dst_key_query;
      q.clear(); 
      q["nwaddr"] = (int64_t) nwaddr; 
    }

    std::string app,msg; 
    Level level;
    storage::Query src_key_query,dst_key_query,
      src_locname_query,dst_locname_query; 
    PrincipalList src_names, dst_names; 
} ; 


}
} 


#endif
