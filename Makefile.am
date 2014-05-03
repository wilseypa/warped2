lib_LTLIBRARIES = libwarped2.la

WARPED2_HPP_FILES = \
    src/Event.hpp \
    src/ObjectState.hpp \
    src/serialization.hpp \
    src/Simulation.hpp \
    src/SimulationObject.hpp 

WARPED2_CPP_FILES = \
    src/Simulation.cpp

libwarped2_la_SOURCES = \
    $(WARPED2_HPP_FILES) \
    $(WARPED2_CPP_FILES)

include_HEADERS = src/warped2.hpp

libwarped2_la_CPPFLAGS=-I$(top_srcdir)/deps
libwarped2_la_CXXFLAGS = -Wall -Wextra -pedantic

SUBDIRS = test
