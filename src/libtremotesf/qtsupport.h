/*
 * Tremotesf
 * Copyright (C) 2015-2021 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_QTSUPPORT_H
#define TREMOTESF_QTSUPPORT_H

#include <QHashFunctions>
#include <QString>

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


#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    using QJsonKeyString = QString;
#define QJsonKeyStringInit QStringLiteral
#else
    using QJsonKeyString = QLatin1String;
    using QJsonKeyStringInit = QLatin1String;
#endif


#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
#define DEFINE_Q_ENUM_NS(Name, ...) \
    Q_NAMESPACE \
    enum class Name { __VA_ARGS__ }; \
    Q_ENUM_NS(Name)
#define Q_ENUM_NS_TYPE(Name) Name
#else
#define DEFINE_Q_ENUM_NS(Name, ...) \
    struct Name ## Struct { \
        Q_GADGET \
    public: \
        Name ## Struct() = delete; \
        enum Enum { __VA_ARGS__ }; \
        Q_ENUM(Enum) \
    }; \
    using Name = Name ## Struct::Enum;
#define Q_ENUM_NS_TYPE(Name) Name ## Struct::Enum
#endif


#endif // TREMOTESF_QTSUPPORT_H
