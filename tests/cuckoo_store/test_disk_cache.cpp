#include "test_disk_cache.h"
#include "disk_cache/disk_cache.h"

std::string DiskCacheUT::rootPath = "/tmp/testdir/";

TEST_F(DiskCacheUT, Start)
{
    int ret = DiskCache::GetInstance().Start(rootPath, 100, 0.2, 0.2);
    EXPECT_EQ(ret, 0);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
