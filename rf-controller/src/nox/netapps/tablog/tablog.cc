#include "tablog.hh"
#include "messenger/jsonmessenger.hh"
#include "assert.hh"
#include <boost/bind.hpp>
#include <stdlib.h>
#include <fstream>
#include <iostream>

namespace vigil
{
  static Vlog_module lg("tablog");

  tablog::tablog(const Context* c, const json_object* node)
    : Component(c)
  {
    for (int i = 0; i < TABLOG_MAX_AUTO_DUMP; i++)
      records[i].name = "";
  }

  tablog::~tablog()
  {
    //Clear type and name
    hash_map<string, type_name_list>::iterator i = tab_type_name.begin();
    while (i != tab_type_name.end())
    {
      i->second.clear();
      i++;
    }
    
    //Clear values
    hash_map<string, list<value_list> >::iterator j = tab_value.begin();
    while (j != tab_value.end())
    {
      clear_value_list(j->second);
      j++;
    }
  }

  void tablog::configure(const Configuration* c) 
  { 
    register_handler<JSONMsg_event>
      (boost::bind(&tablog::handle_msg_event, this, _1));
  }
  
  void tablog::install()
  {  }

  Disposition tablog::handle_msg_event(const Event& e)
  {
    const JSONMsg_event& jme = assert_cast<const JSONMsg_event&>(e);
    json_dict::iterator i;
   
    //Filter all weird messages
    if (jme.jsonobj->type != json_object::JSONT_DICT)
      return STOP;

    json_dict* jodict = (json_dict*) jme.jsonobj->object;
    i = jodict->find("type");
    if (i != jodict->end() &&
	i->second->type == json_object::JSONT_STRING &&
	*((string *) i->second->object) == "tablog" )
    {
      i = jodict->find("command");
      if (i != jodict->end() &&
	  i->second->type == json_object::JSONT_STRING &&
	  *((string *) i->second->object) == "rotate" )
	rotate_logs();
    }
    
    return CONTINUE;
  }

  int tablog::create_table(const char* name,
			   type_name_list fields)
  {
    hash_map<string, type_name_list>::iterator i = tab_type_name.find(name);
    if (i == tab_type_name.end())
    {
      tab_type_name.insert(make_pair(string(name), fields));
      tab_value.insert(make_pair(string(name), list<value_list>()));
      return 0;
    }
    else
    {
      VLOG_ERR(lg, "Attempt to create table %s again", name);
      return 1;
    }
  }

  int tablog::delete_table(const char* name)
  {
    hash_map<string, type_name_list>::iterator i = tab_type_name.find(name);
    if (i != tab_type_name.end())
    {
      //Delete mapping
      i->second.clear();
      tab_type_name.erase(i);

      //Delete values
      hash_map<string, list<value_list> >::iterator j = tab_value.find(name);
      if (j != tab_value.end())
      {
	clear_table(name);
	tab_value.erase(j);
      }

      //Delete file mapping
      hash_map<string, tablog::table_file_option>::iterator k = tab_file.find(name);
      if (k != tab_file.end())
	tab_file.erase(k);

      return 0;
    }
    else
    {
      VLOG_ERR(lg, "Attempt to delete non-existing table %s", name);
      return 1;
    }
  }

  int tablog::clear_table(const char* name)
  {
    hash_map<string, list<value_list> >::iterator j = tab_value.find(name);
    if (j != tab_value.end())
    {
      if (j->second.size() != 0)
	VLOG_INFO(lg, "Removing %zu values when clearing table %s",
		  j->second.size(), name);
      clear_value_list(j->second);

      return 0;
    }
    else
      return 1;
  }

  int tablog::insert(const char* name, value_list values)
  {
    hash_map<string, list<value_list> >::iterator i = tab_value.find(name);
    if (i != tab_value.end())
    {
      i->second.push_back(values);
      return 0;
    }
    else
    {
      VLOG_ERR(lg, "Values inserted into non-existing table %s dropped", 
	       name);
      return 1;
    }
  }

  int tablog::insert(int id, value_list values)
  {
    if (records[id].name == "")
    {
      VLOG_ERR(lg, "Id %d is not configured for auto dump", id);
      return 1;
    }

    const char* name = records[id].name.c_str();
    int ret = insert(name, values);
    if (size(name) >= records[id].threshold)
      switch (records[id].method)
      {
      case DUMP_FILE:
	dump_file(name, records[id].clearContent, records[id].d);
	break;
      case DUMP_SCREEN:
	dump_screen(name, records[id].clearContent, records[id].d);	
	break;
      }
    
    return ret;
  }

  size_t tablog::size(const char* name)
  {
    hash_map<string, list<value_list> >::iterator i = tab_value.find(name);
    if (i != tab_value.end())
      return i->second.size();
    else
    {
      VLOG_ERR(lg, "Non-existing table %s has size 0!", name);
      return 0;
    }
  }

  string tablog::field_names(type_name_list tnl, string d)
  {
    string outString = "";
    type_name_list::iterator i = tnl.begin();
    while (i != tnl.end())
    {
      outString += i->second;
      i++;
      if (i != tnl.end())
	outString += d;
    }
 
    return outString;
  }

  string tablog::field_value(tablog::tablog_type type, void* val)
  {
    char buf[128];

    switch(type)
    {
    case TYPE_STRING:
      return string(*((string *) val));

    case TYPE_UINT64_T:
      sprintf (buf, "%llu", *((uint64_t *) val)); 
	return string(buf);

    case TYPE_UINT32_T:
      sprintf (buf, "%u", *((uint32_t *) val)); 
	return string(buf);

    case TYPE_UINT16_T:
      sprintf (buf, "%u", *((uint16_t *) val)); 
	return string(buf);
    
    case TYPE_UINT8_T:
      sprintf (buf, "%u", *((uint8_t *) val)); 
	return string(buf);
    }

    return "";
  }

  string tablog::field_values(type_name_list tnl, value_list vl, string d)
  {
    string outString = "";
    
    type_name_list::iterator i = tnl.begin();
    value_list::iterator j = vl.begin();
    while (i != tnl.end())
    {
      if (j == vl.end())
      {
	VLOG_ERR(lg, "Missing values in table");
	return outString+"... missing values";
      }

      outString += field_value(i->first, *j);
      
      i++;
      j++;
      if (i != tnl.end())
	outString += d;
    }

    return outString;
  }

  int tablog::set_auto_dump(const char* name, uint16_t threshold,
			    tablog::tablog_dump method, 
			    bool clearContent, string d)
  {
    //Find free id
    int id = 0;
    while (records[id].name != "")
    {
      id++;
      if (id == TABLOG_MAX_AUTO_DUMP)
	return -1;
    }
    
    //Set dump parameter
    records[id].name = string(name);
    records[id].threshold = threshold;
    records[id].method = method;
    records[id].clearContent = clearContent;
    records[id].d = d;

    return id;
  }

  void tablog::prep_dump_file(const char* name, const char* filename,
			      bool output_header, uint32_t max_log_size,
			      string d)
  {
    //Check table exists
    hash_map<string, type_name_list>::const_iterator j = \
      tab_type_name.find(name);
    if (j == tab_type_name.end())
    {
      VLOG_WARN(lg, "Dumping prep aborted for unknown table %s", name);
      return;
    }
    
    //Record mapping
    string fieldnames =  field_names(j->second,d);
    hash_map<string, tablog::table_file_option>::iterator i = tab_file.find(name);
    if (i == tab_file.end())
      tab_file.insert(make_pair(string(name), 
				tablog::table_file_option(string(filename), 
							  max_log_size,
							  output_header,
							  fieldnames)));
    else
    {
      i->second.filename = string(filename);
      i->second.rotate_size = max_log_size;
      i->second.output_header = output_header;
      i->second.header = fieldnames;
    }

    touch_file(filename, output_header, fieldnames);
  }

  void tablog::touch_file(const char* filename, bool output_header, 
			  string fieldnames)
  {
    ifstream inp;
    ofstream out;
    inp.open(filename, ifstream::in);
    inp.close();
    if(inp.fail())
    {
      inp.clear(ios::failbit);
      out.open(filename, ofstream::out);
      //Output header
      if (output_header)
	out << "#" << fieldnames << endl;
      out.close();
    }
  }

  void tablog::dump_file(const char* name, bool clearContent, string d)
  {
    ofstream out;

    //Check table exists
    hash_map<string, type_name_list>::const_iterator l = \
      tab_type_name.find(name);
    if (l == tab_type_name.end())
    {
      VLOG_WARN(lg, "Dumping file aborted for unknown table %s", name);
      return;
    }

    //Find filename
    hash_map<string, tablog::table_file_option>::iterator i = tab_file.find(name);
    if (i == tab_file.end())
    {
      VLOG_ERR(lg, "Table %s's file dump is not configured.", name);
      return;
    }    

    //Dump values
    hash_map<string, list<value_list> >::const_iterator j = \
      tab_value.find(name);
    if (j != tab_value.end())
    {
      out.open(i->second.filename.c_str(), ofstream::app);
      list<value_list>::const_iterator k = j->second.begin();
      while (k != j->second.end())
      {
	out << field_values(l->second, *k, d) << endl;
	k++;
      }
      out.close();
    }

    //Check size of file and rotate if needed
    fs::path p(i->second.filename.c_str(), fs::native);
    if (fs::file_size(p) >= i->second.rotate_size)
      file_log_rotate(i->second.filename.c_str(),
		      i->second.output_header, 
		      i->second.header);
    
    //Check content if needed
    if (clearContent)
      clear_table(name);
  }

  void tablog::rotate_logs()
  {
    hash_map<string, tablog::table_file_option>::iterator i = tab_file.begin();
    while (i != tab_file.end())
    {
      file_log_rotate(i->second.filename.c_str(),
		      i->second.output_header,
		      i->second.header);
      i++;
    }
  }
  
  void tablog::file_log_rotate(const char* filename, bool output_header,
			       string header)
  { 
    VLOG_DBG(lg, "Rotating log %s", filename);

    //Find largest value
    int i = 0;
    fs::path p = get_int_path(filename, i);
    while (fs::exists(p))
      p = get_int_path(filename, ++i);
   
    //Move files
    while (i >= 0)
    {
      fs::rename(get_int_path(filename,i-1), 
		 get_int_path(filename,i));
      i--;
    }

    //Touch file
    touch_file(filename, output_header, header);
  }
  
  fs::path tablog::get_int_path(const char * filename, int i)
  {
    char buf[128];

    if (i == -1)
      return fs::path(filename, fs::native);
      
    sprintf(buf, "%d", i);
    string s = string(buf);
    string r=s.erase(s.find_last_not_of(" ")+1);
    r = string(filename)+"."+r.erase(0,r.find_first_not_of(" "));
    return fs::path(r.c_str(), fs::native);
  }
  
  void tablog::dump_screen(const char* name, bool clearContent, string d)
  {
    //Dump fields
    hash_map<string, type_name_list>::const_iterator i = \
      tab_type_name.find(name);
    if (i == tab_type_name.end())
    {
      VLOG_WARN(lg, "Dumping aborted for unknown table %s", name);
      return;
    }
    VLOG_INFO(lg, "Dumping table %s", name);
    VLOG_INFO(lg, "%s", field_names(i->second,d).c_str());
    
    //Dump values
    hash_map<string, list<value_list> >::const_iterator j = \
      tab_value.find(name);
    if (j != tab_value.end())
    {
      list<value_list>::const_iterator k = j->second.begin();
      while (k != j->second.end())
      {
	VLOG_INFO(lg, "%s", field_values(i->second, *k,d).c_str());
	k++;
      }
      VLOG_INFO(lg, "===End of dump===");
    }

    //Check content if needed
    if (clearContent)
      clear_table(name);
  }

  void tablog::clear_value_list(list<value_list>& val_list)
  {
    list<value_list>::iterator i = val_list.begin();
    while (i != val_list.end())
    {
      i->clear();
      i++;
    }
    val_list.clear();
  }

  void tablog::getInstance(const Context* c,
				  tablog*& component)
  {
    component = dynamic_cast<tablog*>
      (c->get_by_interface(container::Interface_description
			      (typeid(tablog).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<tablog>,
		     tablog);
} // vigil namespace
