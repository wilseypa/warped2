CPPFLAGS = -I$(top_srcdir)/deps -I$(top_srcdir)/src
LDFLAGS=-static
LDADD=$(top_builddir)/libwarped2.la


# With a default ext specified, we can skip defining SOURCES for a progarm
# that has only one file  
AM_DEFAULT_SOURCE_EXT = .cpp

WARPED_TEST_PROGRAMS = \
    test_ObjectState

TESTS = $(WARPED_TEST_PROGRAMS)
check_PROGRAMS = $(WARPED_TEST_PROGRAMS)

