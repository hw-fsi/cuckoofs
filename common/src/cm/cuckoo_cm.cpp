/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "cm/cuckoo_cm.h"

#include <fstream>

#include "log/logging.h"

constexpr int BUFF_SIZE = 512;

CuckooCM CuckooCM::singleton;
std::atomic<bool> CuckooCM::init = false;
int CuckooCM::initStatus = 0;

CuckooCM *CuckooCM::GetInstance(const std::string &zkEndPoint, int zkTimeout, const std::string &clusterName)
{
    if (init.exchange(true)) {
        // 如果已经初始化，不允许重复调用带参版本
        throw std::runtime_error("CuckooCM already initialized. Use GetInstance() without parameters.");
    }
    singleton.zkEndPoint = zkEndPoint;
    singleton.zkTimeout = zkTimeout;
    singleton.clusterName = clusterName;
    initStatus = singleton.Connect();
    return &singleton;
}

CuckooCM *CuckooCM::GetInstance()
{
    if (!init.load()) {
        throw std::runtime_error(
            "CuckooCM must be initialized first with GetInstance(zkEndPoint, zkTimeout, clusterName)");
    }
    return &singleton;
}

int CuckooCM::GetInitStatus() { return initStatus; }

void CuckooCM::DeleteInstance() { singleton.DestroyCM(); }

void CuckooCM::DestroyCM()
{
    if (zhandle) {
        int ret = zookeeper_close(zhandle);
        if (ret != ZOK) {
            CUCKOO_LOG(LOG_ERROR) << "zookeeper close failed : " << ret;
        }
        zhandle = nullptr;
    }
    init.store(false);
    isConnected = false;
    connectionFailed = false;
    isConning = false;
}

int CuckooCM::WaitForConnect()
{
    std::unique_lock<std::mutex> checkLock(zkMutex);
    zkCV.wait(checkLock, [this] { return isConnected || connectionFailed || isConning; });
    if (isConnected) {
        return RETURN_OK;
    } else if (isConning) {
        return RETURN_ERROR;
    }
    return RETURN_ERROR;
}

void CuckooCM::HandleConnected()
{
    std::unique_lock<std::mutex> checkLock(zkMutex);
    isConnected = true;
    if (isReconnection == 1) {
        ReUpload();
    }
    isReconnection = 0;
    zkCV.notify_one();
}

void CuckooCM::HandleNotConnected()
{
    std::unique_lock<std::mutex> checkLock(zkMutex);
    connectionFailed = true;
    zkCV.notify_one();
}

void CuckooCM::HandleConnecting()
{
    std::unique_lock<std::mutex> checkLock(zkMutex);
    isConning = true;
    zkCV.notify_one();
}

void CuckooCM::HandleExpired()
{
    std::unique_lock<std::mutex> checkLock(zkMutex);
    ReConnect();
}
void CuckooCM::InitWatcher(zhandle_t * /*zh*/, int type, int state, const char * /*path*/, void *ctx)
{
    auto *cuckooCM = static_cast<CuckooCM *>(ctx);
    if (type != ZOO_SESSION_EVENT) {
        CUCKOO_LOG(LOG_ERROR) << "Wrong zoo type!";
        return;
    }
    if (state == ZOO_CONNECTED_STATE) {
        cuckooCM->HandleConnected();
        CUCKOO_LOG(LOG_INFO) << "connection success:" << ZOO_CONNECTED_STATE;
    } else if (state == ZOO_CONNECTING_STATE) {
        cuckooCM->HandleConnecting();
        CUCKOO_LOG(LOG_INFO) << "zk connecting : " << state;
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
        cuckooCM->HandleExpired();
        CUCKOO_LOG(LOG_ERROR) << "zk expired";
    } else {
        cuckooCM->HandleNotConnected();
        CUCKOO_LOG(LOG_ERROR) << "Connecte failed : " << state;
        exit(1);
    }
}

int CuckooCM::Connect()
{
    if (zhandle != nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "Connect event repeat init";
        return RETURN_ERROR;
    }
    zhandle = zookeeper_init(zkEndPoint.c_str(), CuckooCM::InitWatcher, zkTimeout, nullptr, this, 0);
    if (zhandle == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "zookeeper connect failed";
        return RETURN_ERROR;
    }
    return WaitForConnect();
}

int CuckooCM::CheckStatusAndReconnect()
{
    int connectedStat = zoo_state(zhandle);
    int ret = ZOK;
    if (connectedStat == ZOO_CONNECTED_STATE) {
        return ret;
    }
    if (zhandle) {
        ret = zookeeper_close(zhandle);
        zhandle = nullptr;
        // exit(1);
    }
    ret = Connect();
    return ret;
}

int CuckooCM::ReConnect()
{
    CUCKOO_LOG(LOG_INFO) << "zk connect session expired, reconnect it";
    int ret = ZOK;
    if (zhandle) {
        ret = zookeeper_close(zhandle);
        zhandle = nullptr;
    }
    zhandle = zookeeper_init(zkEndPoint.c_str(), CuckooCM::InitWatcher, zkTimeout, nullptr, this, 0);
    if (zhandle == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "reconnection zk failed";
    }
    isReconnection = 1;
    return ret;
}

int CuckooCM::FetchLeaders(std::vector<std::string> &leaders)
{
    int ret = 0;
    std::string leaderPath = clusterName + "/leaders";
    String_vector leaderNames;
    ret = zoo_get_children(zhandle, leaderPath.c_str(), 0, &leaderNames);
    if (ret != ZOK) {
        CUCKOO_LOG(LOG_ERROR) << "zoo_get_children failed : " << ret;
        return RETURN_ERROR;
    }
    int n = leaderNames.count;
    for (int i = 0; i < n; i++) {
        std::string childName = leaderNames.data[i];
        std::cout << childName << std::endl;
        std::string nodeLeaderPath = leaderPath + "/" + childName;
        char buf[BUFF_SIZE] = {0};
        int len = BUFF_SIZE;
        int getRet = zoo_get(zhandle, nodeLeaderPath.c_str(), 0, buf, &len, nullptr);
        if (getRet != ZOK) {
            deallocate_String_vector(&leaderNames);
            CUCKOO_LOG(LOG_ERROR) << "zookeeper get node info failed :" << getRet;
            return RETURN_ERROR;
        }
        std::string nodeInfo = buf;
        leaders.push_back(nodeInfo);
    }
    deallocate_String_vector(&leaderNames);
    return RETURN_OK;
}

int CuckooCM::FetchCNLeader(std::string &cnLeader)
{
    int ret = 0;
    std::string cnLeaderPath = clusterName + "/leaders/cn";
    char buf[BUFF_SIZE] = {0};
    int len = BUFF_SIZE;
    ret = zoo_get(zhandle, cnLeaderPath.c_str(), 0, buf, &len, nullptr);
    if (ret != ZOK) {
        CUCKOO_LOG(LOG_ERROR) << "zookeeper get node info failed :" << ret;
        return RETURN_ERROR;
    }
    cnLeader = buf;
    return RETURN_OK;
}

int CuckooCM::FetchCoordinatorInfo(std::string &coordinatorIp, int &coordinatorPort)
{
    std::string cnLeaderInfo;
    int ret = FetchCNLeader(cnLeaderInfo);
    if (ret != 0 || cnLeaderInfo.empty()) {
        return RETURN_ERROR;
    }
    std::string::size_type iPos = cnLeaderInfo.find(':');
    if (iPos == std::string::npos) {
        return RETURN_ERROR;
    }
    coordinatorIp = cnLeaderInfo.substr(0, iPos);
    coordinatorPort = std::atoi(cnLeaderInfo.substr(iPos + 1, cnLeaderInfo.length() - iPos - 1).c_str()) + 10;
    return RETURN_OK;
}

void clusterStatusCallback(zhandle_t * /*zh*/, int type, int /*state*/, const char * /*path*/, void *watcherCtx)
{
    auto *cuckooCM = static_cast<CuckooCM *>(watcherCtx);
    if (type == ZOO_CHANGED_EVENT) {
        cuckooCM->UpdateNodeStatus();
    }
}

void MetaDataStatusCallback(zhandle_t * /*zh*/, int type, int /*state*/, const char * /*path*/, void *watcherCtx)
{
    auto *cuckooCM = static_cast<CuckooCM *>(watcherCtx);
    if (type == ZOO_CREATED_EVENT) {
        cuckooCM->UpdateMetaDataStatus();
    }
}

int CuckooCM::Upload(const std::string & /*path*/, std::string &nodeInfoParam, int &nodeIdParam, std::string &rootPath)
{
    int ret = 0;
    std::string myidPath = rootPath + "/myid";
    std::string nodePath = clusterName + "/StoreNode/Nodes/Node";
    int tmpNodeId = -1;
    if (access(myidPath.c_str(), F_OK) == 0) {
        std::ifstream fin;
        fin.open(myidPath.c_str(), std::ios::in);
        if (!fin.is_open()) {
            CUCKOO_LOG(LOG_ERROR) << "open existed myid faile failed " << strerror(errno);
            return RETURN_ERROR;
        }
        fin >> tmpNodeId;
        fin.close();
    }
    char nodeBuffer[BUFF_SIZE];
    if (tmpNodeId == -1) {
        ret = zoo_create(zhandle,
                         nodePath.c_str(),
                         nodeInfoParam.c_str(),
                         nodeInfoParam.length(),
                         &ZOO_OPEN_ACL_UNSAFE,
                         ZOO_SEQUENCE | ZOO_EPHEMERAL,
                         nodeBuffer,
                         BUFF_SIZE);
        if (ret != ZOK) {
            CUCKOO_LOG(LOG_ERROR) << "create node failed in zk " << ret;
            return RETURN_ERROR;
        }
    } else {
        nodePath = nodePath + "000" + std::to_string(tmpNodeId);
        ret = zoo_create(zhandle,
                         nodePath.c_str(),
                         nodeInfoParam.c_str(),
                         nodeInfoParam.length(),
                         &ZOO_OPEN_ACL_UNSAFE,
                         ZOO_EPHEMERAL,
                         nodeBuffer,
                         BUFF_SIZE);
        if (ret != ZOK) {
            CUCKOO_LOG(LOG_ERROR) << "create node failed in zk " << ret;
            return RETURN_ERROR;
        }
    }
    std::string nodeName = std::string(nodeBuffer);
    size_t iPos = nodeName.find_last_of('/');
    std::string nodeIdStr(nodeName.substr(iPos + 5, nodeName.length() - iPos - 5));
    nodeIdParam = atoi(nodeIdStr.c_str());
    if (tmpNodeId == -1) {
        CUCKOO_LOG(LOG_INFO) << "write to local myid: " << myidPath;
        std::ofstream fout;
        fout.open(myidPath.c_str(), std::ios::out);
        if (!fout.is_open()) {
            CUCKOO_LOG(LOG_ERROR) << "create myid faile failed " << strerror(errno);
            return RETURN_ERROR;
        }

        fout << nodeIdParam;
        fout.close();
    }

    nodeId = nodeIdParam;
    nodeInfo = nodeInfoParam;

    std::string statusPath = clusterName + "/StoreNode/storeNodeStatus";
    char buf[BUFF_SIZE] = {0};
    int length = BUFF_SIZE;
    ret = zoo_wget(zhandle, statusPath.c_str(), clusterStatusCallback, this, buf, &length, nullptr);
    if (ret != ZOK) {
        CUCKOO_LOG(LOG_ERROR) << "wget failed in zk " << ret;
        return RETURN_ERROR;
    }
    if (strcmp(buf, "1") == 0) {
        UpdateNodeStatus();
    }
    return RETURN_OK;
}

int CuckooCM::FetchStoreNodes(std::unordered_map<int, std::string> &storeNodes)
{
    int ret = 0;
    std::string storeClusterViewPath = clusterName + "/StoreNode/Nodes";
    String_vector clusterViews;
    ret = zoo_get_children(zhandle, storeClusterViewPath.c_str(), 0, &clusterViews);
    if (ret != ZOK) {
        CUCKOO_LOG(LOG_ERROR) << "zookeeper get children failed :" << ret;
        return ret;
    }
    int n = clusterViews.count;
    storeNodes.clear();
    for (int i = 0; i < n; ++i) {
        std::string childName = clusterViews.data[i];
        std::string nodePath = storeClusterViewPath + "/" + childName;
        char buf[BUFF_SIZE] = {0};
        int len = BUFF_SIZE;
        int getRet = zoo_get(zhandle, nodePath.c_str(), 0, buf, &len, nullptr);
        if (getRet != ZOK) {
            deallocate_String_vector(&clusterViews);
            CUCKOO_LOG(LOG_ERROR) << "zookeeper get node info failed : " << getRet;
            return ret;
        }
        std::string nodeIdStr(childName.substr(5, childName.length() - 5));
        int nodeId = atoi(nodeIdStr.c_str());
        storeNodes.emplace(nodeId, std::string(buf));
    }
    deallocate_String_vector(&clusterViews);
    return RETURN_OK;
}

int CuckooCM::ReUpload()
{
    int ret = 0;
    std::string nodePath = clusterName + "/StoreNode/Nodes/Node00" + std::to_string(nodeId);
    std::cout << nodePath << std::endl;
    ret = zoo_create(zhandle,
                     nodePath.c_str(),
                     nodeInfo.c_str(),
                     nodeInfo.length(),
                     &ZOO_OPEN_ACL_UNSAFE,
                     ZOO_EPHEMERAL,
                     nullptr,
                     0);
    if (ret != ZOK) {
        CUCKOO_LOG(LOG_ERROR) << "create node failed in zk " << ret;
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

bool CuckooCM::GetNodeStatus() { return ready.load(); }

void CuckooCM::CheckMetaDataStatus()
{
    std::string metaDataStatusPath = clusterName + "/ready";
    int ret = zoo_wexists(zhandle, metaDataStatusPath.c_str(), MetaDataStatusCallback, this, nullptr);
    if (ret == ZOK) {
        /*
                the ready node exists
        */
        UpdateMetaDataStatus();
    }
}

bool CuckooCM::GetMetaDataStatus() { return metaDataReady.load(); }

int CuckooCM::unsetNodeStatus()
{
    ready.store(false);
    return 0;
}

void CuckooCM::UpdateNodeStatus()
{
    if (!ready.load()) {
        ready.store(true);
        storeNodeCompleteCv.notify_all();
    }
}

void CuckooCM::UpdateMetaDataStatus()
{
    if (!metaDataReady.load()) {
        metaDataReady.store(true);
        metaDataReadyCv.notify_all();
    }
}

std::condition_variable &CuckooCM::GetStoreNodeCompleteCv() { return storeNodeCompleteCv; }

std::condition_variable &CuckooCM::GetMetaDataReadyCv() { return metaDataReadyCv; }
