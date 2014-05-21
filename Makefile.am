SUBDIRS = deps test

lib_LTLIBRARIES = libwarped.la

WARPED2_HPP_FILES = \
    src/Configuration.hpp \
    src/Event.hpp \
    src/EventDispatcher.hpp \
    src/LTSFQueue.hpp \
    src/ObjectState.hpp \
    src/SequentialEventDispatcher.hpp \
    src/serialization.hpp \
    src/Simulation.hpp \
    src/SimulationObject.hpp \
    src/STLLTSFQueue.hpp

WARPED2_CPP_FILES = \
    src/Configuration.cpp \
    src/SequentialEventDispatcher.cpp \
    src/Simulation.cpp \
    src/STLLTSFQueue.cpp

libwarped_la_SOURCES = \
    $(WARPED2_HPP_FILES) \
    $(WARPED2_CPP_FILES)

include_HEADERS = src/warped.hpp

libwarped_la_LIBADD = $(top_builddir)/deps/json/libjsoncpp.la
libwarped_la_CXXFLAGS = -Wall -Wextra -pedantic

# Use -isystem here instead of -I to disable warnings for third party code
libwarped_la_CPPFLAGS=-isystem$(top_srcdir)/deps