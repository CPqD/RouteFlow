__all__ = ['init_python']

import sys

def log_warning(int):
    msg = 'call to sys.exit() ignored.'
    try:
        from twisted.python import log
        log.err(msg, system='oxide')
    except:
        print >> sys.stderr, msg
    return 0
    
def init_python():
    """
    Initialize the Python for NOX
    """
    # Override the system exit implementation
    sys.exit = log_warning
