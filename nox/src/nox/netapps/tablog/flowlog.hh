/* Copyright 2010 (C) Stanford University.
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
#ifndef flowlog_HH
#define flowlog_HH

#include "component.hh"
#include "config.h"
#include "tablog.hh"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/** Name of table.
 */
#define FLOWLOG_TABLE "flowrecord"
/** Default filename for dump.
 */
#define FLOWLOG_DEFAULT_FILE "flowrecord.log"
/** Default minimum number of entries before dumping log.
 */
#define FLOWLOG_DEFAULT_THRESHOLD 100

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  /** \brief flowlog: records all flows in network
   * \ingroup noxcomponents
   * 
   * To ensure flow removed is sent (in openflow-default.hh).
   *
   * @author ykk
   * @date January 2010
   */
  class flowlog
    : public Component 
  {
  public:
    /** \brief Filename to dump data into.
     */
    string filename;
    /** Threshold to dump file.
     */
    uint16_t threshold;

    /** \brief Constructor of flowlog.
     *
     * @param c context
     * @param node configuration (JSON object)
     */
    flowlog(const Context* c, const json_object* node);
    
    /** \brief Configure flowlog.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start flowlog.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Handle flow removed event.
     * 
     * @param e flow removed event
     * @return CONTINUE always
     */
    Disposition handle_flow_removed(const Event& e);

    /** \brief Dump remaining entries on shutdown
     *
     * @param e shutdown event
     * @return CONTINUE;
     */
    Disposition handle_shutdown(const Event& e);

    /** \brief Get instance of flowlog.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    flowlog*& component);

  private:
    /** \brief Pointer to tablog
     */
    tablog* tlog;
    /** Id for auto dump
     */
    int dump_id;
  };
}

#endif
