/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <iostream>
#include <ostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "glog/logging.h"

#include "cuckoo_definition.h"

enum Logger { STD_LOGGER = 1, GLOGGER = 2, EXTERNAL_LOGGER = 3 };

#define FILENAME_ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#define CUCKOO_LOG_INTERNAL(level) CuckooLog(FILENAME_, __LINE__, level)
#define CUCKOO_LOG_CM(file, line, level) CuckooLog(file, line, level)
#define CUCKOO_LOG(level) CUCKOO_LOG_INTERNAL(CuckooLogLevel::level)

#define RETURN_ON_ERROR(expr)                                         \
    do {                                                              \
        auto exprStatus = (expr);                                     \
        if (exprStatus != CUCKOO_SUCCESS) {                           \
            CUCKOO_LOG(LOG_ERROR) << #expr " failed, " << exprStatus; \
            return exprStatus;                                        \
        }                                                             \
    } while (0)

class StdLog {
  public:
    ~StdLog()
    {
        if (has_logged_) {
            std::cout << std::endl;
        }
    }

    std::ostream &Stream()
    {
        has_logged_ = true;
        return std::cout;
    }

    template <typename T>
    StdLog &operator<<(const T &t)
    {
        Stream() << t;
        return *this;
    }

  private:
    bool has_logged_{false};
};

class ExternalLog {
  public:
    ExternalLog(const char *fileName, int32_t lineNumber, CuckooLogLevel severity)
        : file_name_(fileName),
          line_number_(lineNumber),
          severity_(severity)
    {
    }

    ~ExternalLog()
    {
        if (has_logged_) {
            externalLogger_(severity_, file_name_, line_number_, ("[CUCKOO] " + os_.str()).c_str());
        }
    }

    std::ostream &Stream()
    {
        has_logged_ = true;
        return os_;
    }

    template <typename T>
    ExternalLog &operator<<(const T &t)
    {
        Stream() << t;
        return *this;
    }

    inline static CuckooLogHandler externalLogger_;

  private:
    bool has_logged_{false};
    std::ostringstream os_;
    const char *file_name_;
    int32_t line_number_;
    CuckooLogLevel severity_;
};

using LogProvider = std::variant<StdLog, google::LogMessage, ExternalLog>;
class CuckooLog {
  public:
    CuckooLog() = default;
    CuckooLog(const char *file_name, int32_t line_number, CuckooLogLevel severity);

    bool IsEnabled() const;

    template <typename T>
    CuckooLog &operator<<(const T &t)
    {
        if (IsEnabled()) {
            Stream() << t;
        }
        return *this;
    }

    int32_t InitLog(CuckooLogLevel initSeverityThreshold,
                    Logger logger,
                    const std::string &initLogDir = "",
                    const std::string &name = "",
                    uint32_t logMaxSize = 0,
                    uint32_t initReservedNum = 4,
                    uint32_t initReservedTime = 24);

    static CuckooLogLevel GetCuckooLogLevel();

    static void SetCuckooLogLevel(CuckooLogLevel level);

    static std::string GetLogPrefix(const char *fileName, int lineNumber, CuckooLogLevel severity);

    static void SetExternalLogger(const CuckooLogHandler &logger);

    static CuckooLog *GetInstance();

  protected:
    std::ostream &Stream();

  private:
    void Cleaner(std::stop_token stoken);
    void Clean();
    bool HasPrefix(const std::string &str, const std::string &prefix);
    void DeleteLogFiles(const std::unordered_set<std::string> &excludeFiles);
    struct FileInfo
    {
        std::string filePath;
        time_t mtime;

        FileInfo(const std::string &path, time_t time)
            : filePath(path),
              mtime(time)
        {
        }
    };
    inline static CuckooLogLevel severityThreshold{LOG_INFO};
    inline static Logger defaultLogger{STD_LOGGER};
    inline static std::unordered_map<int, int> glogSeverityMap;
    inline static std::unordered_map<int, std::string> severityPrefixMap{{LOG_TRACE, "[TRACE]"},
                                                                         {LOG_DEBUG, "[DEBUG]"},
                                                                         {LOG_INFO, "[INFO]"},
                                                                         {LOG_WARNING, "[WARNING]"},
                                                                         {LOG_ERROR, "[ERROR]"},
                                                                         {LOG_FATAL, "[FATAL]"}};
    LogProvider logProvider;
    bool isEnabled;
    std::string logDir;
    uint32_t reservedNum;
    uint32_t reservedTime; // unit : h
    std::jthread cleanLogfileThread;
};
