#ifndef JSON_OBJECT_HH
#define JSON_OBJECT_HH

#include "JSON_parser.h"
#include "hash_map.hh"
#include <stdint.h>
#include <stdlib.h>
#include <list>
#include <string.h>
#include <sstream>

namespace vigil
{
  /** \brief JSON class
   *
   * Uses Jean Gressmann's 
   * <A HREF="http://fara.cs.uni-potsdam.de/~jsg/json_parser/">JSON_Parser</A>
   * with license in embedded JSON_parser.c.
   *
   * Uses hash_map for dictionary and STL list for array.
   *
   * Copyright (C) Stanford University, 2010.
   * @author ykk
   * @date February 2010
   * @see json_dict
   * @see json_array
   * @see jsonmessenger
   */
  class json_object
  {
  public:
    /** \brief Constructor
     * @param str string containing JSON
     * @param size length of string
     * @param depth depth of parsing
     */
    json_object(const uint8_t* str, ssize_t& size,
		int depth=20);

    /** \brief Empty constructor
     * @param type_ of object
     */
    json_object(int type_):
      type(type_), object(NULL)
    {}

    /** \brief Destructor
     */
    ~json_object();

    /** \brief String representation
     * @return JSON string
     */
    std::string get_string(bool noquotes=false);

    /** Enumeration of JSON types
     */
    enum json_type
    {
      /** Object is std::list*
       */
      JSONT_ARRAY,
      /** Object is hash_map<string, json_object*>
       */
      JSONT_DICT,
      /** Object is int*
       */
      JSONT_INTEGER,
      /** Object is float*
       */
      JSONT_FLOAT,
      /** Object is NULL
       */
      JSONT_NULL,
      /** Object is bool*
       */
      JSONT_BOOLEAN,
      /** Object is string*
       */
      JSONT_STRING
    };
    /** Type of object
     * Can be any one of the json_type.
     */
    int type;
    /** Reference to object
     */
    void* object;

  private:
    /** \brief Callback for parsing
     * @param ctx context
     * @param type type
     * @param value value
     */
    static int cb(void* ctx, int type, const JSON_value* value);
    /** Last key.
     */
    std::string lastkey;
    /** Stack
     */
    std::list<json_object*> mystack;
  };

  /** Uses hash map for dictionary
   */
  typedef hash_map<std::string, json_object*> json_dict;

  /** Uses STL list for array
   */
  typedef std::list<json_object*> json_array;
}
#endif
