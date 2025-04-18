#!/bin/bash

DIR=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
ROOT_PATH=$DIR/../..
CACHE_PATH=/tmp/cuckoo_cache
MNT_PATH=./testfs
