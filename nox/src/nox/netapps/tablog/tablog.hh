/* Copyright 2009 (C) Stanford University.
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
#ifndef tablog_HH
#define tablog_HH

#include "component.hh"
#include "config.h"
#include "hash_map.hh"
#include <boost/filesystem/operations.hpp>

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/** Maximum number of automatic dump
 */
#define TABLOG_MAX_AUTO_DUMP 128

namespace vigil
{
  using namespace std;
  using namespace vigil::container;
  namespace fs = boost::filesystem;

  /** \brief tablog: Component that logs tables of information
   * \ingroup noxcomponents
   * 
   * Component dumps data into one of the \ref tablog_dump format.
   *
   * Log rotate can be externally triggered using jsonmessenger.
   * @see #handle_msg_event(const Event& e)
   * 
   * @author ykk
   * @date December 2009
   */
  class tablog
    : public Component 
  {
  public:
    /** \brief Types allowed in tablog fields
     */
    enum tablog_type
    {
      TYPE_STRING,
      TYPE_UINT8_T,
      TYPE_UINT16_T,
      TYPE_UINT32_T,
      TYPE_UINT64_T,
    };
    /** \brief Types of dump available.
     */
    enum tablog_dump
    {
      DUMP_SCREEN,
      DUMP_FILE,
    };
    /** Mapping between type and name
     */
    typedef pair<tablog::tablog_type, string> type_name;
    /** List of type and name
     */
    typedef list<type_name> type_name_list;
    /** List of values
     */
    typedef list<void*> value_list;

    /** \brief Constructor of tablog.
     *
     * @param c context
     * @param node configuration (JSON object) 
     */
    tablog(const Context* c, const json_object* node);

    /** \brief Destructor.
     * 
     * Clear all tables and values.
     */
    ~tablog();

    /** \brief Configure tablog.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start tablog.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Log rotate on ROTATE message
     *
     * <PRE>
     * {
     *   "type" : "tablog",
     *   "command" : "rotate"
     * }
     * </PRE>
     *
     * @param e JSON msg event
     * @return CONTINUE;
     */
    Disposition handle_msg_event(const Event& e);

    /** \brief Create/add table.
     *
     * List of type and name will be freed by component.
     * @see #delete_table
     *
     * @param name name of table, used to refer to it subsequently
     * @param fields fields of table in specific order
     * @return 0 for success and 1 for failure
     */
    int create_table(const char* name, type_name_list fields);

    /** \brief Delete/remove table.
     *
     * Also remove any pending values for table.
     * 
     * @param name name of table
     * @return 0 for success and 1 for failure
     */
    int delete_table(const char* name);

    /** \brief Add values to table
     *
     * List of values will be freed by component.
     *
     * @param name name of table
     * @param values values to insert into table
     * @return 0 for success and 1 for failure
     */
    int insert(const char* name, value_list values);

    /** \brief Add values to table
     *
     * List of values will be freed by component.
     *
     * @param id automatic dump id (return by set_auto_dump)
     * @param values values to insert into table
     * @return 0 for success and 1 for failure
     */
    int insert(int id, value_list values);

    /** \brief Clear table.
     *
     * Remove any pending values for table.
     * 
     * @param name name of table
     * @return 0 for success and 1 for failure
     */
    int clear_table(const char* name);

    /** \brief Check number of pending values.
     *
     * @param name name of table
     * @return number of pending values for table
     */
    size_t size(const char* name);

    /** \brief Set automatic dump.
     *
     * @param name name of table 
     * @param threshold threshold to trigger automatic dump
     * @param method dump method
     * @param clearContent indicate if dump will clear content
     * @param d delimiter to use
     * @return automatic dump id (used for inserting data) else -1 on failure
     */
    int set_auto_dump(const char* name, uint16_t threshold,
		      tablog::tablog_dump method, 
		      bool clearContent=true, string d="\t");

    /** \brief Dump content of log to screen
     *
     * @param name name of table 
     * @param clearContent indicate if dump will clear content
     * @param d delimiter to use
     */
    void dump_screen(const char* name, bool clearContent=true, string d="\t");

    /** \brief Dump content of log to file
     *
     * Need to prepare beforehand.
     * @see prep_dump_file
     *
     * @param name name of table 
     * @param clearContent indicate if dump will clear content
     * @param d delimiter to use
     */
    void dump_file(const char* name, bool clearContent=true, string d="\t");

    /** \brief Configure file dump
     * 
     * If called more than once, the latest call is obeyed.
     * 
     * @param name name of table
     * @param filename name of file to dump
     * @param output_header indicate if header is to be outputed to new file
     * @param max_log_size maximum log size to trigger log rotate
     * @param d delimiter to use
     */
    void prep_dump_file(const char* name, const char* filename, 
			bool output_header, uint32_t max_log_size = 1e9,
			string d="\t");

    /** \brief Rotate all log files.
     */
    void rotate_logs();

    /** \brief Rotate log file.
     * 
     * @param filename name of file to rotate
     * @param output_header indicate of header should be written
     * @param header header string
     */
    void file_log_rotate(const char* filename, bool output_header,
			 string header);
    
    /** \brief Get string of field names.
     *
     * @param tnl type_name_list to read field names from
     * @param d delimiter
     * @return string with field names delimited
     */
    string field_names(type_name_list tnl, string d);

    /** \brief Get string of field valuess.
     *
     * @param tnl type_name_list to read field type from
     * @param vl list of values
     * @param d delimiter
     * @return string with field values delimited
     */
    string field_values(type_name_list tnl, value_list vl, string d);

    /** \brief Get string of value for a field.
     *
     * @param type type of value
     * @param val pointer to value
     * @return string with value of field
     * @see tablog_type
     */
    string field_value(tablog::tablog_type type, void* val);

    /** \brief Get instance of tablog.
     *
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    tablog*& component);

  private:
    /** Structure to hold auto_dump_record.
     */
    struct auto_dump_record
    {
      /** Name of table
       */
      string name;
      /** Threshold to trigger dump
       */
      uint16_t threshold;
      /** Indicate if clear content with dump.
       */
      bool clearContent;
      /** Method to dump
       */
      tablog::tablog_dump method;
      /** Delimiter
       */
      string d;
    };
    /** Automatic dump records
     */
    auto_dump_record records[TABLOG_MAX_AUTO_DUMP];

    /** Table options
     */
    struct table_file_option
    {
      /** \brief Constructor.
       *
       * @param filename_ filename
       * @param rotate_size_ size before rotating log
       * @param output_header_ output header line in log
       * @param header_ header string 
       */
      table_file_option(string filename_, uint32_t rotate_size_,
			bool output_header_, string header_):
	filename(filename_), rotate_size(rotate_size_),
	output_header(output_header_), header(header_)
      { };

      /** Filename
       */
      string filename;
      /** Log rotate size
       */
      uint32_t rotate_size;
      /** Output header line in log file
       */
      bool output_header;
      /** Header string
       */
      string header;
    };

    /** Map of tables and file options.
     */
    hash_map<string, tablog::table_file_option> tab_file;

    /** Map of tables.
     */
    hash_map<string, list<value_list> > tab_value;
    
    /** Map of tables and types-name
     */
    hash_map<string, type_name_list> tab_type_name;

    /** \brief Clear value list.
     *
     * @param val_list list of value
     */
    void clear_value_list(list<value_list>& val_list);

    /** \brief Crate file if it does not exists.
     *
     * @param filename filename
     * @param output_header indicate if header should be written
     * @param fieldnames string of field names
     */
    void touch_file(const char* filename, bool output_header, 
		    string fieldnames);

    /** \brief Get integer string (trimmed)
     * 
     * @param filename filename to append integer to
     * @param i value to trim
     * @return boost filesystem path
     */
    fs::path get_int_path(const char* filename, int i);
  };
}

#endif
