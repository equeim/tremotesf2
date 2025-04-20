// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

#include "demangle.h"

using namespace tremotesf;

struct Foo {};
class Bar {};

namespace foobar {
    struct Foo {};
    class Bar {};
}

template<typename T>
struct What {};

class DemangleTest final : public QObject {
    Q_OBJECT

private slots:
    void checkInt() {
        const int foo{};
        QCOMPARE(typeName(foo), "int");
    }

    void checkStruct() {
        const Foo foo{};
        QCOMPARE(typeName(foo), "Foo");
    }

    void checkClass() {
        const Bar bar{};
        QCOMPARE(typeName(bar), "Bar");
    }

    void checkNamespacedStruct() {
        const foobar::Foo foo{};
        QCOMPARE(typeName(foo), "foobar::Foo");
    }

    void checkNamespacedClass() {
        const foobar::Bar bar{};
        QCOMPARE(typeName(bar), "foobar::Bar");
    }

    void checkTemplatedStruct() {
        const What<int> what{};
        QCOMPARE(typeName(what), "What<int>");
    }

    void checkTemplatedStruct2() {
        const What<Foo> what{};
        QCOMPARE(typeName(what), "What<Foo>");
    }

    void checkTemplatedStruct3() {
        const What<foobar::Foo> what{};
        QCOMPARE(typeName(what), "What<foobar::Foo>");
    }
};

QTEST_GUILESS_MAIN(DemangleTest)

#include "demangle_test.moc"
