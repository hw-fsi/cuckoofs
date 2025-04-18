#!/bin/bash

DIR=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
source $DIR/cuckoo_meta_config.sh

if [[ "$cnIp" == "$localIp" ]]; then
    cnPath=${cnPathPrefix}0
    pg_ctl stop -l $DIR/cnlogfile0.log -D $cnPath -m immediate
    rm $DIR/cnlogfile0.log
fi

for ((w = 0; w < ${#workerIpList[@]}; w++)); do
    workerIp=${workerIpList[$w]}
    if [[ "$workerIp" == "$localIp" ]]; then
        for ((i = 0; i < ${workerNumList[w]}; i++)); do
            workerPath=${workerPathPrefix}$i
            pg_ctl stop -l $DIR/workerlogfile$i.log -D $workerPath -m immediate
            rm $DIR/workerlogfile$i.log
        done
    fi
done
