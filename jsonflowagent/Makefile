CC=g++

OBJS=cJSON.o mibs/mib.o mibs/phy_port.o mibs/port_stats.o parser.o main.o
TARGETS=jfa

CFLAGS=-I. -Wall -g
CXXFLAGS=`net-snmp-config --cflags` $(CFLAGS)
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)

jfa: $(OBJS)
	$(CC) -o jfa $(OBJS) $(BUILDAGENTLIBS)

clean:
	rm $(OBJS) $(TARGETS)
