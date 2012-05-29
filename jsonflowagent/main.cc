/*
 *  Copyright 2012 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

#include "mibs/phy_port.hh"
#include "mibs/port_stats.hh"
#include "parser.hh"

#define JFA_BUF_SIZE (32768)

static int keep_running_;
static int poll_interval_ = 5;

const char *features_msg =
    "{\"type\":\"jsonstats\",\"command\":\"features_request\"}";
const char *stats_msg =
    "{\"type\":\"jsonstats\",\"command\":\"port_stats_request\"}";

RETSIGTYPE
stop_server(int a) {
    keep_running_ = 0;
}

int main (int argc, char **argv) {
  int background = 0;
  char buf[JFA_BUF_SIZE];

  /* TODO: Accept commandline arguments */

  snmp_enable_calllog();

  netsnmp_enable_subagent();
  netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

  /* run in background, if requested */
  if (background && netsnmp_daemonize(1, 0))
      exit(1);

  SOCK_STARTUP;
  init_agent("json-flow-agent");

  init_phy_port();
  init_port_stats();

  init_snmp("json-flow-agent");

  /* In case we receive a request to stop (kill -TERM or kill -INT) */
  keep_running_ = 1;
  signal(SIGTERM, stop_server);
  signal(SIGINT, stop_server);

  /* Connect to jsonstats NOX component */
  int status = 0;
  struct addrinfo hints;
  struct addrinfo *sf_info;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  /* TODO: Un-hardcode this NOX JSONmessenger server address */
  if ((status = getaddrinfo("localhost", "2703", &hints, &sf_info)) != 0) {
    snmp_log(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  int noxfd = socket(AF_INET, SOCK_STREAM, 0);
  while (connect(noxfd, sf_info->ai_addr, sf_info->ai_addrlen) != 0) {
    snmp_log(LOG_DEBUG,"Trouble connecting to NOX JSON source");
    sleep(1);
  }

  if (send(noxfd, features_msg, strlen(features_msg), 0) != 0)
    snmp_log(LOG_ERR, "Failed to send features_request");

  snmp_log(LOG_INFO,"json-flow-agent is up and running.\n");

  /* Ready our set of fds */
  fd_set fdset;
  int numfds = (noxfd + 1);
  int block = 0, count = 0;

  /* Set up our timer */
  struct timeval expire_time,timer,workaround_timer;
  timerclear(&timer);
  timerclear(&expire_time);
  timerclear(&workaround_timer);
  timer.tv_sec = poll_interval_;

  size_t len = 0;
  while(keep_running_) {
    FD_ZERO(&fdset);
    FD_SET(noxfd, &fdset);

    gettimeofday(&expire_time, NULL);
    expire_time.tv_sec += poll_interval_;

    /**
     * We pass the workaround_timer to snmp_select_info to get around a bug
     * in snmp_select_info where the timer is always reset to 1usec after 15
     * cumulative seconds of timeouts have been waited on.
     */
    snmp_select_info(&numfds, &fdset, &workaround_timer, &block);
    count = select(numfds, &fdset, NULL, NULL, &timer);

    if (count == 0) {
      /* Regular stats request sending */
      if (send(noxfd,stats_msg, strlen(stats_msg), 0) != 0)
        snmp_log(LOG_ERR, "Failed to send port_stats_request");

      snmp_timeout();
      timer.tv_sec = poll_interval_;
      timer.tv_usec = 0;
    } else if (count > 0) {
      /* Either jsonstats or netsnmp has an event to deal with */
      if(FD_ISSET(noxfd, &fdset)) {
        if ((len = read(noxfd, buf, JFA_BUF_SIZE)) > 0) {
          parse_json(buf, len);
        }

        /* Make sure snmp_read() doesn't tinker with our jsonstats fd */
        FD_CLR(noxfd, &fdset);
        --count;
      }
      if (count > 0) /* If there are SNMP events */
        snmp_read(&fdset);

      struct timeval current;
      gettimeofday(&current, NULL);
      timersub(&expire_time, &current, &timer);
    }
  }

  /* Cleanup connection to jsonstats */
  close(noxfd);
  freeaddrinfo(sf_info);

  snmp_shutdown("json-flow-agent");
  SOCK_CLEANUP;

  return 0;
}
