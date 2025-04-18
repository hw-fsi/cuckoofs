#include "test_cuckoo_store.h"

#include "connection/node.h"

std::shared_ptr<CuckooConfig> CuckooStoreUT::config = GetInit().GetCuckooConfig();
std::shared_ptr<OpenInstance> CuckooStoreUT::openInstance = nullptr;
char *CuckooStoreUT::writeBuf = nullptr;
size_t CuckooStoreUT::size = 0;
char *CuckooStoreUT::readBuf = nullptr;
size_t CuckooStoreUT::readSize = 0;
char *CuckooStoreUT::readBuf2 = nullptr;

/* ------------------------------------------- open local -------------------------------------------*/

TEST_F(CuckooStoreUT, CreateLocalWRonly)
{
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/OpenLocal", O_WRONLY | O_CREAT);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenLocalWRonlyExist)
{
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/OpenLocal", O_WRONLY);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenLocalWRonlyNoneExist)
{
    NewOpenInstance(101, StoreNode::GetInstance()->GetNodeId(), "/OpenLocalNoneExist", O_WRONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, -EIO);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

TEST_F(CuckooStoreUT, OpenLocalRDonlyExist)
{
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/OpenLocal", O_RDONLY);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenLocalRDonlyNoneExist)
{
    NewOpenInstance(101, StoreNode::GetInstance()->GetNodeId(), "/OpenLocalNoneExist", O_RDONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

TEST_F(CuckooStoreUT, OpenLocalRDWRExist)
{
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/OpenLocal", O_RDWR);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenLocalRDWRNoneExist)
{
    NewOpenInstance(101, StoreNode::GetInstance()->GetNodeId(), "/OpenLocalNoneExist", O_RDWR);

    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- open remote -------------------------------------------*/

TEST_F(CuckooStoreUT, CreateRemoteWRonly)
{
    NewOpenInstance(200, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemote", O_WRONLY | O_CREAT);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenRemoteWRonlyExist)
{
    NewOpenInstance(200, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemote", O_WRONLY);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenRemoteWRonlyNoneExist)
{
    NewOpenInstance(201, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemoteNoneExist", O_WRONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, -EIO);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

TEST_F(CuckooStoreUT, OpenRemoteRDonlyExist)
{
    NewOpenInstance(200, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemote", O_RDONLY);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenRemoteRDonlyNoneExist)
{
    NewOpenInstance(201, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemoteNoneExist", O_RDONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

TEST_F(CuckooStoreUT, OpenRemoteRDWRExist)
{
    NewOpenInstance(200, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemote", O_RDWR);

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, OpenRemoteRDWRNoneExist)
{
    NewOpenInstance(201, StoreNode::GetInstance()->GetNodeId() + 1, "/OpenRemoteNoneExist", O_RDWR);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, -EIO);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- write local -------------------------------------------*/

TEST_F(CuckooStoreUT, WriteLocalLarge)
{
    NewOpenInstance(1000, StoreNode::GetInstance()->GetNodeId(), "/WriteLocal", O_WRONLY | O_CREAT);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE + 1;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), size);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteLocalZero)
{
    NewOpenInstance(1000, StoreNode::GetInstance()->GetNodeId(), "/WriteLocal", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE + 1;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, 0, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), 0);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteLocalSeq)
{
    NewOpenInstance(1000, StoreNode::GetInstance()->GetNodeId(), "/WriteLocal", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 2;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");
    // local no buffer cache
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    // perisist previous to cache file, new to buffer
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size * 2);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), size * 3);

    free(buf);
}

TEST_F(CuckooStoreUT, WriteLocalRandom)
{
    NewOpenInstance(1000, StoreNode::GetInstance()->GetNodeId(), "/WriteLocal", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 2;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), size * 2);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteLocalSeqToRandom)
{
    NewOpenInstance(1000, StoreNode::GetInstance()->GetNodeId(), "/WriteLocal", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 4;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), size * 2);
    free(buf);
}

/* ------------------------------------------- write remote -------------------------------------------*/

TEST_F(CuckooStoreUT, WriteRemoteLarge)
{
    NewOpenInstance(2000, StoreNode::GetInstance()->GetNodeId() + 1, "/WriteRemote", O_WRONLY | O_CREAT);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE + 1;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), size);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteRemoteZero)
{
    NewOpenInstance(2000, StoreNode::GetInstance()->GetNodeId() + 1, "/WriteRemote", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE + 1;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, 0, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, 0);
    EXPECT_EQ(openInstance->currentSize.load(), 0);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteRemoteSeq)
{
    NewOpenInstance(2000, StoreNode::GetInstance()->GetNodeId() + 1, "/WriteRemote", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 2;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");
    // write to buffer
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size * 2);
    // perisist previous to cache file, new to buffer
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size * 2);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size);
    EXPECT_EQ(openInstance->currentSize.load(), size * 3);

    free(buf);
}

TEST_F(CuckooStoreUT, WriteRemoteRandom)
{
    NewOpenInstance(2000, StoreNode::GetInstance()->GetNodeId() + 1, "/WriteRemote", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 2;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size);
    EXPECT_EQ(openInstance->currentSize.load(), size * 2);
    free(buf);
}

TEST_F(CuckooStoreUT, WriteRemoteSeqToRandom)
{
    NewOpenInstance(2000, StoreNode::GetInstance()->GetNodeId() + 1, "/WriteRemote", O_WRONLY);

    size_t size = CUCKOO_STORE_STREAM_MAX_SIZE / 4;
    char *buf = (char *)malloc(size);
    strcpy(buf, "abc");

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, size);
    EXPECT_EQ(ret, 0);
    auto bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size * 2);
    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), buf, size, 0);
    EXPECT_EQ(ret, 0);
    bufferedSize = openInstance->writeStream.GetSize();
    EXPECT_EQ(bufferedSize, size);
    EXPECT_EQ(openInstance->currentSize.load(), size * 2);
    free(buf);
}

/* ------------------------------------------- read local -------------------------------------------*/

TEST_F(CuckooStoreUT, ReadLocalSeqSmall)
{
    NewOpenInstance(10000, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalSmall", O_WRONLY | O_CREAT);
    ResetBuf(false);
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(10000, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalSmall", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    auto buffer = std::shared_ptr<char>((char *)malloc(size), free);
    openInstance->readBuffer = buffer;
    openInstance->readBufferSize = size;
    ret = CuckooStore::GetInstance()->ReadSmallFiles(openInstance.get());
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadLocalRandomSmall)
{
    NewOpenInstance(10000, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalSmall", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    auto buffer = std::shared_ptr<char>((char *)malloc(size), free);
    openInstance->readBuffer = buffer;
    openInstance->readBufferSize = size;
    int ret = CuckooStore::GetInstance()->ReadSmallFiles(openInstance.get());
    EXPECT_EQ(ret, 0);

    memset(readBuf, 0, readSize);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadLocalSeqLarge)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_WRONLY | O_CREAT);
    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadLocalRandomLarge)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadLocalSeqToRandomLarge)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    // not serial
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadLocalExceed)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);
    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size - 1);
    EXPECT_EQ(ret, 1);
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReadLocalHole)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_WRONLY);
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, size * 2);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDONLY);
    openInstance->originalSize = size * 3;
    openInstance->currentSize = size * 3;

    memset(readBuf, 0, readSize);
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size);
    EXPECT_EQ(ret, readSize);
    void *zeroBlock = std::memset(new char[readSize], 0, readSize);
    bool result = std::memcmp(readBuf, zeroBlock, readSize) == 0;
    EXPECT_TRUE(result);
    delete[] static_cast<char *>(zeroBlock);
}

/* ------------------------------------------- read remote -------------------------------------------*/

TEST_F(CuckooStoreUT, ReadRemoteSeqSmall)
{
    NewOpenInstance(20000, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteSmall", O_WRONLY | O_CREAT);
    ResetBuf(false);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(20000, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteSmall", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    auto buffer = std::shared_ptr<char>((char *)malloc(size), free);
    openInstance->readBuffer = buffer;
    openInstance->readBufferSize = size;
    ret = CuckooStore::GetInstance()->ReadSmallFiles(openInstance.get());
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadRemoteRandomSmall)
{
    NewOpenInstance(20000, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteSmall", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    auto buffer = std::shared_ptr<char>((char *)malloc(size), free);
    openInstance->readBuffer = buffer;
    openInstance->readBufferSize = size;
    int ret = CuckooStore::GetInstance()->ReadSmallFiles(openInstance.get());
    EXPECT_EQ(ret, 0);

    memset(readBuf, 0, readSize);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadRemoteSeqLarge)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_WRONLY | O_CREAT);
    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadRemoteRandomLarge)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadRemoteSeqToRandomLarge)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
    // not serial
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadRemoteExceed)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);
    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size - 1);
    EXPECT_EQ(ret, 1);
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReadRemoteHole)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_WRONLY);
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, size * 2);
    EXPECT_EQ(ret, 0);

    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDONLY);
    openInstance->originalSize = size * 3;
    openInstance->currentSize = size * 3;

    memset(readBuf, 0, readSize);
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, size);
    EXPECT_EQ(ret, readSize);
    void *zeroBlock = std::memset(new char[readSize], 0, readSize);
    bool result = std::memcmp(readBuf, zeroBlock, readSize) == 0;
    EXPECT_TRUE(result);
    delete[] static_cast<char *>(zeroBlock);
}

/* ------------------------------------------- RDWR local -------------------------------------------*/
// all large file
TEST_F(CuckooStoreUT, PrereadWriteLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReadWriteLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, PrereadWriteReadLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadWriteReadLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, WritePreReadWriteLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    // pre read should be stopped
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, WriteReadWriteLocal)
{
    NewOpenInstance(10001, StoreNode::GetInstance()->GetNodeId(), "/ReadLocalLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    // pre read should be stopped
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

/* ------------------------------------------- RDWR remote -------------------------------------------*/
// all large file
TEST_F(CuckooStoreUT, PrereadWriteRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReadWriteRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, PrereadWriteReadRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, ReadWriteReadRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));
}

TEST_F(CuckooStoreUT, WritePreReadWriteRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    // pre read should be stopped
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, readSize);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf + readSize, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, WriteReadWriteRemote)
{
    NewOpenInstance(20001, StoreNode::GetInstance()->GetNodeId() + 1, "/ReadRemoteLarge", O_RDWR);
    openInstance->originalSize = size;
    openInstance->currentSize = size;

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    // pre read should be stopped
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));
    ret = CuckooStore::GetInstance()->ReadFile(openInstance.get(), readBuf, readSize, 0);
    EXPECT_EQ(ret, readSize);
    EXPECT_EQ(0, memcmp(writeBuf, readBuf, readSize));

    ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
}

/* ------------------------------------------- close local -------------------------------------------*/

TEST_F(CuckooStoreUT, FlushLocal)
{
    NewOpenInstance(100000, StoreNode::GetInstance()->GetNodeId(), "/CloseLocal", O_RDWR | O_CREAT);

    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReleaseLocal)
{
    NewOpenInstance(100000, StoreNode::GetInstance()->GetNodeId(), "/CloseLocal", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReleaseWithoutFlushLocal)
{
    NewOpenInstance(100000, StoreNode::GetInstance()->GetNodeId(), "/CloseLocal", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, FlushTwiceLocal)
{
    NewOpenInstance(100000, StoreNode::GetInstance()->GetNodeId(), "/CloseLocal", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

/* ------------------------------------------- close remote -------------------------------------------*/

TEST_F(CuckooStoreUT, FlushRemote)
{
    NewOpenInstance(200000, StoreNode::GetInstance()->GetNodeId() + 1, "/CloseRemote", O_RDWR | O_CREAT);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReleaseRemote)
{
    NewOpenInstance(200000, StoreNode::GetInstance()->GetNodeId() + 1, "/CloseRemote", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReleaseWithoutFlushRemote)
{
    NewOpenInstance(200000, StoreNode::GetInstance()->GetNodeId() + 1, "/CloseRemote", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, FlushTwiceRemote)
{
    NewOpenInstance(200000, StoreNode::GetInstance()->GetNodeId() + 1, "/CloseRemote", O_RDWR);

    memset(readBuf, 0, readSize);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, CUCKOO_STORE_STREAM_MAX_SIZE, 0);
    EXPECT_EQ(ret, 0);

    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
}

/* ------------------------------------------- delete local -------------------------------------------*/

TEST_F(CuckooStoreUT, DeleteLocal)
{
    uint64_t inodeId = 100;
    int nodeId = StoreNode::GetInstance()->GetNodeId();
    std::string path = "/OpenLocal";
    int ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 1000;
    path = "/WriteLocal";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 10000;
    path = "/ReadLocalSmall";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 10001;
    path = "/ReadLocalLarge";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 100000;
    path = "/CloseLocal";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, DeleteLocalNoneExist)
{
    uint64_t inodeId = 1000000;
    int nodeId = StoreNode::GetInstance()->GetNodeId();
    std::string path = "/DeleteLocalNoneExist";
    int ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- delete remote -------------------------------------------*/

TEST_F(CuckooStoreUT, DeleteRemote)
{
    uint64_t inodeId = 200;
    int nodeId = StoreNode::GetInstance()->GetNodeId();
    std::string path = "/OpenRemote";
    int ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 2000;
    path = "/WriteRemote";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 20000;
    path = "/ReadRemoteSmall";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 20001;
    path = "/ReadRemoteLarge";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);

    inodeId = 200000;
    path = "/CloseRemote";
    ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, DeleteRemoteNoneExist)
{
    uint64_t inodeId = 2000000;
    int nodeId = StoreNode::GetInstance()->GetNodeId() + 1;
    std::string path = "/DeleteRemoteNoneExist";
    int ret = CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- statFs -------------------------------------------*/

TEST_F(CuckooStoreUT, StatFs)
{
    struct statvfs vfsbuf;
    int ret = CuckooStore::GetInstance()->StatFS(&vfsbuf);
    EXPECT_EQ(ret, 0);
}

/* ------------------------------------------- truncate file local -------------------------------------------*/

TEST_F(CuckooStoreUT, TruncateFileLocal)
{
    NewOpenInstance(10000000, StoreNode::GetInstance()->GetNodeId(), "/TruncateFileLocal", O_WRONLY | O_CREAT);

    int ret = CuckooStore::GetInstance()->TruncateFile(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->DeleteFiles(openInstance->inodeId, openInstance->nodeId, openInstance->path);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, TruncateFileLocalNoneExist)
{
    NewOpenInstance(10000001, StoreNode::GetInstance()->GetNodeId(), "/TruncateFileLocalNoneExist", O_WRONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->TruncateFile(openInstance.get(), 1000);
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, -EIO);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- truncate file remote -------------------------------------------*/

TEST_F(CuckooStoreUT, TruncateFileRemote)
{
    NewOpenInstance(20000000, StoreNode::GetInstance()->GetNodeId() + 1, "/TruncateFileRemote", O_WRONLY | O_CREAT);

    int ret = CuckooStore::GetInstance()->TruncateFile(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->DeleteFiles(openInstance->inodeId, openInstance->nodeId, openInstance->path);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, TruncateFileRemoteNoneExist)
{
    NewOpenInstance(20000001, StoreNode::GetInstance()->GetNodeId() + 1, "/TruncateFileRemoteNoneExist", O_WRONLY);
    openInstance->originalSize = 1;

    int ret = CuckooStore::GetInstance()->TruncateFile(openInstance.get(), 1000);
    if (config->GetBool(CuckooPropertyKey::CUCKOO_PERSIST)) {
        EXPECT_EQ(ret, -EIO);
    } else {
        EXPECT_EQ(ret, -ENOENT);
    }
}

/* ------------------------------------------- truncate openInstance local -------------------------------------------*/

TEST_F(CuckooStoreUT, TruncateOpenInstanceLocal)
{
    NewOpenInstance(10000010, StoreNode::GetInstance()->GetNodeId(), "/TruncateOpenInstanceLocal", O_WRONLY);

    int ret = CuckooStore::GetInstance()->TruncateOpenInstance(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 1000);
    EXPECT_EQ(openInstance->currentSize, 1000);
}

TEST_F(CuckooStoreUT, WriteTruncateOpenInstanceLocal)
{
    NewOpenInstance(10000010, StoreNode::GetInstance()->GetNodeId(), "/TruncateOpenInstanceLocal", O_WRONLY | O_CREAT);

    ResetBuf(true);

    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 0);
    EXPECT_EQ(openInstance->currentSize, size);

    ret = CuckooStore::GetInstance()->TruncateOpenInstance(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 1000);
    EXPECT_EQ(openInstance->currentSize, 1000);
}

/* ------------------------------------------- truncate openInstance remote
 * -------------------------------------------*/

TEST_F(CuckooStoreUT, TruncateOpenInstanceRemote)
{
    NewOpenInstance(20000010, StoreNode::GetInstance()->GetNodeId() + 1, "/TruncateOpenInstanceRemote", O_WRONLY);

    int ret = CuckooStore::GetInstance()->TruncateOpenInstance(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 1000);
    EXPECT_EQ(openInstance->currentSize, 1000);
}

TEST_F(CuckooStoreUT, WriteTruncateOpenInstanceRemote)
{
    NewOpenInstance(20000010,
                    StoreNode::GetInstance()->GetNodeId() + 1,
                    "/TruncateOpenInstanceRemote",
                    O_WRONLY | O_CREAT);

    ResetBuf(true);
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 0);
    EXPECT_EQ(openInstance->currentSize, size);

    ret = CuckooStore::GetInstance()->TruncateOpenInstance(openInstance.get(), 1000);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(openInstance->originalSize, 1000);
    EXPECT_EQ(openInstance->currentSize, 1000);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
