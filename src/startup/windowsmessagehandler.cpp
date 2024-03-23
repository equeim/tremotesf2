// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowsmessagehandler.h"

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QString>

#include <windows.h>

#include "fileutils.h"
#include "literals.h"
#include "log/log.h"
#include "windowshelpers.h"

namespace fs = std::filesystem;

namespace tremotesf {
    namespace {
        class [[maybe_unused]] MessageQueue final {
        public:
            void pushEvicting(QString&& message) {
                {
                    const std::lock_guard lock(mMutex);
                    if (mNewMessagesCancelled) return;
                    if (mQueue.size() == maximumSize) {
                        mQueue.pop_front();
                    }
                    mQueue.push_back(std::move(message));
                }
                mCv.notify_one();
            }

            std::optional<QString> popBlocking() {
                std::unique_lock lock(mMutex);
                mCv.wait(lock, [&] { return !mQueue.empty() || mNewMessagesCancelled; });
                if (mQueue.empty()) return {};
                QString message = std::move(mQueue.front());
                mQueue.pop_front();
                return message;
            }

            void cancelNewMessages() {
                {
                    const std::lock_guard lock(mMutex);
                    mNewMessagesCancelled = true;
                }
                mCv.notify_one();
            }

        private:
            std::deque<QString> mQueue{};
            std::mutex mMutex{};
            std::condition_variable mCv{};
            bool mNewMessagesCancelled{};

            static constexpr size_t maximumSize = 10000;
        };

        class [[maybe_unused]] FileLogger final {
        public:
            void logMessage(QString&& message) { mQueue.pushEvicting(std::move(message)); }

            // We are not doing this in destructor because we need
            // thread to be able to call logMessage() while we are joining it
            void finishWriting() {
                info().log("FileLogger: finishing logging");
                debug().log("FileLogger: wait until thread started writing or finished with error");
                {
                    std::unique_lock lock(mMutex);
                    mCv.wait(lock, [&] { return mStartedWriting || mFinishedWriting; });
                }
                debug().log("FileLogger: cancelling new messages");
                mQueue.cancelNewMessages();
                debug().log("FileLogger: joining write thread");
                mWriteThread.join();
                debug().log("FileLogger: joined write thread");
            }

        private:
            void writeMessagesToFile() {
                debug().log("FileLogger: started write thread");

                auto finishGuard = QScopeGuard([this] {
                    debug().log("FileLogger: finished write thread");
                    mQueue.cancelNewMessages();
                    {
                        const std::lock_guard lock(mMutex);
                        mFinishedWriting = true;
                    }
                    mCv.notify_one();
                });

                const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
                debug().log("FileLogger: creating logs directory {}", QDir::toNativeSeparators(dirPath));
                try {
                    fs::create_directories(fs::path(getCWString(dirPath)));
                } catch (const fs::filesystem_error& e) {
                    warning().logWithException(e, "FileLogger: failed to create logs directory");
                    return;
                }
                debug().log("FileLogger: created logs directory");

                auto filePath = QString::fromStdString(
                    fmt::format("{}/{}.log", dirPath, QDateTime::currentDateTime().toString(u"yyyy-MM-dd_hh-mm-ss.zzz"))
                );
                debug().log("FileLogger: creating log file {}", QDir::toNativeSeparators(filePath));
                QFile file(filePath);
                try {
                    openFile(file, QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text | QIODevice::Unbuffered);
                } catch (const QFileError& e) {
                    warning().logWithException(e, "FileLogger: failed to create log file");
                    return;
                }
                debug().log("FileLogger: created log file");

                {
                    const std::lock_guard lock(mMutex);
                    mStartedWriting = true;
                }
                mCv.notify_one();

                while (true) {
                    const auto message = mQueue.popBlocking();
                    if (!message.has_value()) {
                        return;
                    }
                    writeMessageToFile(*message, file);
                }
            }

            void writeMessageToFile(const QString& message, QFile& file) {
                try {
                    writeBytes(file, message.toUtf8());
                    static constexpr std::array<char, 1> lineTerminator{'\n'};
                    writeBytes(file, lineTerminator);
                } catch ([[maybe_unused]] const QFileError& e) {}
            }

            MessageQueue mQueue{};

            std::mutex mMutex{};
            std::condition_variable mCv{};
            bool mStartedWriting{};
            bool mFinishedWriting{};

            std::thread mWriteThread{&FileLogger::writeMessagesToFile, this};
        };

        void writeToDebugger(const wchar_t* message) {
            if (IsDebuggerPresent()) {
                OutputDebugStringW(message);
                OutputDebugStringW(L"\r\n");
            }
        }

        std::unique_ptr<FileLogger> globalFileLogger{};

        [[maybe_unused]] void releaseMessageHandler(QString&& message) {
            writeToDebugger(getCWString(message));
            if (globalFileLogger) {
                globalFileLogger->logMessage(std::move(message));
            }
        }

        [[maybe_unused]] void debugMessageHandler(QString&& message) {
            const auto wstr = getCWString(message);
            writeToDebugger(wstr);
            static const auto stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
            static const bool stderrIsConsole = [] {
                DWORD mode{};
                return GetConsoleMode(stderrHandle, &mode) != FALSE;
            }();
            if (stderrIsConsole) {
                WriteConsoleW(stderrHandle, wstr, static_cast<DWORD>(message.size()), nullptr, nullptr);
                WriteConsoleW(stderrHandle, L"\n", 1, nullptr, nullptr);
            } else {
                // stderr is redirected to pipe or a file, write UTF-8
                const auto utf8 = message.toUtf8();
                fwrite(utf8.data(), 1, static_cast<size_t>(utf8.size()), stderr);
                fputc('\n', stderr);
            }
        }

        void windowsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
            QString formatted = qFormatLogMessage(type, context, message);
#ifdef NDEBUG
            releaseMessageHandler(std::move(formatted));
#else
            debugMessageHandler(std::move(formatted));
#endif
        }
    }

    void initWindowsMessageHandler() {
        qInstallMessageHandler(windowsMessageHandler);
        qSetMessagePattern(
            "[%{time yyyy.MM.dd h:mm:ss.zzz t} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{message}"_l1
        );
#ifdef NDEBUG
        globalFileLogger = std::make_unique<FileLogger>();
        debug().log("FileLogger: created, starting write thread");
#endif
    }

    void deinitWindowsMessageHandler() {
#ifdef NDEBUG
        if (globalFileLogger) {
            globalFileLogger->finishWriting();
        }
#endif
    }
}
