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
#ifndef rttlog_HH
#define rttlog_HH

#include "component.hh"
#include "config.h"
#include "networkstate/switchrtt.hh"
#include "tablog/tablog.hh"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/** Name of table.
 */
#define RTTLOG_TABLE "switchrtt"
/** Default filename for dump.
 */
#define RTTLOG_DEFAULT_FILE "switchrtt.log"
/** Default minimum number of entries before dumping log.
 */
#define RTTLOG_DEFAULT_THRESHOLD 100

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  /** \brief rttlog: Logs switch RTT
   * \ingroup noxcomponents
   * 
   * Logs every switchrtt's interval, i.e., probe all switches once.
   * Can specify filename of dump and threshold to dump by using the 
   * file and threshold arguments respectively, e.g., 
   * <PRE>rttlog=file=myswitchrtt.log,threshold=1000</PRE>.
   *
   * @author ykk
   * @date December 2009
   */
  class rttlog
    : public Component 
  {
  public:
    /** \brief Filename to dump data into.
     */
    string filename;
    /** Threshold to dump file.
     */
    uint16_t threshold;

    /** \brief Constructor of rttlog.
     *
     * @param c context
     * @param node configuration (JSON object)
     */
    rttlog(const Context* c, const json_object* node);

    /** \brief Configure rttlog.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start rttlog.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Dump remaining entries on shutdown
     *
     * @param e shutdown event
     * @return CONTINUE;
     */
    Disposition handle_shutdown(const Event& e);

    /** \brief Get instance of rttlog.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    rttlog*& component);

  private:
    /** \brief Pointer to switchrtt
     */
    switchrtt* srtt;
    /** \brief Pointer to tablog
     */
    tablog* tlog;
    /** Id for auto dump
     */
    int dump_id;
    
    /** \brief Function to log periodically
     */
    void periodic_log();
  };
}

#endif
