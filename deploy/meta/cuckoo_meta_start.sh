#!/bin/bash
DIR=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
source $DIR/cuckoo_meta_config.sh

uniquePortPrefix=555
cnPortPrefix=${uniquePortPrefix}0
cnPoolerPortPrefix=${uniquePortPrefix}1
workerPortPrefix=${uniquePortPrefix}2
workerPollerPortPrefix=${uniquePortPrefix}3
cnMonitorPortPrefix=${uniquePortPrefix}8
workerMonitorPortPrefix=${uniquePortPrefix}9
cuckooConnectionPoolSize=32
cuckooConnectionPoolShmemSize=$((256)) #unit: MB
username=$USER

server_name_list=()
server_ip_list=()
server_port_list=()

shardcount=50

if [[ "$cnIp" == "$localIp" ]]; then
    cnPath=${cnPathPrefix}0
    cnPort=${cnPortPrefix}0
    cnMonitorPort=${cnMonitorPortPrefix}0
    cnPoolerPort=${cnPoolerPortPrefix}0
    rm -r $cnPath/*
    initdb -D $cnPath
    cp $DIR/postgresql.conf.template $cnPath/postgresql.conf
    echo "shared_preload_libraries = 'cuckoo'" >>$cnPath/postgresql.conf
    echo "port=$cnPort" >>$cnPath/postgresql.conf
    echo "listen_addresses = '*'" >>$cnPath/postgresql.conf
    echo "host all all 0.0.0.0/0 trust" >>$cnPath/pg_hba.conf
    echo "wal_level = logical" >>$cnPath/postgresql.conf
    echo "max_replication_slots = 8" >>$cnPath/postgresql.conf
    echo "max_wal_senders = 8" >>$cnPath/postgresql.conf
    #echo "log_statement = 'all'" >> $cnPath/postgresql.conf

    echo "cuckoo_connection_pool.port = $cnPoolerPort" >>$cnPath/postgresql.conf
    echo "cuckoo_connection_pool.pool_size = $cuckooConnectionPoolSize" >>$cnPath/postgresql.conf
    echo "cuckoo_connection_pool.shmem_size = $cuckooConnectionPoolShmemSize" >>$cnPath/postgresql.conf

    pg_ctl start -l $DIR/cnlogfile0.log -D $cnPath -c #o "-d 1"
    psql -d postgres -h $cnIp -p $cnPort -c "CREATE EXTENSION cuckoo;"
    server_name_list[${#server_name_list[@]}]=cn0
    server_ip_list[${#server_ip_list[@]}]=$cnIp
    server_port_list[${#server_port_list[@]}]=$cnPort
fi

for ((n = 0; n < ${#workerIpList[@]}; n++)); do
    for ((i = 0; i < ${workerNumList[n]}; i++)); do
        workerIp=${workerIpList[$n]}
        workerPort=$workerPortPrefix$i
        workerMonitorPort=$workerMonitorPortPrefix$i

        if [[ "$workerIp" == "$localIp" ]]; then
            workerPath=$workerPathPrefix$i
            workerPoolerPort=$workerPollerPortPrefix$i
            rm -r $workerPath/*
            initdb -D $workerPath
            cp $DIR/postgresql.conf.template $workerPath/postgresql.conf
            echo "shared_preload_libraries = 'cuckoo'" >>$workerPath/postgresql.conf
            echo "port=$workerPort" >>$workerPath/postgresql.conf
            echo "listen_addresses = '*'" >>$workerPath/postgresql.conf
            echo "host all all 0.0.0.0/0 trust" >>$workerPath/pg_hba.conf
            echo "wal_level = logical" >>$workerPath/postgresql.conf
            echo "max_replication_slots = 8" >>$workerPath/postgresql.conf
            echo "max_wal_senders = 8" >>$workerPath/postgresql.conf
            #echo "log_statement = 'all'" >> $workerPath/postgresql.conf

            echo "cuckoo_connection_pool.port = $workerPoolerPort" >>$workerPath/postgresql.conf
            echo "cuckoo_connection_pool.pool_size = $cuckooConnectionPoolSize" >>$workerPath/postgresql.conf
            echo "cuckoo_connection_pool.shmem_size = $cuckooConnectionPoolShmemSize" >>$workerPath/postgresql.conf

            pg_ctl start -l $DIR/workerlogfile$i.log -D $workerPath -c #o "-d 1"
            psql -d postgres -h $workerIp -p $workerPort -c "CREATE EXTENSION cuckoo;"
        fi

        server_name_list[${#server_name_list[@]}]=worker_${n}_${i}
        server_ip_list[${#server_ip_list[@]}]=$workerIp
        server_port_list[${#server_port_list[@]}]=$workerPort
    done
done

# Load Cuckoo

if [[ "$cnIp" == "$localIp" ]]; then
    server_num=${#server_port_list[@]}
    for ((i = 0; i < $server_num; i++)); do
        name=${server_name_list[i]}
        ip=${server_ip_list[i]}
        port=${server_port_list[i]}
        id=$i
        for ((j = 0; j < $server_num; j++)); do
            to_ip=${server_ip_list[j]}
            to_port=${server_port_list[j]}
            if [ $i -ne $j ]; then
                is_local=false
            else
                is_local=true
            fi
            echo "select cuckoo_insert_foreign_server($id, '$name', '$ip', $port, $is_local, '$username');"
            psql -d postgres -h $to_ip -p $to_port -c "select cuckoo_insert_foreign_server($id, '$name', '$ip', $port, $is_local, '$username');"
        done
    done

    for ((i = 0; i < $server_num; i++)); do
        ip=${server_ip_list[i]}
        port=${server_port_list[i]}
        psql -d postgres -h $ip -p $port -c "select cuckoo_build_shard_table($shardcount);select cuckoo_create_distributed_data_table();select cuckoo_start_background_service();"
    done
fi

if [[ "$cnIp" == "$localIp" ]]; then
    psql -d postgres -h $cnIp -p ${cnPortPrefix}0 -c "SELECT cuckoo_plain_mkdir('/');"
fi
