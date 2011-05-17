#!/usr/bin/python
# 
# TARP
# 4/1/08
# Attempting to use rrdTool a bit, monitor flows on our network switch
# 
# TARP
# 4/2/08
# Automating harvesting, image generation, multiple graphs
#

import sys
sys.path.append('/usr/local/rrdtool-1.2.27/lib/python2.4/site-packages/')
import rrdtool
import time
import subprocess

SEC = 1
MIN = SEC*60
HOUR = MIN*60
DAY = HOUR*24
WEEK = DAY*7
MONTH = DAY*31
YEAR = DAY*365
data_command = "ssh root@badwater.nicira.com -p 1616 " + \
               "/root/src/openflow/utilities/dpctl dump-tables 0"

class Graph:
    def __init__(self):
        global WEEK

        args = {}
        args['imgformat'] = 'PNG'
        args['width'] = 540
        args['height'] = 100
        args['start'] = -WEEK
        args['end'] = -1
        args['vertical-label'] = 'Flows'
        args['title'] = 'Flow Monitoring'
        args['lower-limit'] = 0

        self.defines = []
        self.draw = []
        self.args = args

    def create(self, path):
        args = [path]
        for var in self.args:
            args.append("--" + str(var))
            args.append(str(self.args[var]))
        for define in self.defines:
            args.append(str(define))
        for item in self.draw:
            args.append(str(item))

        rrdtool.graph(*args)

# Customized for dpctl dump-tables
# e.g., data that looks like:
#
# id: 0 name: mac      n_flows:    335 max_flows:   1024
# id: 1 name: hash2    n_flows:  12376 max_flows: 131072
# id: 2 name: linear   n_flows:     12 max_flows:    100
#
class Data:
    def __init__(self, filepath):
        self.filepath = filepath
        self.args = {'step':300}
        self.sources = []
        self.databases = []

        self.type = 'GAUGE'
        self.heartbeat = 500
        self.min = 0
        self.max = 1000000

        # Ratio of missing/total data before declaring unknown
        self.confidence = .5

        self.name_index = 3
        self.flows_index = 5

    def extrapolate_sources(self):
        sources = []

        table = self.reap()
        lines =  table.splitlines()
        for line in lines:
            if len(line) > 0:
                sources.append(line.split()[self.name_index])

        names = []
        for existing in self.sources:
            names.append(existing.split(':')[1])

        for source in sources:
            if source not in names:
                self.sources.append("DS:%s:%s:%s:%s:%s" % \
                    (source, self.type, self.heartbeat, self.min, self.max))

    def add_rra(self, type, sample_period, total_period):
        type = type.upper()
        assert type in ('AVERAGE', 'MAX', 'MIN', 'LAST')
        sample_size = sample_period / self.args['step']
        num_samples  = total_period / self.args['step']
        self.databases.append("RRA:%s:%s:%s:%s" % \
                  (type, self.confidence, sample_size, num_samples))

    def create(self):
        args = [self.filepath]
        for var in self.args:
            args.append("--" + str(var))
            args.append(str(self.args[var]))
        for source in self.sources:
            args.append(str(source))
        for database in self.databases:
            args.append(str(database))

        rrdtool.create(*args)

    def reap(self, command=None):
        if command == None:
            global data_command
            command = data_command
        table = subprocess.Popen(command, stdout=subprocess.PIPE, \
                                 shell=True).stdout.read()
        return table

    # Use 'N' ('now') as the default data entry time
    def harvest(self, time='N'):
        assert self.sources

        table = self.reap()
        points = str(time)
        info = {}

        for line in table.splitlines():
            if len(line) > 0:
                spline = line.split()
                info[spline[self.name_index]] = int(spline[self.flows_index])

        for existing in self.sources:
            source = existing.split(":")[1]
            if source in info.keys():
                points += ":" + str(info[source])
            else:
                # Unknown data point
                points += ':U'

        rrdtool.update(self.filepath, points)


def main():
    global SEC
    global MIN
    global HOUR
    global DAY

    step = SEC*5
    data = Data('flows.rrd')
    data.args['step'] = step
    data.heartbeat = step*2
    data.extrapolate_sources()
    assert step < max(SEC*5, SEC*30, MIN*5)
    data.add_rra('average',SEC*5,HOUR)    # Last hour in 5 sec chunks
    data.add_rra('average',SEC*30,HOUR*3)    # Last 3 hours in 30 sec chunks
    data.add_rra('average',MIN*5,DAY)   # Last day in 5 min chunks
    data.create()

    while True:
        data.harvest()

        graph = Graph()
        graph.defines.append('DEF:mac=flows.rrd:mac:AVERAGE')
        graph.draw.append('AREA:mac#FF000055:"mac"')
        graph.defines.append('DEF:hash2=flows.rrd:hash2:AVERAGE')
        graph.draw.append('AREA:hash2#00FF0055:"hash"')
        graph.defines.append('DEF:linear=flows.rrd:linear:AVERAGE')
        graph.draw.append('AREA:linear#0000FF55:"linear"')

        graph.args['start'] = -HOUR
        graph.args['title'] = "Flow Monitoring: Last Hour"
        graph.create('hour_avg.png')

        graph.args['start'] = -HOUR*3
        graph.args['title'] = "Flow Monitoring: Last 3 Hours"
        graph.create('3hour_avg.png')

        graph.args['start'] = -DAY
        graph.args['title'] = "Flow Monitoring: Last Day"
        graph.create('day_avg.png')

        time.sleep(step)

        # Could auto-generate this as well, even have html-correct tags
        # with the rrd function that generates html img tags (or are the
        # images generated from the tag?)
"""
<html>
<head>
<meta http-equiv="Refresh" content="5">
<title>Flow Monitor</html>
</head>

<body>
<img src="hour_avg.png">
<img src="3hour_avg.png">
<img src="day_avg.png">
</body>
<html>
"""


if __name__ == "__main__":
    main()
