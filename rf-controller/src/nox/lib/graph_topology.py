import pydot
import time
import re
import os.path

from pyapps import locator
from pyapps import discovery
from vigil.packet import *


def dot_output(graph, name):

    sfx  = name.split('.')[len(name.split('.')) -1]
    base = os.path.basename(name)

    if sfx == "svg":    
        svg = graph.create_svg(prog='dot')
        # Strip the XML prologue and just return the guts.
        return svg[svg.index('<svg'):]
    else:
        graph.write_jpg(name, prog='dot')

def create_node_graph(output = "jpg"):
    nodes = {}
    switch_edge_ports = {} 
    edge_list = [] 

    for link in discovery.adjacency_list:    
        node1_name = longlong_to_octstr(link[0])[6:].replace(':','')
        node2_name = longlong_to_octstr(link[2])[6:].replace(':','')
        nodes[node1_name] = True
        nodes[node2_name] = True
        edge_list.append((node1_name, node2_name))
        switch_edge_ports[(node1_name, node2_name)] = (link[1], link[3])

    g = pydot.graph_from_edges(edge_list, directed=True)

    # for all edge inferred by discovery, set port labels    
    for linkt in switch_edge_ports:
        edgel = g.get_edge(linkt[0], linkt[1])
        if type(edgel) != type([]):
            edgel.set('headlabel',str(switch_edge_ports[linkt][1]))
            edgel.set('taillabel',str(switch_edge_ports[linkt][0]))
        else:    
            for edge in edgel:
                edge.set('headlabel',str(switch_edge_ports[linkt][1]))
                edge.set('taillabel',str(switch_edge_ports[linkt][0]))

    for node in g.get_node_list():
        node.set('style', 'filled,setlinewidth(2)')
        node.set('fillcolor', '#ffffcc')
        node.set('color', '#99cc99')
        node.set('tooltip', 'switch')

    return dot_output(g, "pyapps/www/discovery."+output)


def create_topology_graph(output = "jpg",
                          locations = None,
                          adjacency_list = discovery.adjacency_list):
    import pyapps.locator                        
    if not locations:
        locations = pyapps.locator.locations

    hosts = {} 
    nodes = {}
    locator_edges = {} 
    switch_edge_ports = {} 
    edge_list = [] 


    for link in locations:
        node1_name = longlong_to_octstr(link[0])[6:].replace(':','.')
        node2_name = longlong_to_octstr(link[2])[6:].replace(':','.')
        nodes[node1_name] = True
        hosts[node2_name] = True
        edge_list.append((node1_name, node2_name))
        locator_edges[(node1_name, node2_name)] = (link[1], locations[link])

    for link in adjacency_list:    
        node1_name = longlong_to_octstr(link[0])[6:].replace(':','.')
        node2_name = longlong_to_octstr(link[2])[6:].replace(':','.')
        nodes[node1_name] = True
        nodes[node2_name] = True
        edge_list.append((node1_name, node2_name))
        switch_edge_ports[(node1_name, node2_name)] = (link[1], link[3])

    g = pydot.graph_from_edges(edge_list, directed=True)

    # for all edges inferred by locator, make them bidirection and
    # set their color
    for linkt in locator_edges:
        edge = g.get_edge(linkt[0], linkt[1])
        if type(edge) == type([]):
            edge = edge[0]

        edge.set('color','blue')
        edge.set('dir','both')
        edge.set('taillabel',str(locator_edges[linkt][0]))
        #edge.set('label',"%2.3f" % (time.time() - locator_edges[linkt][1]))

    # for all edge inferred by discovery, set port labels    
    for linkt in switch_edge_ports:
        edgel = g.get_edge(linkt[0], linkt[1])
        if type(edgel) != type([]):
            edgel.set('headlabel',str(switch_edge_ports[linkt][1]))
            edgel.set('taillabel',str(switch_edge_ports[linkt][0]))
        else:    
            for edge in edgel:
                edge.set('headlabel',str(switch_edge_ports[linkt][1]))
                edge.set('taillabel',str(switch_edge_ports[linkt][0]))

    for node in g.get_node_list():
        if not node.get_name() in nodes:
            node.set('shape', 'box')
            node.set('style', 'rounded,setlinewidth(2)')
            node.set('color', 'blue')
            node.set('tooltip', 'host')
            node.set('URL', '/display_host.mhtml?name='+node.get_name().replace('.',':'))
        else:    
            node.set('style', 'filled,setlinewidth(2)')
            node.set('fillcolor', '#ffffcc')
            node.set('color', '#99cc99')
            node.set('tooltip', 'switch')

    return dot_output(g, "pyapps/www/topology."+output)
