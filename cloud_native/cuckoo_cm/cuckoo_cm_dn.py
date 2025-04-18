import os
import time
import logging
from utils import logger
from cm.cuckoo_cm import *


def main():
    data_dir =os.environ.get("data_dir", "/home/cuckooMeta/data")
    log_level = os.environ.get("cm_log_level", "INFO")
    cm_log_data_dir = os.path.join(data_dir, "cmlog")
    logger.logging_init(cm_log_data_dir, log_level)

    cuckoo_cm = CuckooCM(is_cn=False)
    cuckoo_cm.connect_zk()
    cuckoo_cm.init_sys()
    is_leader = cuckoo_cm.leader_select()
    if is_leader:
        if not cuckoo_cm.is_sys_ready():
            cuckoo_cm.watch_leader_and_candidates()
        cuckoo_cm.watch_replicas()
    else:
        if not cuckoo_cm.is_sys_ready():
            cuckoo_cm.watch_leader_and_candidates()
        cuckoo_cm.write_replica()
    cuckoo_cm.watch_conn()
    cuckoo_cm.monitor_store_node()
    time.sleep(30)
    cuckoo_cm.set_store_cluster_ready()
    cuckoo_cm.start_watch_replica_thread()


if __name__ == "__main__":
    main()