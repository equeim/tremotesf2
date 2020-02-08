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
#include <iterator>
#include <type_traits>
#include <vector>

#include <QtGlobal>
#include <QHashFunctions>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
/*
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
*/
#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, Arguments)      \
    QT_BEGIN_INCLUDE_NAMESPACE                                      \
    namespace std {                                                 \
        template <>                                                 \
        struct hash< QT_PREPEND_NAMESPACE(Class) > {                \
            using argument_type = QT_PREPEND_NAMESPACE(Class);      \
            using result_type = size_t;                             \
            size_t operator()(Arguments s) const                    \
                noexcept(noexcept(QT_PREPEND_NAMESPACE(qHash)(s)))  \
            {                                                       \
                /* this seeds qHash with the result of */           \
                /* std::hash applied to an int, to reap */          \
                /* any protection against predictable hash */       \
                /* values the std implementation may provide */     \
                return QT_PREPEND_NAMESPACE(qHash)(s,               \
                           QT_PREPEND_NAMESPACE(qHash)(             \
                                      std::hash<int>{}(0)));        \
            }                                                       \
        };                                                          \
    }                                                               \
    QT_END_INCLUDE_NAMESPACE                                        \
    /*end*/

#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(Class) \
    QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH(Class, const argument_type &)

QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QString)
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QByteArray)
#endif

namespace libtremotesf
{
    template<class C, class V>
    inline auto contains_impl(const C& container, const V& value, int) -> decltype(container.find(value), true)
    {
        return container.find(value) != std::end(container);
    }

    template<class C, class V>
    inline bool contains_impl(const C& container, const V& value, long)
    {
        return std::find(std::begin(container), std::end(container), value) != std::end(container);
    }

    template<class C, class V>
    inline bool contains(const C& container, const V& value)
    {
        return contains_impl(container, value, 0);
    }
    template<class C, class V>
    inline typename C::size_type index_of(const C& container, const V& value) {
        return static_cast<typename C::size_type>(std::find(container.cbegin(), container.cend(), value) - container.cbegin());
    }

    template<class C, class V>
    inline void erase_one(C& container, const V& value) {
        container.erase(std::find(container.begin(), container.end(), value));
    }


    template<typename T, typename std::enable_if<std::is_scalar<T>::value && !std::is_floating_point<T>::value, int>::type = 0>
    inline void setChanged(T& value, T newValue, bool& changed)
    {
        if (newValue != value) {
            value = newValue;
            changed = true;
        }
    }

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline void setChanged(T& value, T newValue, bool& changed)
    {
        if (!qFuzzyCompare(newValue, value)) {
            value = newValue;
            changed = true;
        }
    }

    template<typename T, typename std::enable_if<!std::is_scalar<T>::value, int>::type = 0>
    inline void setChanged(T& value, T&& newValue, bool& changed)
    {
        if (newValue != value) {
            value = std::forward<T>(newValue);
            changed = true;
        }
    }

    template<typename T>
    struct VectorBatchRemover
    {
        std::vector<T>& items;

        std::vector<int>& removedIndexes;
        std::vector<int>& indexesAfterRemoved;

        const typename std::vector<T>::iterator begin = items.begin();

        int endIndex = -1;
        int beginIndex = 0;

        void remove(int index) {
            removedIndexes.push_back(index);
            if (endIndex == -1) {
                reset(index);
            } else {
                if (index == (beginIndex - 1)) {
                    beginIndex = index;
                } else {
                    remove();
                    reset(index);
                }
            }
        }

        void remove() {
            items.erase(begin + beginIndex, begin + endIndex + 1);
            if (!indexesAfterRemoved.empty()) {
                const int shift = static_cast<int>(endIndex - beginIndex + 1);
                for (int& index : indexesAfterRemoved) {
                    index -= shift;
                }
            }
        }

        void reset(int index) {
            endIndex = index;
            beginIndex = index;
        }
    };
}

namespace tremotesf
{
    using libtremotesf::contains;
    using libtremotesf::index_of;
    using libtremotesf::erase_one;
    using libtremotesf::setChanged;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
        using QJsonKeyString = QString;
#define QJsonKeyStringInit QStringLiteral
#else
        using QJsonKeyString = QLatin1String;
        using QJsonKeyStringInit = QLatin1String;
#endif

#endif // TREMOTESF_STDUTILS_H
