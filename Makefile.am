lib_LTLIBRARIES = libwarped.la

WARPED2_HPP_FILES = \
    src/Event.hpp \
    src/LTSFQueue.hpp \
    src/ObjectState.hpp \
    src/serialization.hpp \
    src/Simulation.hpp \
    src/SimulationObject.hpp \
    src/STLLTSFQueue.hpp 

WARPED2_CPP_FILES = \
    src/Simulation.cpp \
    src/STLLTSFQueue.cpp

libwarped_la_SOURCES = \
    $(WARPED2_HPP_FILES) \
    $(WARPED2_CPP_FILES)

include_HEADERS = src/warped.hpp

libwarped_la_CPPFLAGS=-I$(top_srcdir)/deps
libwarped_la_CXXFLAGS = -Wall -Wextra -pedantic

SUBDIRS = test
