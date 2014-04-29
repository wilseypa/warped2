## Main library

lib_LTLIBRARIES = libwarped2.la

WARPED2_HPP_FILES = \ 
    src/Event.hpp \
    src/ObjectState.hpp \
    src/SimulationObject.hpp

WARPED2_CPP_FILES = \
    src/Event.cpp

libwarped2_la_SOURCES = \
    $(WARPED2_HPP_FILES) \
    $(WARPED2_CPP_FILES)

libwarped2_la_HEADERS = src/warped2.hpp

libwarped2_la_CPPFLAGS=-I$(top_srcdir)/deps

SUBDIRS = test
