/*
 *      Copyright 2011 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef RFSLAVE_HH_
#define RFSLAVE_HH_

enum logType {
        LOGCONFIG_USE_FILE,
        LOGCONFIG_USE_SYSLOG,
        LOGCONFIG_NOT_CONFIGURED,
};

void print_help(char *, int);

#endif /* RFSLAVE_HH_ */
