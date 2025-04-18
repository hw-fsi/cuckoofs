#! /bin/bash

DIR=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
source $DIR/cuckoo_client_config.sh

if [ -d $CACHE_PATH ]; then
    rm -rf $CACHE_PATH
fi

isCuckooClientExist=$(ps -ux | grep -m 1 cuckoo_client | grep -v "grep" | wc -l)
if [ "${isCuckooClientExist}" = "0" ]; then
    echo "no running cuckoo_client"
else
    cuckooClientPid=$(ps -ux | grep -m 1 cuckoo_client | awk '{print $2}')
    sudo kill -9 $cuckooClientPid
fi

pushd ${ROOT_PATH}/build
sudo umount -l $MNT_PATH
rm -rf cuckoo_client.log
