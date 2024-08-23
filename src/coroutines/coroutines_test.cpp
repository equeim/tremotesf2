// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <chrono>
#include <qcoreapplication.h>
#include <qtestcase.h>
#include <qtestsupport_core.h>

#include "coroutines/coroutines.h"
#include "coroutines/scope.h"
#include "coroutines/timer.h"
#include "coroutines/waitall.h"

using namespace std::chrono_literals;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

namespace tremotesf {
    class CoroutinesTest final : public QObject {
        Q_OBJECT
    private slots:
        void checkImmediateCoroutine() {
            CoroutineScope scope{};
            bool executed{};
            scope.launch(immediateCoroutine(executed));
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(executed);
        }

        void checkImmediateCoroutineCancellationFromInside() {
            CoroutineScope scope{};
            bool executed{};
            scope.launch(immediateCoroutineCancellingFromInside(executed));
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(!executed);
        }

        void checkImmediateCoroutineWithNested() {
            CoroutineScope scope{};
            int value{};
            scope.launch(immediateCoroutineWithNested(value));
            QCOMPARE(scope.coroutinesCount(), 0);
            QCOMPARE(value, testReturnValue);
        }

        void checkImmediateCoroutineWithNestedCancellationFromInside() {
            CoroutineScope scope{};
            int value{};
            scope.launch(immediateCoroutineWithNestedCancellingFromInside(value));
            QCOMPARE(scope.coroutinesCount(), 0);
            QCOMPARE(value, 0);
        }

        void checkSuspendingCoroutine() {
            CoroutineScope scope{};
            bool executed{};
            scope.launch(suspendingCoroutine(executed));
            QCOMPARE(scope.coroutinesCount(), 1);
            waitTestTime();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(executed);
        }

        void checkSuspendingCoroutineCancellation() {
            CoroutineScope scope{};
            bool executed{};
            scope.launch(suspendingCoroutine(executed));
            QCOMPARE(scope.coroutinesCount(), 1);
            scope.cancelAll();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(!executed);
        }

        void checkSuspendingCoroutineCancellationFromInside() {
            CoroutineScope scope{};
            bool executed{};
            scope.launch(suspendingCoroutine(executed));
            QCOMPARE(scope.coroutinesCount(), 1);
            scope.cancelAll();
            waitTestTime();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(!executed);
        }

        void checkSuspendingWithNested() {
            CoroutineScope scope{};
            int value{};
            scope.launch(suspendingCoroutineWithNested(value));
            QCOMPARE(scope.coroutinesCount(), 1);
            waitTestTime();
            QCOMPARE(scope.coroutinesCount(), 0);
            QCOMPARE(value, testReturnValue);
        }

        void checkSuspendingWithNestedCancellation() {
            CoroutineScope scope{};
            int value{};
            scope.launch(suspendingCoroutineWithNested(value));
            QCOMPARE(scope.coroutinesCount(), 1);
            scope.cancelAll();
            QCOMPARE(scope.coroutinesCount(), 0);
            QCOMPARE(value, 0);
        }

        void checkWaitAllCoroutineImmediate() {
            CoroutineScope scope{};
            WaitAllCoroutineSideEffects sideEffects{};
            scope.launch(waitAllCoroutineImmediate(sideEffects));
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(sideEffects.executed1);
            QVERIFY(sideEffects.executed2);
            QVERIFY(sideEffects.executedAll);
        }

        void checkWaitAllCoroutineImmediateCancellationFromInside() {
            CoroutineScope scope{};
            WaitAllCoroutineSideEffects sideEffects{};
            scope.launch(waitAllCoroutineImmediateCancellingFromInside(sideEffects));
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(sideEffects.executed1);
            QVERIFY(!sideEffects.executed2);
            QVERIFY(!sideEffects.executedAll);
        }

        void checkWaitAllCoroutineSuspending() {
            CoroutineScope scope{};
            WaitAllCoroutineSideEffects sideEffects{};
            scope.launch(waitAllCoroutineSuspending(sideEffects));
            QCOMPARE(scope.coroutinesCount(), 1);
            waitTestTime();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(sideEffects.executed1);
            QVERIFY(sideEffects.executed2);
            QVERIFY(sideEffects.executedAll);
        }

        void checkWaitAllCoroutineSuspendingCancellation() {
            CoroutineScope scope{};
            WaitAllCoroutineSideEffects sideEffects{};
            scope.launch(waitAllCoroutineSuspending(sideEffects));
            QCOMPARE(scope.coroutinesCount(), 1);
            scope.cancelAll();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(!sideEffects.executed1);
            QVERIFY(!sideEffects.executed2);
            QVERIFY(!sideEffects.executedAll);
        }

        void checkWaitAllCoroutineSuspendingCancellationFromInside() {
            CoroutineScope scope{};
            WaitAllCoroutineSideEffects sideEffects{};
            scope.launch(waitAllCoroutineSuspendingCancellingFromInside(sideEffects));
            QCOMPARE(scope.coroutinesCount(), 1);
            waitTestTime();
            QCOMPARE(scope.coroutinesCount(), 0);
            QVERIFY(!sideEffects.executed1);
            QVERIFY(!sideEffects.executed2);
            QVERIFY(!sideEffects.executedAll);
        }

    private:
        static constexpr int testReturnValue = 42;
        static constexpr auto testWaitTime = 40ms;
        static constexpr auto testWaitTimeReduced = 20ms;

        void waitTestTime() {
            // Default QTimer guarantees 5% accuracy
            QTest::qWait(duration_cast<milliseconds>(testWaitTime * 1.05).count() + 1);
            QCoreApplication::processEvents();
        }

        Coroutine<> immediateCoroutine(bool& executed) {
            executed = true;
            co_return;
        }

        Coroutine<> immediateCoroutineCancellingFromInside(bool& executed) {
            cancelCoroutine();
            executed = true;
            co_return;
        }

        Coroutine<int> immediateCoroutineReturningValue() { co_return testReturnValue; }

        Coroutine<> immediateCoroutineWithNested(int& value) { value = co_await immediateCoroutineReturningValue(); }

        Coroutine<int> immediateCoroutineReturningValueCancellingFromInside() {
            cancelCoroutine();
            co_return testReturnValue;
        }

        Coroutine<> immediateCoroutineWithNestedCancellingFromInside(int& value) {
            value = co_await immediateCoroutineReturningValueCancellingFromInside();
        }

        Coroutine<> suspendingCoroutine(bool& executed) {
            co_await waitFor(testWaitTime);
            executed = true;
        }

        Coroutine<int> suspendingReturningValue() {
            co_await waitFor(testWaitTime);
            co_return testReturnValue;
        }

        Coroutine<> suspendingCoroutineWithNested(int& value) { value = co_await suspendingReturningValue(); }

        Coroutine<> suspendingCoroutineCancellingFromInside(bool& executed) {
            co_await waitFor(testWaitTimeReduced);
            cancelCoroutine();
            executed = true;
        }

        struct WaitAllCoroutineSideEffects {
            bool executed1;
            bool executed2;
            bool executedAll;
        };

        Coroutine<> waitAllCoroutineImmediate(WaitAllCoroutineSideEffects& sideEffects) {
            co_await waitAll(immediateCoroutine(sideEffects.executed1), immediateCoroutine(sideEffects.executed2));
            sideEffects.executedAll = true;
        }

        Coroutine<> waitAllCoroutineImmediateCancellingFromInside(WaitAllCoroutineSideEffects& sideEffects) {
            co_await waitAll(
                immediateCoroutine(sideEffects.executed1),
                immediateCoroutineCancellingFromInside(sideEffects.executed2)
            );
            sideEffects.executedAll = true;
        }

        Coroutine<> waitAllCoroutineSuspending(WaitAllCoroutineSideEffects& sideEffects) {
            co_await waitAll(suspendingCoroutine(sideEffects.executed1), suspendingCoroutine(sideEffects.executed2));
            sideEffects.executedAll = true;
        }

        Coroutine<> waitAllCoroutineSuspendingCancellingFromInside(WaitAllCoroutineSideEffects& sideEffects) {
            co_await waitAll(
                suspendingCoroutine(sideEffects.executed1),
                suspendingCoroutineCancellingFromInside(sideEffects.executed2)
            );
            sideEffects.executedAll = true;
        }
    };
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)

QTEST_GUILESS_MAIN(tremotesf::CoroutinesTest)

#include "coroutines_test.moc"
