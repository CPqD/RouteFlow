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
from time import time

class TokenBucket():
    """A trivial token bucket implementation"""
    def __init__(self, tokens, rate):
        self._tokens = tokens
        self._size = tokens
        self._rate = rate
        self._ts = time()

    def consume(self, token_count):
        """Try to consume token_count tokens.
        Return true if successful, or false if insufficient tokens
        available.
        """
        if token_count <= self.tokens:
            self._tokens -= token_count
            return True
        return False
    
    def _get_tokens(self):
        if self._tokens < self._size:
            now = time()
            new_tokens = self._rate * (time() - self._ts)
            self._tokens = min(self._size, self._tokens + new_tokens)
            self._ts = now
        return self._tokens
    tokens = property(_get_tokens)
