#include "json_object.hh"

#include <iostream>   //remove
using namespace std;  //remove

namespace vigil
{
  json_object::json_object(const uint8_t* str, ssize_t& size, 
			   int depth)
  {
    object=NULL;
    type=JSONT_NULL;

    //Configure parser
    JSON_config config;
    struct JSON_parser_struct* jc = NULL;

    init_JSON_config(&config);
    config.depth = depth;
    config.callback = &cb;
    config.callback_ctx = this;
    config.allow_comments = 1;
    config.handle_floats_manually = 0;
    
    jc = new_JSON_parser(&config);

    //Parse string
    for (int i = 0; i < size; i++)
    {
      if (!JSON_parser_char(jc, (int) *str))
      {
      cout<<"json syntax error at:\n"<<str; // remove / return this as error?
	delete_JSON_parser(jc);
	type = JSONT_NULL;
	return;
      }
      str++;
    }
    if (!JSON_parser_done(jc))
    {
      delete_JSON_parser(jc);
      type = JSONT_NULL;
      return;
    }
  }

  json_object::~json_object()
  {
    json_dict::iterator i;

    if (object != NULL)
      switch(type)
      {
      case JSONT_ARRAY:
	json_array *y;
	y = (json_array *) object;
	y->clear();
	delete y;
	break;
      case JSONT_DICT:
	json_dict *x;
	x = (json_dict *) object;
	i = x->begin();
	while (i != x->end())
	{
	  delete i->second;
	  i++;
	}
	delete x;
	break;
      case JSONT_INTEGER:
	delete (int*) object;
	break;
      case JSONT_FLOAT:
	delete (float*) object;
	break;
      case JSONT_BOOLEAN:
	delete (bool*) object;
	break;
      case JSONT_STRING:
	delete (std::string *)object;
	break;
      }

    type = JSONT_NULL;
    object = NULL;
  }

  std::string json_object::get_string(bool noquotes)
  {
    std::string retStr = "";
    std::ostringstream s;
    json_dict::iterator i;
    json_array::iterator j;

    switch (type)
    {
    case JSONT_ARRAY:
      retStr += "[";
      json_array *y;
      y = (json_array *) object;
      j = y->begin();
      while (j != y->end())
      {
	retStr += (*j)->get_string()+",";
	j++;
      }
      if (retStr.length() > 1)
	retStr = retStr.substr(0,retStr.length()-1);
      retStr += "]";
      break;
    case JSONT_DICT:
      retStr += "{";
      json_dict *x;
      x = (json_dict *) object;
      i = x->begin();
      while (i != x->end())
      {
	retStr += "\""+i->first+"\":"+i->second->get_string()+",";
	i++;
      }
      retStr = retStr.substr(0,retStr.length()-1);
      retStr += "}";
      break;
    case JSONT_INTEGER:
      s << *((int *) object);
      retStr += s.str();
      break;
    case JSONT_FLOAT:
      s << *((float *) object);
      retStr += s.str();
      break;
    case JSONT_NULL:
      retStr += "null";
      break;
    case JSONT_BOOLEAN:
      if (*((bool*) object))
	retStr += "true";
      else
	retStr += "false";
      break;
    case JSONT_STRING:
      if(noquotes){
        retStr += *((std::string *)object); }
      else {
        retStr += "\""+*((std::string *)object)+"\""; }
      break;
    } 
    
    return retStr;
  }

  int json_object::cb(void* ctx, int type, const JSON_value* value)
  {
    json_object* jo = (json_object*) ctx;
    json_array::iterator i = jo->mystack.begin();
    json_object* newobj = NULL;

    switch(type)
    {
    case JSON_T_ARRAY_BEGIN:
      newobj = new json_object(JSONT_ARRAY);
      newobj->object = new json_array();
      jo->mystack.push_front(newobj);
      break;
    case JSON_T_ARRAY_END:
      jo->mystack.pop_front();
      break;
    case JSON_T_OBJECT_BEGIN:
      newobj = new json_object(JSONT_DICT);
      newobj->object = new json_dict();
      jo->mystack.push_front(newobj);
      break;
    case JSON_T_OBJECT_END:
      jo->mystack.pop_front();
      break;
    case JSON_T_KEY:
      jo->lastkey = value->vu.str.value;
      break;
    case JSON_T_STRING:
      newobj= new json_object(JSONT_STRING);
      newobj->object = new std::string(value->vu.str.value);
      break;
    case JSON_T_INTEGER:
      newobj = new json_object(JSONT_INTEGER);
      newobj->object = new int(value->vu.integer_value);
      break;
    case JSON_T_FLOAT:
      newobj = new json_object(JSONT_FLOAT);
      newobj->object = new float(value->vu.float_value);
      break;
    case JSON_T_TRUE:
      newobj = new json_object(JSONT_BOOLEAN);
      newobj->object = new bool(true);
      break;
    case JSON_T_FALSE:
      newobj = new json_object(JSONT_BOOLEAN);
      newobj->object = new bool(false);
      break;
    case JSON_T_NULL:
      newobj = new json_object(JSONT_NULL);
      newobj->object = NULL;
      break;
    }

    if (newobj != NULL)
    {
      if (i == jo->mystack.end())
      {
	jo->type = newobj->type;
	jo->object = newobj->object;
      }
      else
      {
	if ((*i)->type == JSONT_DICT)
	  ((json_dict *) (*i)->object)->insert(std::make_pair(jo->lastkey, newobj));
	else
	  ((json_array *) (*i)->object)->push_back(newobj);
      }
    }

    return 1;
  }
}
