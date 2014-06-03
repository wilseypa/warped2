CPPFLAGS = -I$(top_srcdir)/deps -I$(top_srcdir)/src
LDFLAGS = -static
LDADD = $(top_builddir)/libwarped.la

# With a default ext specified, we can skip defining SOURCES for a progarm
# that has only one file  
AM_DEFAULT_SOURCE_EXT = .cpp

WARPED_TEST_PROGS = \
    test_AggregateEventStatistics \
    test_CommandLineConfiguration \
    test_Event \
    test_IndividualEventStatistics \
    test_ObjectState \
    test_ProfileGuidedPartitioner \
    test_RoundRobinPartitioner \
    test_serialization \
    test_Simulation \
    test_SimulationObject \
    test_STLLTSFQueue

noinst_HEADERS = catch.hpp mocks.hpp

TESTS = $(WARPED_TEST_PROGS)
check_PROGRAMS = $(WARPED_TEST_PROGS)

