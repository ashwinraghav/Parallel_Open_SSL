#!/bin/bash

./config --prefix=/af21/zl4ef/openssl-dev
OPENCL_INCLUDE=/usr/local/cuda/include make
make install

