#include "logger.h"
#include "core/run.h"
#include "core/error.h"
#include <filesystem>
#include <iomanip>

#define _CRT_SECURE_NO_WARNINGS 1

// logger implementation

logger::logger(runtime_config & cfg) {
    if (cfg.doNotLog) {
        return;
    }

    time_t now = time(nullptr);
    char buf[20];

    size_t written = std::strftime(
            buf, 20, "%X", std::localtime(&now));

    for (char& c: buf) {
        switch (c) {
            case ':':
                c = '-';
                break;
            case ' ':
                c = '_';
            default:
                break;
        }
    }

    std::string filename = cfg.tag.empty()
            ? cfg.toTest.filename() : cfg.tag;

    filename += '_' + std::string(buf, written) + ".txt";
    std::filesystem::path stressDir = "stress";
    auto logsDir = stressDir / "logs";
    path = logsDir / filename;

    std::error_code errCode;

    if (exists(stressDir) && !is_directory(stressDir, errCode)) {
        throw error("Path ./stress exists, but it is not a folder");

    } else if (!exists(stressDir) && !create_directory(stressDir, errCode)) {
        throw error("Path ./stress couldn't be created");

    } else if (!exists(logsDir) && !create_directory(logsDir, errCode)) {
        throw error("Path ./stress/logs couldn't be created");
    }
}

logger::STATUS logger::writeTest(const runtime_config &cfg, const test_result &result, uint32_t testId, bool flush) {
    if (path.empty()) {
        return STATUS::NOT_NEEDED;

    } else if (bad) {
        return STATUS::BAD;

    } else if (!logFile.is_open()) {
        logFile.open(path, std::ios::binary);
        if (logFile.fail()) {
            bad = true;
            return STATUS::OPEN_ERROR;
        }
    }

    std::stringstream stream; // todo: newlines?

    stream << "------- TEST " << testId << " -------" << std::endl;
    stream << "verdict: " << result.verdict.toShortString();
    stream << ", " << result.execResult.time << " ms, ";
    stream << std::setprecision(1) << std::fixed;
    stream << (result.execResult.memory/1014.l/1024) << " MB" << std::endl;

    if (result.execResult.error.hasError()) {
        stream << result.execResult.error.errorExplanation() << std::endl;

        if (cfg.logStderrOnRE) {
            stream << "stderr dump: ";
            if (result.err.empty()) {
                stream << "<empty>" << std::endl;
            } else {
                stream << ">>>" << std::endl;
                stream << result.err;
                stream << std::endl << "<<<" << std::endl;
            }
        }
    }

    stream << std::endl << result.input << std::endl << std::endl;
    logFile << stream.str();

    if (logFile.fail()) {
        bad = true;
        return STATUS::LOG_ERROR;
    }
    if (flush) {
        logFile.flush();
    }
    return STATUS::OK;
}

std::string logger::statusExplanation(STATUS status) {
    switch (status) {
        case STATUS::OPEN_ERROR:
            return "[!] Unable to create log file, logger disabled";
        case STATUS::LOG_ERROR:
            return "[!] Unable to store a test result, logger disabled";
        default:
            return "";
    }
}

std::string logger::info() {
    if (path.empty()) {
        return "[*] Logger disabled";
    } else if (bad && logFile.is_open()) {
        return "[*] Partially logged in " + path.filename().string();
    } else if (bad && !logFile.is_open()) {
        return "[*] Failed to create log file";
    } else if (!bad && logFile.is_open()) {
        return "[*] Check " + path.filename().string();
    } else { // !bad && !logFile.is_open()
        return "[*] Nothing was logged";
    }
}
