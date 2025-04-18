#!/bin/bash
CURDIR=`pwd`
export PATH=/home/cuckoo/metadb/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/obs/lib:/home/cuckoo/metadb/lib:$LD_LIBRARY_PATH
export CONFIG_FILE=$CURDIR/config/config.json

$CURDIR/build.sh test # run UT
$CURDIR/deploy/meta/cuckoo_meta_start.sh
$CURDIR/deploy/client/cuckoo_client_start.sh
sudo apt-get install -y fio
$CURDIR/.github/workflows/smoke_test.sh $CURDIR/build/testfs
$CURDIR/deploy/client/cuckoo_client_stop.sh
$CURDIR/deploy/meta/cuckoo_meta_stop.sh
$CURDIR/build.sh clean
$CURDIR/build.sh clean dist
