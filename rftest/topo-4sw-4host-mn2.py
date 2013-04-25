"""Custom topology example

author: Marcelo Nascimento (marcelon@cpqd.com.br)

Four switches connected in mesh topology plus a host for each switch:

       h1 --- sA ---- sB --- h2
               |  \    |
               |   \   |
	       |    \  | 
               |     \ |
       h3 --- sC ---- sD --- h4

Adding the 'topos' dict with a key/value pair to generate our newly defined
topology enables one to pass in '--topo=mytopo' from the command line.
"""

from mininet.topo import Topo

class RFTopo( Topo ):
    "RouteFlow Demo Setup"

    def __init__( self, enable_all = True ):
        "Create custom topo."

        # Add default members to class.
        Topo.__init__(self)

        # Set Node IDs for hosts and switches
        h1 = self.addHost( 'h1' )
        h2 = self.addHost( 'h2' )
        h3 = self.addHost( 'h3' )
        h4 = self.addHost( 'h4' )
        sA = self.addSwitch( 's1' )
        sB = self.addSwitch( 's2' )
        sC = self.addSwitch( 's3' )
        sD = self.addSwitch( 's4' )

        # Add edges
        self.addLink( h1, sA )
        self.addLink( h2, sB )
        self.addLink( h3, sC )
        self.addLink( h4, sD )
        self.addLink( sA, sB )
        self.addLink( sB, sD )
        self.addLink( sD, sC )
        self.addLink( sC, sA )
        self.addLink( sA, sD )

topos = { 'rftopo': ( lambda: RFTopo() ) }
