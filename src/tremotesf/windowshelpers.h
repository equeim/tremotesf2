#ifndef TREMOTESF_WINDOWSHELPERS_H
#define TREMOTESF_WINDOWSHELPERS_H

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <QString>

namespace tremotesf {
    bool isRunningOnWindows10OrGreater();
    bool isRunningOnWindows10_1607OrGreater();
    bool isRunningOnWindows10_1809OrGreater();
    bool isRunningOnWindows10_2004OrGreater();
    bool isRunningOnWindows11OrGreater();

    /**
     * @brief checkWin32Bool
     * @param win32BoolResult BOOL returned from Win32 function
     * @param functionName Name of function that returned BOOL
     * @throws std::system_error
     */
    void checkWin32Bool(int win32BoolResult, std::string_view functionName);

    /**
     * @brief checkHResult
     * @param hresult HRESULT returned from COM function
     * @param functionName Name of function that returned HRESULT
     * @throws winrt::hresult_error
     */
    void checkHResult(int32_t hresult, std::string_view functionName);

    [[nodiscard]]
    inline const wchar_t* getCWString(const QString& string) {
        using utf16Type = std::remove_pointer_t<decltype(string.utf16())>;
        static_assert(sizeof(utf16Type) == sizeof(wchar_t));
        return reinterpret_cast<const wchar_t*>(string.utf16());
    }
    inline const wchar_t* getCWString(QString&&) = delete;
}

#endif // TREMOTESF_WINDOWSHELPERS_H
