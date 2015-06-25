SUBDIRS = deps test

lib_LTLIBRARIES = libwarped.la

# These are the headers that will be installed. Only list headers included
# (directly or indirectly) through warped.hpp. Keep all these source listings
# sorted.
WARPED_INSTALL_HPP_FILES = \
    src/Configuration.hpp \
    src/FileStream.hpp \
    src/Event.hpp \
    src/ObjectState.hpp \
    src/Partitioner.hpp \
	src/RandomNumberGenerator.hpp \
    src/serialization.hpp \
    src/Simulation.hpp \
    src/SimulationObject.hpp \
    src/warped.hpp \
    src/utility/memory.hpp \
    src/utility/warnings.hpp

# All other headers are listed in this variable
WARPED_NOINST_HPP_FILES = \
    src/AggregateEventStatistics.hpp \
    src/CommandLineConfiguration.hpp \
    src/EventDispatcher.hpp \
    src/EventStatistics.hpp \
    src/IndividualEventStatistics.hpp \
    src/LTSFQueue.hpp \
    src/NullEventStatistics.hpp \
    src/ProfileGuidedPartitioner.hpp \
    src/RoundRobinPartitioner.hpp \
    src/SequentialEventDispatcher.hpp \
    src/STLLTSFQueue.hpp \
    src/LadderQueue.hpp \
	src/RandomNumberGenerator.hpp \
    src/TimeWarpEventDispatcher.hpp \
    src/utility/strings.hpp \
	src/TicketLock.hpp \
    src/TimeWarpMatternGVTManager.hpp \
    src/TimeWarpKernelMessage.hpp \
    src/TimeWarpCommunicationManager.hpp \
    src/TimeWarpMPICommunicationManager.hpp \
    src/TimeWarpStateManager.hpp \
    src/TimeWarpPeriodicStateManager.hpp \
    src/TimeWarpOutputManager.hpp \
    src/TimeWarpAggressiveOutputManager.hpp \
    src/TimeWarpFileStreamManager.hpp \
    src/TimeWarpFileStream.hpp \
    src/TimeWarpEventSet.hpp \
	src/TimeWarpLocalGVTManager.hpp \
	src/TimeWarpTerminationManager.hpp \
	src/TimeWarpStatistics.hpp

# All cpp files are listed in this variable
WARPED_CPP_FILES = \
    src/AggregateEventStatistics.cpp \
    src/CommandLineConfiguration.cpp \
    src/Configuration.cpp \
    src/EventStatistics.cpp \
    src/IndividualEventStatistics.cpp \
    src/ProfileGuidedPartitioner.cpp \
    src/RoundRobinPartitioner.cpp \
    src/SequentialEventDispatcher.cpp \
    src/Simulation.cpp \
    src/SimulationObject.cpp \
    src/STLLTSFQueue.cpp \
    src/LadderQueue.cpp \
    src/TimeWarpEventDispatcher.cpp \
    src/TimeWarpMatternGVTManager.cpp \
    src/TimeWarpCommunicationManager.cpp \
    src/TimeWarpMPICommunicationManager.cpp \
    src/TimeWarpStateManager.cpp \
    src/TimeWarpPeriodicStateManager.cpp \
    src/TimeWarpOutputManager.cpp \
    src/TimeWarpAggressiveOutputManager.cpp \
    src/TimeWarpFileStreamManager.cpp \
    src/FileStream.cpp \
    src/TimeWarpFileStream.cpp \
    src/TimeWarpEventSet.cpp \
	src/TimeWarpLocalGVTManager.cpp \
	src/TimeWarpTerminationManager.cpp \
	src/TimeWarpStatistics.cpp

libwarped_la_SOURCES = \
    $(WARPED_INSTALL_HPP_FILES) \
    $(WARPED_NOINST_HPP_FILES) \
    $(WARPED_CPP_FILES)

include_HEADERS = $(WARPED_INSTALL_HPP_FILES)

libwarped_la_LIBADD = \
    $(top_builddir)/deps/json/libjsoncpp.la \
    $(top_builddir)/deps/metis/libmetis/libmetis.la

libwarped_la_CXXFLAGS = -Wall -Wextra -pedantic -Werror

# Use -isystem here instead of -I to disable warnings for third party code
libwarped_la_CPPFLAGS=-isystem$(top_srcdir)/deps
