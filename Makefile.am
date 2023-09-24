SUBDIRS = deps test

lib_LTLIBRARIES = libwarped.la

# These are the headers that will be installed. Only list headers included
# (directly or indirectly) through warped.hpp. Keep all these source listings
# sorted.
WARPED_INSTALL_HPP_FILES = \
    src/Configuration.hpp \
    src/FileStream.hpp \
    src/Event.hpp \
    src/LogicalProcess.hpp \
    src/LPState.hpp \
    src/Partitioner.hpp \
	src/RandomNumberGenerator.hpp \
    src/serialization.hpp \
    src/Simulation.hpp \
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
    src/LadderQueue.hpp \
    src/CircularList.hpp \
    src/CircularQueue.hpp \
    src/LTSFQueue.hpp \
    src/NullEventStatistics.hpp \
    src/ProfileGuidedPartitioner.hpp \
	src/RandomNumberGenerator.hpp \
    src/RoundRobinPartitioner.hpp \
    src/SequentialEventDispatcher.hpp \
    src/STLLTSFQueue.hpp \
    src/SplayTree.hpp \
    src/TimeWarpEventDispatcher.hpp \
	src/TicketLock.hpp \
    src/TimeWarpAggressiveOutputManager.hpp \
    src/TimeWarpAsynchronousGVTManager.hpp \
    src/TimeWarpCommunicationManager.hpp \
    src/TimeWarpEventSet.hpp \
    src/TimeWarpFileStream.hpp \
    src/TimeWarpFileStreamManager.hpp \
	src/TimeWarpGVTManager.hpp \
    src/TimeWarpKernelMessage.hpp \
    src/TimeWarpMPICommunicationManager.hpp \
    src/TimeWarpOutputManager.hpp \
    src/TimeWarpPeriodicStateManager.hpp \
    src/TimeWarpStateManager.hpp \
	src/TimeWarpStatistics.hpp \
	src/TimeWarpSynchronousGVTManager.hpp \
	src/TimeWarpTerminationManager.hpp \
    src/utility/strings.hpp \
    src/Latency.hpp \
    src/Utils.hpp

# All cpp files are listed in this variable
WARPED_CPP_FILES = \
    src/AggregateEventStatistics.cpp \
    src/CommandLineConfiguration.cpp \
    src/Configuration.cpp \
    src/TimeWarpEventSet.cpp \
    src/EventStatistics.cpp \
    src/FileStream.cpp \
    src/IndividualEventStatistics.cpp \
    src/LogicalProcess.cpp \
    src/LadderQueue.cpp \
    src/ProfileGuidedPartitioner.cpp \
    src/RoundRobinPartitioner.cpp \
    src/SequentialEventDispatcher.cpp \
    src/Simulation.cpp \
    src/SplayTree.cpp \
    src/STLLTSFQueue.cpp \
    src/TimeWarpAggressiveOutputManager.cpp \
    src/TimeWarpAsynchronousGVTManager.cpp \
    src/TimeWarpCommunicationManager.cpp \
    src/TimeWarpEventDispatcher.cpp \
    src/TimeWarpFileStream.cpp \
    src/TimeWarpFileStreamManager.cpp \
	src/TimeWarpGVTManager.cpp \
    src/TimeWarpMPICommunicationManager.cpp \
    src/TimeWarpOutputManager.cpp \
    src/TimeWarpPeriodicStateManager.cpp \
    src/TimeWarpStateManager.cpp \
	src/TimeWarpStatistics.cpp \
	src/TimeWarpSynchronousGVTManager.cpp \
	src/TimeWarpTerminationManager.cpp \
    src/Latency.cpp \
    src/Utils.cpp

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
