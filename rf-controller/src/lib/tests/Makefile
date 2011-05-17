.PHONY : clean

CC = g++

CFLAGS = -g -Wall -I../../include -I../../openflow/include -O3 #-fprofile-arcs -ftest-coverage 

SRCS = ../expr.cc ../errno_exception.cc main.cc
OBJS = $(patsubst %.cc,%.o,$(SRCS))

CSRCS = ../../openflow/lib/util.c
COBJS = $(patsubst %.c,%.o,$(CSRCS))

default: $(OBJS) $(COBJS)
	$(CC) $(CFLAGS) -o classify $(OBJS) $(COBJS) #-lgcov

$(OBJS): %.o : %.cc
	$(CC) -c $(CFLAGS) $< -o $@

$(COBJS): %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean : 
	rm -f *~ *.o classify *.gcov *.gcda *.gcno ../*.gcov ../*.gcda ../*.gcno