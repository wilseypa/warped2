#!/bin/bash

make clean
autoreconf -i
./configure --with-mpi-includedir=/usr/include/x86_64-linux-gnu/openmpi --with-mpi-libdir=/usr/lib/x86_64-linux-gnu/openmpi --prefix=/home/shindegy/warped2/build
make
make install
