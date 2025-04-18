#include "connection/node.h"
#include "test_cuckoo_store.h"

std::shared_ptr<CuckooConfig> CuckooStoreUT::config = nullptr;
std::shared_ptr<OpenInstance> CuckooStoreUT::openInstance = nullptr;
char *CuckooStoreUT::writeBuf = nullptr;
size_t CuckooStoreUT::size = 0;
char *CuckooStoreUT::readBuf = nullptr;
size_t CuckooStoreUT::readSize = 0;
char *CuckooStoreUT::readBuf2 = nullptr;

static Pipe s_pipe;

/*-------------------------------------------- Pipe --------------------------------------------*/

TEST_F(CuckooStoreUT, WaitPush)
{
    ResetBuf(true);
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/WaitPush", O_RDWR | O_CREAT);
    int ret = CuckooStore::GetInstance()->WriteFile(openInstance.get(), writeBuf, size, 0);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), true, true);
    EXPECT_EQ(ret, 0);
    ret = CuckooStore::GetInstance()->CloseTmpFiles(openInstance.get(), false, true);
    EXPECT_EQ(ret, 0);
    NewOpenInstance(100, StoreNode::GetInstance()->GetNodeId(), "/WaitPush", O_RDONLY);
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    ret = CuckooStore::GetInstance()->OpenFile(openInstance.get());
    EXPECT_EQ(ret, 0);

    int blockSize = config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE);
    s_pipe.Init(blockSize);
    ssize_t retSize = s_pipe.WaitPush(openInstance.get(), 0);
    EXPECT_EQ(retSize, blockSize);
}

TEST_F(CuckooStoreUT, WaitPop)
{
    int popSize = config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE) / 2;
    char *buf = (char *)malloc(popSize);
    bool end = false;
    ssize_t retSize = s_pipe.WaitPop(buf, popSize, end);
    EXPECT_EQ(retSize, popSize);
    EXPECT_FALSE(end);
    retSize = s_pipe.WaitPop(buf, popSize, end);
    EXPECT_EQ(retSize, popSize);
    EXPECT_TRUE(end);
    s_pipe.Destroy();
}

/*-------------------------------------------- ReadStream --------------------------------------------*/

TEST_F(CuckooStoreUT, ReadStreamInit)
{
    int fileBlocks = (openInstance->currentSize + CUCKOO_BLOCK_SIZE - 1) / CUCKOO_BLOCK_SIZE;

    EXPECT_TRUE(openInstance->readStream.Init(openInstance.get(), fileBlocks, CUCKOO_BLOCK_SIZE));
    openInstance->readStream.StartPushThreaded();
}

TEST_F(CuckooStoreUT, ReadStreamReadZero)
{
    ssize_t ret = openInstance->readStream.WaitPop(readBuf, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(CuckooStoreUT, ReadStreamReadExceed)
{
    size_t largeSize = 2 * config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE);
    char *buf = (char *)malloc(size);
    auto index = openInstance->readStream.pipeIndex;
    ssize_t ret = openInstance->readStream.WaitPop(buf, largeSize);
    EXPECT_EQ(ret, largeSize);
    EXPECT_EQ((index + 2) % openInstance->readStream.pipeNum, openInstance->readStream.pipeIndex);
    free(buf);
}

TEST_F(CuckooStoreUT, ReadStreamReadHalf)
{
    size_t halfSize = config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE) / 2;
    char *buf = (char *)malloc(halfSize);
    auto index = openInstance->readStream.pipeIndex;
    ssize_t ret = openInstance->readStream.WaitPop(buf, halfSize);
    EXPECT_EQ(ret, halfSize);
    EXPECT_EQ(index, openInstance->readStream.pipeIndex);
    ret = openInstance->readStream.WaitPop(buf, halfSize);
    EXPECT_EQ(ret, halfSize);
    EXPECT_EQ((index + 1) % openInstance->readStream.pipeNum, openInstance->readStream.pipeIndex);
    free(buf);
}

TEST_F(CuckooStoreUT, ReadStreamReadFull)
{
    size_t fullSize = config->GetUint32(CuckooPropertyKey::CUCKOO_BLOCK_SIZE);
    char *buf = (char *)malloc(fullSize);
    ssize_t ret = 0;
    while (ret == (ssize_t)readSize) {
        auto index = openInstance->readStream.pipeIndex;
        ret = openInstance->readStream.WaitPop(buf, fullSize);
        EXPECT_EQ(ret, fullSize);
        EXPECT_EQ((index + 1) % openInstance->readStream.pipeNum, openInstance->readStream.pipeIndex);
    }
    free(buf);
    openInstance = nullptr;
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
