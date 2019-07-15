/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_STDUTILS_H
#define TREMOTESF_STDUTILS_H

#include <functional>
#include <memory>

#include <QHash>
#include <QString>

namespace std {
    template<>
    class hash<QString> {
    public:
        size_t operator()(const QString& string) const
        {
            return qHash(string);
        }
    };

    template<>
    class hash<QByteArray> {
    public:
        size_t operator()(const QByteArray& bytes) const
        {
            return qHash(bytes);
        }
    };
}


namespace tremotesf
{
    template<class C, class V>
    inline auto contains_impl(const C& container, const V& value, int) -> decltype(container.find(value), true)
    {
        return container.find(value) != end(container);
    }

    template<class C, class V>
    inline bool contains_impl(const C& container, const V& value, long)
    {
        return std::find(begin(container), end(container), value) != end(container);
    }

    template<class C, class V>
    inline bool contains(const C& container, const V& value)
    {
        return contains_impl(container, value, 0);
    }
    template<class C, class V>
    inline typename C::difference_type index_of(const C& container, const V& value) {
        return std::find(container.cbegin(), container.cend(), value) - container.cbegin();
    }

    template<class C, class V>
    inline void erase_one(C& container, const V& value) {
        container.erase(std::find(container.begin(), container.end(), value));
    }

    template<class T>
    inline bool operator==(const std::shared_ptr<T>& shared, const T* raw) {
        return shared.get() == raw;
    }
}

#endif // TREMOTESF_STDUTILS_H
