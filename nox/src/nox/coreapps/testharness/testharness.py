# Copyright 2008 (C) Nicira, Inc.
# 
# This file is part of NOX.
# 
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.
import subprocess
import exceptions
import logging
import getopt
import signal
import select
import time
import fcntl
import sys
import os

NOX_TEST_RESULTS_DIR = "__nox_tests"

logging.basicConfig(level=logging.DEBUG, format="%(message)s")
statusLg = logging.getLogger('test_status')
harnessLg = logging.getLogger('test_harness')


def usage():
    print """\
   Usage: testharness [-v][-d][-b <builddir] <root_dir>...
       v: verbose mode
       d: turn on debugging
       b: nox build directory
""",

class TimeoutException(Exception):
    def __init__(self):
        pass
    def __str__(self):
        return "TimeoutException"

def sigalarm_handler(signum, frame):
    raise TimeoutException()
signal.signal(signal.SIGALRM, sigalarm_handler)


# Attempt to read a line from a stream, raising an exception if a timeout
# is exceeded.
def read_timeout(stream, t):
    signal.alarm(t)
    str = stream.readline()
    signal.alarm(0)
    return str

class TestAggregator:

    def __init__(self, builddir):
        self.test_list = []
        self.results_finished   = [] #tests which didn't time out
        self.results_timeout    = [] #tests which did time out
        self.results_badformat  = [] #bad output formatting
        self.results_notperfect = [] #at least one failed assertion

        self.total_scripts = 0

        self.total_ok  = 0
        self.total_run = 0

        self.env = os.environ

        self.statusLgLevel = statusLg.getEffectiveLevel()
        self.harnessLgLevel = harnessLg.getEffectiveLevel()
        harnessLg.debug("Initial log levels %s/%s",
                        repr(self.statusLgLevel), repr(self.harnessLgLevel))

        self.pobj = None   # Process object for in-progress test.

        if not builddir:
            harnessLg.warn('builddir not set, using current working directory')
            builddir = ""
        builddir = os.path.normpath(os.path.join(os.getcwd(), builddir))
        self.env['abs_builddir'] = builddir
        self.env['testdefs_sh'] = os.path.join(builddir, "src", "nox", "coreapps", "testharness", "testdefs.sh")
        self.env['PYTHONPATH'] = "."

    def addFile(self, a):
        if not os.access(a, os.X_OK):
            harnessLg.error("Test not executable: %s", a)
        else:
            harnessLg.debug('Adding test: %s', a)
            self.test_list.append(a)

    def addDirTree(self, a):
        harnessLg.debug("Adding test directory tree: %s", a)
        if not os.path.exists(a) or not os.path.isdir(a):
            harnessLg.error("Invalid test root directory: %s ", a)
            return
        for dirpath, dirnames, filenames in os.walk(a, topdown=True):
            if "TEST_DIRECTORY" in filenames:
                harnessLg.debug("Found test directory: %s", dirpath)
                for file in filenames:
                    if not file.startswith('test_'):
                        continue
                    file = os.path.normpath(os.path.join(dirpath, file))
                    if not os.access(file, os.X_OK):
                        continue
                    harnessLg.debug('Adding test: %s', file)
                    self.test_list.append(file)

    def prepare_testing_parent_dir(self):
        if os.access(NOX_TEST_RESULTS_DIR, os.R_OK):
            os.system("rm -rf "+NOX_TEST_RESULTS_DIR)
        os.system("mkdir "+NOX_TEST_RESULTS_DIR)

    def run(self):
        print '++++++++++++++++++++++++++++++'
        harnessLg.debug('Creating test parent directory '+NOX_TEST_RESULTS_DIR)

        self.prepare_testing_parent_dir()
        self.start_time = time.time()
        for test in self.test_list:
            try:
                self.run_test(test)
            except KeyboardInterrupt, e:
                if self.pobj != None:
                    try:
                        os.killpg(self.pobj.pid, signal.SIGKILL)
                    except OSError, e:
                        pass  # May not be anything to kill...
                raise
            self.pobj = None
            self.results_finished.append(test)
            # Restore log levels in case changed by test
            harnessLg.setLevel(self.harnessLgLevel)
            statusLg.setLevel(self.statusLgLevel)
            harnessLg.debug("Restored log levels %s/%s for next test",
                            repr(self.statusLgLevel), repr(self.harnessLgLevel))
        self.finish_time = time.time()

    def prepare_testing_dir(self, test):
        dirname = os.path.join(NOX_TEST_RESULTS_DIR, test)
        os.system("mkdir -p " + dirname)
        return dirname

    def get_test_output(self, stream, max_time):
        old_time = int(time.time())
        while True:
            read_time = max_time - (int(time.time()) - old_time)
            line = read_timeout(stream, read_time)
            if not line:
                harnessLg.debug("Test closed stdout")
                return (None, None)
            line = line[:-1]  # Get rid of trailing newline

            harnessLg.debug("Test output: %s", repr(line))

            try:
                cmd, arg = line.split(": ", 1)
            except ValueError, e:
                harnessLg.debug("Test output is invalid, ignoring.")
                statusLg.info("%s", line)
                continue

            cmd = cmd.lower()
            return (cmd, arg)

    def _handle_timeout(self, test):
        self.results_timeout.append(test)
        statusLg.error('TIMEOUT: %s, killing (pid=%s)...', test, self.pobj.pid)
        try:
            os.killpg(self.pobj.pid, signal.SIGKILL)
        except OSError, e:
            statusLg.error('ERROR: could not kill pid %s: %s',
                           self.pobj.pid, str(e))

    def run_test(self, test):
        harnessLg.info('| %s', test)
        dirname = self.prepare_testing_dir(test)

        self.env['abs_testsrcdir'] = os.path.dirname(os.path.normpath(os.path.join(os.getcwd(), test)))
        self.env['abs_testdir'] = os.path.normpath(os.path.join(os.getcwd(), dirname))

        self.total_scripts += 1

        if not os.access(test, os.X_OK):
            harnessLg.warn('%s not executable skipping', test)
            self.results_badformat.append(test)
            return

        wd = os.path.join(self.env["abs_builddir"], "src")
        abs_test = os.path.normpath(os.path.join(os.getcwd(), test))
        cmd = ["./nox/coreapps/testharness/testrunner.py",abs_test]
        self.pobj = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=self.env, cwd=wd)

        # Read the expected runtime
        try:
            cmd, arg = self.get_test_output(self.pobj.stdout, 6)
        except TimeoutException, e:
            self._handle_timeout(test)
            return
        harnessLg.debug("cmd=%s, arg=%s", repr(cmd), repr(arg))
        if cmd == None:
            harnessLg.error('SKIPPED: test closed stdout before init complete')
            self.results_badformat.append(test)
            return
        if cmd != "+runtime":
            harnessLg.error('SKIPPED: no runtime during init')
            self.results_badformat.append(test)
            return
        runtime = int(arg)

        old_time = int(time.time())
        failed = False
        while True:
            max_time = runtime - (int(time.time()) - old_time)
            try:
                cmd, arg = self.get_test_output(self.pobj.stdout, max_time)
            except TimeoutException,e:
                self._handle_timeout(test)
                return
            if cmd == None:
                break
            elif cmd == "+assert_pass":
                statusLg.info("PASSED assertion: %s", arg)
                self.total_ok  += 1
                self.total_run += 1
            elif cmd == "+assert_fail":
                statusLg.error("FAILED assertion: %s", arg)
                self.total_run += 1
                if not failed:
                    self.results_notperfect.append(test)
                    failed = True
            elif cmd == "+print_msg":
                statusLg.critical("MSG: %s", arg)
            elif cmd == "+output":
                statusLg.critical("%s", arg)
            elif cmd == "+verbose":
                if arg.strip() == "True":
                    statusLg.setLevel(logging.DEBUG)
                    harnessLg.debug("Set status log level to %s",
                                    repr(logging.DEBUG))
                else:
                    statusLg.setLevel(logging.WARN)
                    harnessLg.debug("Set status log level to %s",
                                    repr(logging.WARN))
            elif cmd == "+debug":
                if arg.strip() == "True":
                    harnessLg.setLevel(logging.DEBUG)
                    harnessLg.debug("Set harness log level to %s",
                                    repr(logging.DEBUG))
                else:
                    harnessLg.setLevel(logging.INFO)
                    harnessLg.debug("Set harness log level to %s",
                                    repr(logging.INFO))
            else:
                # Unknown command, must be other test output
                statusLg.info("%s", cmd + ": " + arg)
        max_time = runtime - (int(time.time() - old_time))
        signal.alarm(max_time)
        try:
            retcode = self.pobj.wait()
        except TimeoutException, e:
            self._handle_timeout(test)
            return
        signal.alarm(0)

        self.total_run += 1
        if retcode == 0:
            statusLg.info("PASSED assertion: test case exit code == 0")
            self.total_ok += 1
        else:
            statusLg.error("FAILED assertion: test case exit code == 0")
            statusLg.info("Exit code is: " + str(retcode))
            if test not in self.results_notperfect:
                self.results_notperfect.append(test)
        # Make sure we get rid of any orphaned processes
        try:
            os.killpg(self.pobj.pid, signal.SIGKILL)
        except OSError, e:
            pass # There may be no such children.

    def report_brief(self):
        print '++++++++++++++++++++++++++++++'
        print '  Total testing time: %ds ' % (int(self.finish_time) - int(self.start_time))
        print '  reported results: ',  len(self.results_finished)
        print '  success rate: %d/%d' % (self.total_ok, self.total_run)
        print '  timeouts: ', len(self.results_timeout)
        print '  bad formatting: ', len(self.results_badformat)
        print '++++++++++++++++++++++++++++++'


    def report_failures(self):
        if len(self.results_timeout) > 0:
            print ' Tests which timed out'
            for test in self.results_timeout:
                print '\t',test
        if len(self.results_badformat) > 0:
            print ' Badly formatted tests'
            for test in self.results_badformat:
                print '\t',test
        if len(self.results_notperfect) > 0:
            print ' Tests with at least one failed assertion'
            for test in self.results_notperfect:
                print '\t',test

    def tests_failed(self):
        return (len(self.results_timeout) > 0
                or len(self.results_badformat) > 0
                or len(self.results_notperfect) > 0
                or self.total_ok != self.total_run)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvdb:", ["help","verbose","debug","builddir="])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    builddir = None
    verbose = False
    debug   = False

    for o, a in opts:
        if o in ("-v","--verbose"):
            verbose = True
        elif o in ("-d","--debug"):
            debug = True
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-b", "--builddir"):
            builddir = a
        else:
            assert False, "unhandled option"

    if verbose:
        statusLg.setLevel(logging.DEBUG)
    else:
        statusLg.setLevel(logging.WARN)
    if debug:
        harnessLg.setLevel(logging.DEBUG)
    else:
        harnessLg.setLevel(logging.INFO)

    ta = TestAggregator(builddir)

    for arg in args:
        ta.addDirTree(arg)

    if len(ta.test_list) == 0:
        exit(1)
    ta.run()
    ta.report_brief()
    ta.report_failures()
    if ta.tests_failed():
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    main()
