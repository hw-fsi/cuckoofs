#! /bin/bash

DIR=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
source $DIR/cuckoo_client_config.sh

pushd ${ROOT_PATH}/build

if [ ! -d $MNT_PATH ]; then
    mkdir $MNT_PATH
fi

if [ -d $CACHE_PATH ]; then
    rm -rf $CACHE_PATH
fi

mkdir $CACHE_PATH
for i in {0..100}; do
    mkdir $CACHE_PATH/$i
done

nohup ./bin/cuckoo_client $MNT_PATH -f -o direct_io -o attr_timeout=200 -o entry_timeout=200 -rpc_endpoint=0.0.0.0:56039 -socket_max_unwritten_bytes=268435456 >cuckoo_client.log 2>&1 &
