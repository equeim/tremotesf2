// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <csignal>
#include <string>

#include <version>
#ifdef __cpp_lib_stacktrace
#    include <stacktrace>
#endif

#include <windows.h>

#include <QFileInfo>
#include <QStringBuilder>

#include <fmt/format.h>

#include "windowsfatalerrorhandlers.h"
#include "windowshelpers.h"
#include "log/log.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        bool showedReport{};

#ifdef __cpp_lib_stacktrace
        constexpr size_t maxPathLength = 65535;
        constexpr auto maxPathLengthDword = static_cast<DWORD>(maxPathLength);

#    ifdef _MSC_VER
        QString getExecutablePath() {
            std::wstring executablePath(maxPathLength, L'\0');
            const auto length = GetModuleFileNameW(nullptr, executablePath.data(), maxPathLengthDword);
            if (length == 0 || (length == maxPathLengthDword && GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                return {};
            }
            executablePath.resize(static_cast<size_t>(length));
            return QString::fromStdWString(executablePath);
        }
#    endif

        void appendStackTraceToReport(std::string& report) {
#    ifdef _MSC_VER
            const auto executablePath = getExecutablePath();
            if (executablePath.isEmpty()) {
                report += "\n\nFailed to determine path to executable, not reporting stack trace\n";
                return;
            }
            const QFileInfo executable(executablePath);
            const auto executableDir = executable.path();
            const QString pdbPath = executableDir % '/' % executable.completeBaseName() % ".pdb"_L1;
            if (!QFileInfo::exists(pdbPath)) {
                fmt::format_to(
                    std::back_insert_iterator(report),
                    "\n\nPDB file does not exist at expected path {}, not reporting stack trace\n",
                    pdbPath
                );
                return;
            }
            SetEnvironmentVariableW(L"_NT_SYMBOL_PATH", getCWString(executableDir));
#    endif
            const auto trace = std::stacktrace::current();
            report += "\n\nStack trace:\n";
            report += std::to_string(trace);
        }
#endif

        void showReport(std::string report) {
            showedReport = true;
#ifdef __cpp_lib_stacktrace
            appendStackTraceToReport(report);
#endif
            warning().log(report);
            showFatalErrorReportInDialog(std::move(report));
        }

        void onTerminate() {
            std::string report = "FATAL ERROR: std::terminate called";
            if (const auto exception_ptr = std::current_exception(); exception_ptr) {
                report += "\n\nUnhandled C++ exception:\n";
                try {
                    std::rethrow_exception(exception_ptr);
                } catch (const std::exception& e) {
                    fmt::format_to(
                        std::back_insert_iterator(report),
                        impl::singleArgumentFormatString,
                        formatExceptionRecursively(e)
                    );
                } catch (const winrt::hresult_error& e) {
                    fmt::format_to(
                        std::back_insert_iterator(report),
                        impl::singleArgumentFormatString,
                        formatExceptionRecursively(e)
                    );
                } catch (...) {
                    report += "Type of exception is unknown";
                }
            }
            showReport(std::move(report));
            std::abort();
        }

        std::string_view sehExceptionCodeName(DWORD exceptionCode) {
            switch (exceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                return "EXCEPTION_ACCESS_VIOLATION";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_BREAKPOINT:
                return "EXCEPTION_BREAKPOINT";
            case EXCEPTION_DATATYPE_MISALIGNMENT:
                return "EXCEPTION_DATATYPE_MISALIGNMENT";
            case EXCEPTION_FLT_DENORMAL_OPERAND:
                return "EXCEPTION_FLT_DENORMAL_OPERAND";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_INEXACT_RESULT:
                return "EXCEPTION_FLT_INEXACT_RESULT";
            case EXCEPTION_FLT_INVALID_OPERATION:
                return "EXCEPTION_FLT_INVALID_OPERATION";
            case EXCEPTION_FLT_OVERFLOW:
                return "EXCEPTION_FLT_OVERFLOW";
            case EXCEPTION_FLT_STACK_CHECK:
                return "EXCEPTION_FLT_STACK_CHECK";
            case EXCEPTION_FLT_UNDERFLOW:
                return "EXCEPTION_FLT_UNDERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_IN_PAGE_ERROR:
                return "EXCEPTION_IN_PAGE_ERROR";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                return "EXCEPTION_INT_DIVIDE_BY_ZERO";
            case EXCEPTION_INT_OVERFLOW:
                return "EXCEPTION_INT_OVERFLOW";
            case EXCEPTION_INVALID_DISPOSITION:
                return "EXCEPTION_INVALID_DISPOSITION";
            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION:
                return "EXCEPTION_PRIV_INSTRUCTION";
            case EXCEPTION_SINGLE_STEP:
                return "EXCEPTION_SINGLE_STEP";
            case EXCEPTION_STACK_OVERFLOW:
                return "EXCEPTION_STACK_OVERFLOW";
            default:
                break;
            }
            return "UNKNOWN EXCEPTION";
        }

        constexpr DWORD cppExceptionCode = 0xe06d7363;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        LPTOP_LEVEL_EXCEPTION_FILTER defaultSehExceptionFilter{};

        LONG WINAPI sehExceptionFilter(LPEXCEPTION_POINTERS exceptionInfo) {
            if (exceptionInfo->ExceptionRecord->ExceptionCode == cppExceptionCode) {
                // C++ exception, delegate to onTerminate
                if (defaultSehExceptionFilter) {
                    defaultSehExceptionFilter(exceptionInfo);
                }
                return EXCEPTION_CONTINUE_SEARCH;
            }

            std::string report{};
            fmt::format_to(
                std::back_insert_iterator(report),
                "FATAL ERROR: Unhandled SEH exception 0x{:x} {}",
                exceptionInfo->ExceptionRecord->ExceptionCode,
                sehExceptionCodeName(exceptionInfo->ExceptionRecord->ExceptionCode)
            );
            {
                PEXCEPTION_RECORD nested = exceptionInfo->ExceptionRecord->ExceptionRecord;
                while (nested) {
                    fmt::format_to(
                        std::back_insert_iterator(report),
                        "\nCaused by: 0x{:x} {}",
                        nested->ExceptionCode,
                        sehExceptionCodeName(nested->ExceptionCode)
                    );
                    nested = nested->ExceptionRecord;
                }
            }
            showReport(std::move(report));
            return EXCEPTION_CONTINUE_SEARCH;
        }

        void abortHandler(int) {
            if (!showedReport) {
                showReport("FATAL ERROR: std::abort called");
            }
        }
    }

    void windowsSetUpFatalErrorHandlers() {
        /**
         * std::set_terminate is thread-local
         * SetUnhandledExceptionFilter and std::signal are global
         */
        std::set_terminate(&onTerminate);
        defaultSehExceptionFilter = SetUnhandledExceptionFilter(&sehExceptionFilter);
        _set_abort_behavior(0, _WRITE_ABORT_MSG);
        std::signal(SIGABRT, &abortHandler);
    }

    void windowsSetUpFatalErrorHandlersInThread() {
        static thread_local bool set = false;
        if (!set) {
            std::set_terminate(&onTerminate);
            set = true;
        }
    }

    std::string makeFatalErrorReportFromLogMessage(const QString& message, const QMessageLogContext& context) {
        std::string report = fmt::format(
            "FATAL ERROR: {}\nFunction: {}\nSource file: {}:{}",
            message,
            context.function,
            context.file,
            context.line
        );
#ifdef __cpp_lib_stacktrace
        appendStackTraceToReport(report);
#endif
        return report;
    }

    void showFatalErrorReportInDialog(std::string report) {
        showedReport = true;
        report.insert(0, "Press Ctrl+C to copy this report\n\n");
        MessageBoxA(nullptr, report.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
    }
}
