include ../Make.vars

EXTRA_DIST=\
	test-classifier.sh			\
	test-coop-preblock-hook.sh		\
	test-coop-sema.sh			\
	test-coop-signals.sh			\
	test-event-dispatcher-blocking.sh	\
	test-event-dispatcher-starvation.sh	\
	test-poll-loop-removal.sh		\
	test-timer-dispatcher-delay.sh		\
	test-timer-dispatcher-duplicates.sh	\
	test-timer-dispatcher-starvation.sh	\
	test-timeval.sh				\
	test-type-props.sh


if PY_ENABLED
AM_CPPFLAGS += -I $(INCLUDEPY)
endif # PY_ENABLED

TESTS = \
	test-classifier.sh			\
	test-coop-preblock-hook.sh		\
	test-coop-sema.sh			\
	test-coop-signals.sh			\
	test-ethernetaddr			\
	test-event-dispatcher-blocking.sh	\
	test-event-dispatcher-starvation.sh	\
	test-poll-loop-removal.sh		\
	test-timer-dispatcher-delay.sh		\
	test-timer-dispatcher-duplicates.sh	\
	test-timer-dispatcher-starvation.sh	\
	test-timeval.sh				\
	test-type-props.sh

check_PROGRAMS = \
	test-classifier				\
	test-coop-preblock-hook			\
	test-coop-sema				\
	test-coop-signals			\
	test-ethernetaddr			\
	test-event-dispatcher-blocking		\
	test-event-dispatcher-starvation	\
	test-poll-loop-removal			\
	test-timer-dispatcher-delay		\
	test-timer-dispatcher-duplicates	\
	test-timer-dispatcher-starvation	\
	test-timeval				\
	test-type-props

LDADD += ../lib/libnoxcore.la ../builtin/.libs/libbuiltin.la  \
    $(BOOST_LDFLAGS)  \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) 			\
	$(BOOST_FILESYSTEM_LIB) 				\
	$(SSL_LIBS) \
    ../components.xsd.o \
    ../nox.xsd.o

test_classifier_SOURCES = test-classifier.cc test-classifier.hh

test_coop_preblock_hook_SOURCES = test-coop-preblock-hook.cc

test_coop_sema_SOURCES = test-coop-sema.cc

test_coop_signals_SOURCES = test-coop-signals.cc

test_ethernetaddr_SOURCES = test-ethernetaddr.cc

test_event_dispatcher_blocking_SOURCES = test-event-dispatcher-blocking.cc

test_event_dispatcher_starvation_SOURCES = test-event-dispatcher-starvation.cc

test_poll_loop_removal_SOURCES = test-poll-loop-removal.cc

test_timer_dispatcher_delay_SOURCES = test-timer-dispatcher-delay.cc

test_timer_dispatcher_duplicates_SOURCES = test-timer-dispatcher-duplicates.cc

test_timer_dispatcher_starvation_SOURCES = test-timer-dispatcher-starvation.cc

test_timeval_SOURCES = test-timeval.cc ../lib/timeval.cc
test_type_props_SOURCES = test-type-props.c
