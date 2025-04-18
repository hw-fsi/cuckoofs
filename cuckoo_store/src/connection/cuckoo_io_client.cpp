/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "connection/cuckoo_io_client.h"

#include "log/logging.h"

static int BrpcErrorCodeToFuseErrno(int brpcErrorCode)
{
    // append more case here if you encountered EFAULT
    switch (brpcErrorCode) {
    case 0:
        return 0;
    case brpc::ENOSERVICE:
    case brpc::ENOMETHOD:
        return EOPNOTSUPP;
    case brpc::EREQUEST:
        return EINVAL;
    case brpc::ERPCAUTH:
        return EPERM;
    case brpc::ERPCTIMEDOUT:
        return ETIMEDOUT;
    case brpc::EFAILEDSOCKET:
        return EIO;
    default:
        return EFAULT;
    }
}

/* return 0: OK; return negative: remote IO error, return positive: network error */
int CuckooIOClient::OpenFile(uint64_t inodeId,
                             int oflags,
                             uint64_t &physicalFd,
                             uint64_t originalSize,
                             const std::string &path,
                             bool nodeFail)
{
    cuckoo::brpc_io::OpenRequest request;
    request.set_inode_id(inodeId);
    request.set_oflags(oflags);
    request.set_path(path);
    request.set_size(originalSize);
    request.set_node_fail(nodeFail);
    cuckoo::brpc_io::OpenReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->OpenFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "Open file by brpc failed " << cntl.ErrorText() << "error code: " << cntl.ErrorCode();
        return BrpcErrorCodeToFuseErrno(cntl.ErrorCode()); // positive reply
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::OpenFile failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    physicalFd = response.physical_fd();
    CUCKOO_LOG(LOG_INFO) << "Open file successfully! you have opened cuckooFd: " << physicalFd;
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::CloseFile(uint64_t physicalFd,
                              bool isFlush,
                              bool isSync,
                              const char *buf,
                              size_t size,
                              off_t offset)
{
    cuckoo::brpc_io::CloseRequest request;
    request.set_physical_fd(physicalFd);
    request.set_flush(isFlush);
    request.set_sync(isSync);
    request.set_offset(offset);

    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);
    auto dummyDeleter = [](void *) -> void {};
    cntl.request_attachment().append_user_data((void *)buf, size, dummyDeleter);

    stub->CloseFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "Close file by brpc failed " << cntl.ErrorText() << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::CloseFile failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    CUCKOO_LOG(LOG_INFO) << "Close file successfully!";
    return 0;
}

// return positive: read length, return negative error of both network and IO
int CuckooIOClient::ReadFile(uint64_t /*inodeId*/,
                             int /*oflags*/,
                             char *readBuffer,
                             uint64_t &physicalFd,
                             int bufferSize,
                             off_t offset,
                             const std::string &path)
{
    // have to open file before call this method
    cuckoo::brpc_io::ReadRequest request;
    request.set_physical_fd(physicalFd);
    request.set_offset(offset);
    request.set_read_size(bufferSize);
    request.set_path(path);
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->ReadFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "Read file by brpc failed " << cntl.ErrorText() << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode()); // -errno
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::ReadFile failed: " << strerror(-response.error_code());
        return response.error_code(); // negative reply
    }

    int retLen = cntl.response_attachment().size();
    if (retLen > bufferSize) {
        CUCKOO_LOG(LOG_ERROR) << "Return more bytes than requested.";
        return -EIO;
    }

    cntl.response_attachment().cutn(readBuffer, retLen);
    CUCKOO_LOG(LOG_INFO) << "In CuckooIOClient::ReadFile(): read file successfully! you have read: " << retLen
                         << " bytes";
    return retLen;
}

// return 0: OK; return negative: remote IO error, return positive: network error
ssize_t CuckooIOClient::ReadSmallFile(uint64_t inodeId,
                                      ssize_t size,
                                      std::string &path,
                                      char *readBuffer,
                                      int oflags,
                                      bool nodeFail)
{
    cuckoo::brpc_io::ReadSmallFileRequest request;
    request.set_inode_id(inodeId);
    request.set_read_size(size);
    request.set_path(path);
    request.set_oflags(oflags);
    request.set_node_fail(nodeFail);
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->ReadSmallFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "Read small file by brpc failed " << cntl.ErrorText()
                              << "error code: " << cntl.ErrorCode();
        return BrpcErrorCodeToFuseErrno(cntl.ErrorCode()); // positive reply
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::ReadSmallFile failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    int retLen = cntl.response_attachment().size();
    if (retLen != size) {
        CUCKOO_LOG(LOG_ERROR) << "Return bytes doesn't equal to requested.";
        return -EIO;
    }

    cntl.response_attachment().cutn(readBuffer, retLen);
    CUCKOO_LOG(LOG_INFO) << "In CuckooIOClient::ReadSmallFile(): read file successfully! you have read: " << size
                         << " bytes";
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::WriteFile(uint64_t physicalFd, const char *writeBuffer, uint64_t size, off_t offset)
{
    cuckoo::brpc_io::WriteRequest request;
    request.set_physical_fd(physicalFd);
    request.set_offset(offset);
    cuckoo::brpc_io::WriteReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);
    auto dummyDeleter = [](void *) -> void {};
    cntl.request_attachment().append_user_data((void *)writeBuffer, size, dummyDeleter);

    stub->WriteFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "WriteFile by brpc failed " << cntl.ErrorText() << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::WriteFile failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    int writedSize = response.write_size();
    if ((uint64_t)writedSize != size) {
        CUCKOO_LOG(LOG_ERROR) << "Write size doesn't equal to requested.";
        return -EIO;
    }

    CUCKOO_LOG(LOG_INFO) << "In CuckooIOClient::WriteFile(): write file successfully! you have write: " << writedSize
                         << " bytes";
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::DeleteFile(uint64_t inodeId, int nodeId, std::string &path)
{
    cuckoo::brpc_io::DeleteRequest request;
    request.set_inode_id(inodeId);
    request.set_node_id(nodeId);
    request.set_path(path);
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->DeleteFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "Delete file by brpc failed " << cntl.ErrorText()
                              << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::Delete failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    CUCKOO_LOG(LOG_INFO) << "Delete file successfully!";
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::StatFS(std::string &path, struct StatFSBuf *fsBuf)
{
    cuckoo::brpc_io::StatFSRequest request;
    request.set_path(path);
    cuckoo::brpc_io::StatFSReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->StatFS(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "StatFS by brpc failed " << cntl.ErrorText() << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::StatFS failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    fsBuf->f_blocks = response.fblocks();
    fsBuf->f_bfree = response.fbfree();
    fsBuf->f_bavail = response.fbavail();
    fsBuf->f_files = response.ffiles();
    fsBuf->f_ffree = response.fffree();
    CUCKOO_LOG(LOG_INFO) << "StatFS successfully!";
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::TruncateOpenInstance(uint64_t physicalFd, off_t size)
{
    cuckoo::brpc_io::TruncateOpenInstanceRequest request;
    request.set_physical_fd(physicalFd);
    request.set_size(size);
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->TruncateOpenInstance(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "TruncateOpenInstance by brpc failed " << cntl.ErrorText()
                              << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::TruncateOpenInstance failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    CUCKOO_LOG(LOG_INFO) << "TruncateOpenInstance successfully!";
    return 0;
}

// return 0: OK, return negative: error of both network and IO
int CuckooIOClient::TruncateFile(uint64_t physicalFd, off_t size)
{
    cuckoo::brpc_io::TruncateFileRequest request;
    request.set_physical_fd(physicalFd);
    request.set_size(size);
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->TruncateFile(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "TruncateFile by brpc failed " << cntl.ErrorText()
                              << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::TruncateFile failed: " << strerror(-response.error_code());
        return response.error_code();
    }

    CUCKOO_LOG(LOG_INFO) << "TruncateFile successfully!";
    return 0;
}

int CuckooIOClient::CheckConnection()
{
    cuckoo::brpc_io::CheckConnectionRequest request;
    cuckoo::brpc_io::ErrorCodeOnlyReply response;
    brpc::Controller cntl;
    cntl.set_timeout_ms(10000);

    stub->CheckConnection(&cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        CUCKOO_LOG(LOG_ERROR) << "CheckConnection by brpc failed " << cntl.ErrorText()
                              << "error code: " << cntl.ErrorCode();
        return -BrpcErrorCodeToFuseErrno(cntl.ErrorCode());
    }

    if (response.error_code() != 0) {
        CUCKOO_LOG(LOG_ERROR) << "CuckooIOClient::CheckConnection failed: " << strerror(-response.error_code());
        return response.error_code();
    }
    return 0;
}
