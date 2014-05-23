# WARPED Unit Tests

This directory contains all unit tests for the WARPED library.

WARPED unit tests are written with the [Catch](http://catch-test.net/) framework.  

## Usage

The Catch library is included with the WARPED source, so installing it separately is not required. 

The following command builds warped and runs all unit tests if the build was successful:

	autoreconf -i && ./configure && make && make check

