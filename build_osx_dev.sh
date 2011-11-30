#!/bin/sh

./Configure darwin64-x86_64-cc no-rc4 --prefix=/usr/local/openssl-dev
make depend
make
make install

