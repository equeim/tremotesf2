// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_WINDOWSHELPERS_H
#define TREMOTESF_WINDOWSHELPERS_H

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <QString>

namespace tremotesf {
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

    [[nodiscard]] inline const wchar_t* getCWString(const QString& string) {
        using utf16Type = std::remove_pointer_t<decltype(string.utf16())>;
        static_assert(sizeof(utf16Type) == sizeof(wchar_t));
        return reinterpret_cast<const wchar_t*>(string.utf16());
    }
    inline const wchar_t* getCWString(QString&&) = delete;
}

#endif // TREMOTESF_WINDOWSHELPERS_H
