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
#
# Reactor for plugging twisted into the vigil main loop.  Uses
# oxidereactorglue under the covers to register fildescriptors.
#

import signal
import nox.lib.core
import oxidereactor
import twisted
import logging, types
from   twisted.internet import base, error, interfaces, posixbase, task
from   twisted.internet.interfaces import IResolverSimple
from   twisted.internet.process import reapAllProcesses

from   twisted.python.runtime import seconds
from   twisted.python import log, reflect
from   zope.interface import implements

def doRead(reader, reactor):
    why = None
    why = reader.doRead()
    if why:
        twisted.internet.reactor._disconnectSelectable(reader, why, 1)
    reactor.callPendingThreadCalls()
        
def doWrite(writer, reactor):
    why = None
    why = writer.doWrite()
    if why:
        twisted.internet.reactor._disconnectSelectable(writer, why, 0)
    reactor.callPendingThreadCalls()

class DelayedCallWrapper(base.DelayedCall):

    def __init__(self, delay, func, args, kw):
        base.DelayedCall.__init__(self, delay, func, args, kw, None, None)
        self.dc = oxidereactor.delayedcall()

    def delay(self, secondsLater):
        if self.cancelled:
            raise error.AlreadyCancelled
        elif self.called:
            raise error.AlreadyCalled
        if secondsLater < 0:
            self.dc.delay(True, long(-secondsLater),
                          long(-secondsLater * 1000 % 1000))
        else:
            self.dc.delay(False, long(secondsLater),
                          long(secondsLater * 1000 % 1000))

    def reset(self, secondsFromNow):
        if self.cancelled:
            raise error.AlreadyCancelled
        elif self.called:
            raise error.AlreadyCalled
        self.dc.reset(long(secondsFromNow),
                      long(secondsFromNow * 1000 % 1000))

    def cancel(self):
        if self.cancelled:
            raise error.AlreadyCancelled
        elif self.called:
            raise error.AlreadyCalled
        self.cancelled = 1
        self.dc.cancel()

    def __call__(self):
        try:
            self.called = 1
            self.func(*self.args, **self.kw);
        except:
            log.deferr()
            if hasattr(self, "creator"):
                e = "\n"
                e += " C: previous exception occurred in " + \
                    "a DelayedCall created here:\n"
                e += " C:"
                e += "".join(self.creator).rstrip().replace("\n","\n C:")
                e += "\n"
                log.msg(e)

class Resolver:
    implements (IResolverSimple)

    def __init__(self, oreactor):
        self.oreactor = oreactor

    def getHostByName(self, name, timeout = (1, 3, 11, 45)):
        from twisted.internet.defer import Deferred

        d = Deferred()

        self.oreactor.resolve(name, d.callback)

        def query_complete(address, name):
            if address is None or address == "":
                from twisted.internet import error

                msg = "address %r not found" % (name,)
                err = error.DNSLookupError(msg)

                from twisted.internet import defer
                return defer.fail(err)
            else:
                return address

        return d.addCallback(query_complete, name)

class pyoxidereactor (posixbase.PosixReactorBase):

    def __init__(self, ctxt):
        from twisted.internet.main import installReactor
        self.oreactor = oxidereactor.oxidereactor(ctxt, "oxidereactor")
        posixbase.PosixReactorBase.__init__(self)
        installReactor(self)
        self.installResolver(Resolver(self.oreactor))
        signal.signal(signal.SIGCHLD, self._handleSigchld)

        # Twisted uses os.waitpid(pid, WNOHANG) but doesn't try again
        # if the call returns nothing (since not being able to block).
        # Poll once a second on behalf of Twisted core to detect child
        # processes dying properly.
        task.LoopingCall(reapAllProcesses).start(1)

    # The removeReader, removeWriter, addReader, and addWriter
    # functions must be implemented, because they are used extensively
    # by Python code.
    def removeReader(self, reader):
        self.oreactor.removeReader(reader.fileno())

    def removeWriter(self, writer):
        self.oreactor.removeWriter(writer.fileno())

    def addReader(self, reader):
        self.oreactor.addReader(reader.fileno(), reader, 
                                lambda reader: doRead(reader, self))

    def addWriter(self, writer):
        self.oreactor.addWriter(writer.fileno(), writer, 
                                lambda writer: doWrite(writer, self))

    # doIteration is called from PosixReactorBase.mainLoop
    #   which is called from PosixReactorBase.run
    #     and we never call either one.
    # doIteration is also called from ReactorBase.iterate,
    #   which we never call.
    # So it seems questionable whether we have to implement this.
    # For now, stub it out.
    def doIteration(self, delay=0):
        raise NotImplementedError()

    # stop is called from ReactorBase.sigInt,
    #   which is called upon receipt of SIGINT
    #     only if PosixReactorBase.startRunning is allowed to install
    #     signal handlers. It seems uncertain, at best, that we'll
    #     want Python to handle our signals.
    def stop(self):
        raise NotImplementedError()

    # removeAll is called from ReactorBase.disconnectAll
    #   which is called indirectly from stop
    #     which we concluded above doesn't get called in our system.
    # So there's no need to implement it.
    def removeAll(self):
        raise NotImplementedError()

    # IReactorTime interface for timer management
    def callLater(self, delay, f, *args, **kw):
        if not callable(f):
            raise TypeError("'f' object is not callable")
        tple = DelayedCallWrapper(delay, f, args, kw)
        self.oreactor.callLater(long(tple.getTime()),
                             long(tple.getTime() * 1000000 % 1000000),
                             tple);
        return tple

    def getDelayedCalls(self):
        raise NotImplementedError()

    # Calls to be invoked in the reactor thread
    def callPendingThreadCalls(self):
        if self.threadCallQueue:
            count = 0
            total = len(self.threadCallQueue)
	    for (f, a, kw) in self.threadCallQueue:
                try:
                    f(*a, **kw)
                except:
                    log.err()
	        count += 1
                if count == total:
                    break
            del self.threadCallQueue[:count]
	    if self.threadCallQueue:
                if self.waker:
                    self.waker.wakeUp()

class vlog(log.FileLogObserver):
    """
    Passes Twisted log messages to the C++ vlog.
    """
    def __init__(self, v):
        log.FileLogObserver.__init__(self, log.NullFile())
        self.modules = {}
        self.v = v

    def __call__(self, event):
        msg = event['message']
        if not msg:
            if event['isError'] and event.has_key('failure'):
                msg = ((event.get('why') or 'Unhandled Error')
                       + '\n' + event['failure'].getTraceback())
            elif event.has_key('format'):
                if hasattr(self, '_safeFormat'):
                    msg = self._safeFormat(event['format'], event)
                else:
                    msg = log._safeFormat(event['format'], event)
            else:
                return
        else:
            msg = ' '.join(map(reflect.safe_str, msg))
        try:
            module = event['system']
            # Initialize the vlog modules on-demand, since systems are
            # not initialized explicitly in the Twisted logging API.
            if not self.modules.has_key(module):
                if module == '-':
                    module = 'reactor'
                    self.modules['-'] = self.v.mod_init(module)
                    self.modules[module] = self.modules['-']
                else:
                    self.modules[module] = self.v.mod_init(module)

            # vlog module identifier
            module = self.modules[module]

            fmt = {'system': module, 'msg': msg.replace("\n", "\n\t")}
            if hasattr(self, '_safeFormat'):
                msg = self._safeFormat("%(msg)s", fmt)
            else:
                msg = log._safeFormat("%(msg)s", fmt)
            if event["isError"]:
                self.v.err(module, msg)
            else:
                self.v.msg(module, msg)
        except Exception, e:
            # Can't pass an exception, since then the observer would
            # be disabled automatically.
            pass

v = oxidereactor.vigillog()
log.startLoggingWithObserver(vlog(v), 0)

class VigilLogger(logging.Logger):
    """
    Stores the C++ logger identifier.
    """
    def __init__(self, name):
        logging.Logger.__init__(self, name)
        self.vigil_logger_id = v.mod_init(name)

    def isEnabledFor(self, level):
        if level < logging.DEBUG:
            return v.is_dbg_enabled(self.vigil_logger_id)
        elif level < logging.INFO:
            return v.is_dbg_enabled(self.vigil_logger_id)
        elif level < logging.WARN:
            return v.is_info_enabled(self.vigil_logger_id)
        elif level < logging.ERROR:
            return v.is_warn_enabled(self.vigil_logger_id)
        elif level < logging.FATAL:
            return v.is_err_enabled(self.vigil_logger_id)
        else:
            return v.is_emer_enabled(self.vigil_logger_id)

    def makeRecord(self, name, level, fn, lno, msg, args, exc_info, 
                   func=None, extra=None):
        """
        Inject the vigil logger id into a standard Python log record.
        """
        rv = logging.Logger.makeRecord(self, name, level, fn, lno, msg, args,
                                       exc_info, func, extra)
        rv.__dict__['vigil_logger_id'] = self.vigil_logger_id
        return rv

class VigilHandler(logging.Handler):
    """
    A Python logging handler class which writes logging records, with
    minimal formatting, to the C++ logging infrastructure.
    """
    def __init__(self):
        """
        Initialize the handler.
        """
        logging.Handler.__init__(self)
        
    def emit(self, record):
        """
        Emit a record.
        """
        try:
            vigil_logger_id = record.__dict__['vigil_logger_id']
            msg = self.format(record)

            if record.levelno < logging.DEBUG:
                o = v.dbg
            elif record.levelno < logging.INFO:
                o = v.dbg
            elif record.levelno < logging.WARN:
                o = v.info
            elif record.levelno < logging.ERROR:
                o = v.warn
            elif record.levelno < logging.FATAL:
                o = v.err
            else:
                o = v.fatal
            
            fs = "%s"
            if not hasattr(types, "UnicodeType"): #if no unicode support...
                o(vigil_logger_id, fs % msg)
            else:
                if isinstance(msg, str):
                    #caller may have passed us an encoded byte string...
                    msg = unicode(msg, 'utf-8')
                msg = msg.encode('utf-8')
                o(vigil_logger_id, fs % msg)
            self.flush()
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            self.handleError(record)

def config():
    vigil_handler = VigilHandler()
    fs = format='%(message)s'
    dfs = None
    fmt = logging.Formatter(fs, dfs)
    vigil_handler.setFormatter(fmt)
    logging.root.addHandler(vigil_handler)
    logging.root.setLevel(logging.DEBUG)

config()
logging.setLoggerClass(VigilLogger)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return pyoxidereactor(ctxt)

    return Factory()
