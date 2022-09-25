#include "libtremotesf/log.h"

#include "windowsmessagehandler.h"

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QString>

#include <guiddef.h>
#include <winrt/base.h>

#include <windows.h>

#include "tremotesf/windowshelpers.h"

namespace fs = std::filesystem;

namespace tremotesf {
    namespace {
        class [[maybe_unused]] MessageQueue final {
        public:
            void pushEvicting(QString&& message) {
                {
                    std::lock_guard lock(mMutex);
                    if (mCancelled) return;
                    if (mQueue.size() == maximumSize) {
                        mQueue.pop_front();
                    }
                    mQueue.push_back(std::move(message));
                }
                mCv.notify_one();
            }

            std::optional<QString> popBlocking() {
                std::unique_lock lock(mMutex);
                mCv.wait(lock, [&] { return !mQueue.empty() || mCancelled; });
                if (mCancelled) return {};
                QString message = std::move(mQueue.front());
                mQueue.pop_front();
                return message;
            }

            void cancel() {
                {
                    std::lock_guard lock(mMutex);
                    mCancelled = true;
                    mQueue.clear();
                }
                mCv.notify_one();
            }

        private:
            std::deque<QString> mQueue{};
            std::mutex mMutex{};
            std::condition_variable mCv{};
            bool mCancelled{};

            static constexpr size_t maximumSize = 10000;
        };

        class [[maybe_unused]] FileLogger final {
        public:
            ~FileLogger() {
                mQueue.cancel();
                mThread.join();
            }

            void logMessage(QString&& message) {
                mQueue.pushEvicting(std::move(message));
            }

        private:
            void writeMessagesToFile() {
                const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
                try {
                    fs::create_directories(fs::path(getCWString(dirPath)));
                } catch ([[maybe_unused]] const fs::filesystem_error& e) {
                    mQueue.cancel();
                    return;
                }
                auto filePath = QString::fromStdString(fmt::format(
                    "{}/{}.log",
                    dirPath,
                    QDateTime::currentDateTime().toString(u"yyyy-MM-dd_hh-mm-ss.zzz")
                ));
                QFile file(filePath);
                if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text | QIODevice::Unbuffered)) {
                    mQueue.cancel();
                    return;
                }

                while (true) {
                    const auto message = mQueue.popBlocking();
                    if (!message.has_value()) {
                        return;
                    }
                    writeMessageToFile(*message, file);
                }
            }

            void writeMessageToFile(const QString& message, QFile& file) {
                file.write(message.toUtf8());
                file.putChar('\n');
            }

            MessageQueue mQueue{};
            std::thread mThread{&FileLogger::writeMessagesToFile, this};
        };

        void writeToDebugger(const wchar_t* message) {
            if (IsDebuggerPresent()) {
                OutputDebugStringW(message);
                OutputDebugStringW(L"\r\n");
            }
        }

        [[maybe_unused]]
        void releaseMessageHandler(QString&& message) {
            writeToDebugger(getCWString(message));
            static FileLogger logger{};
            logger.logMessage(std::move(message));
        }

        [[maybe_unused]]
        void debugMessageHandler(QString&& message) {
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
    }

    void windowsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
        [[maybe_unused]]
        static const bool set = [] {
            qSetMessagePattern(QLatin1String("[%{time yyyy.MM.dd h:mm:ss.zzz t} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{message}"));
            return true;
        }();
        QString formatted = qFormatLogMessage(type, context, message);
#ifdef NDEBUG
        releaseMessageHandler(std::move(formatted));
#else
        debugMessageHandler(std::move(formatted));
#endif
    }
}
