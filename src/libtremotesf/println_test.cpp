#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QVariant>

#include "println.h"
#include "torrent.h"

using namespace libtremotesf;

class PrintlnTest : public QObject {
    Q_OBJECT
private slots:
    void stdoutStringLiteral() {
        printlnStdout("foo");
        printlnStdout("{}", "foo");
        printlnStdout(FMT_STRING("{}"), "foo");
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), "foo");
        printlnStdout(fmt::runtime("{}"), "foo");
#endif
    }

    void stdoutStdString() {
        const std::string str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutStdStringView() {
        const std::string_view str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQString() {
        const QString str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQStringView() {
        const QString _str = "foo";
        const QStringView str = _str;
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQLatin1String() {
        const QLatin1String str("foo");
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

#if QT_VERSION_MAJOR >= 6
    void stdoutQUtf8StringView() {
        const QUtf8StringView str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQAnyStringView() {
        const QAnyStringView str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), str);
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }
#endif

    void stdoutQVariant() {
        const QVariant value = "foo";
        printlnStdout(value);
        printlnStdout("{}", value);
        printlnStdout(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), value);
        printlnStdout(fmt::runtime("{}"), value);
#endif
    }

    void stdoutQStringList() {
        const QStringList list{"foo"};
        printlnStdout(list);
        printlnStdout("{}", list);
        printlnStdout(FMT_STRING("{}"), list);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), list);
        printlnStdout(fmt::runtime("{}"), list);
#endif
    }

    void stdoutTorrent() {
        const Torrent value(0, {}, nullptr);
        printlnStdout(value);
        printlnStdout("{}", value);
        printlnStdout(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        printlnStdout(FMT_COMPILE("{}"), value);
        printlnStdout(fmt::runtime("{}"), value);
#endif
    }


    void infoStringLiteral() {
        printlnInfo("foo");
        printlnInfo("{}", "foo");
        printlnInfo(FMT_STRING("{}"), "foo");
        printlnInfo(FMT_COMPILE("{}"), "foo");
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), "foo");
#endif
    }

    void infoStdString() {
        const std::string str = "foo";
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoStdStringView() {
        const std::string_view str = "foo";
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQString() {
        const QString str = "foo";
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQStringView() {
        const QString _str = "foo";
        const QStringView str = _str;
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQLatin1String() {
        const QLatin1String str("foo");
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

#if QT_VERSION_MAJOR >= 6
    void infoQUtf8StringView() {
        const QUtf8StringView str = "foo";
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQAnyStringView() {
        const QAnyStringView str = "foo";
        printlnInfo(str);
        printlnInfo("{}", str);
        printlnInfo(FMT_STRING("{}"), str);
        printlnInfo(FMT_COMPILE("{}"), str);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), str);
#endif
    }
#endif

    void infoQVariant() {
        const QVariant value = "foo";
        printlnInfo(value);
        printlnInfo("{}", value);
        printlnInfo(FMT_STRING("{}"), value);
        printlnInfo(FMT_COMPILE("{}"), value);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), value);
#endif
    }

    void infoQStringList() {
        const QStringList list{"foo"};
        printlnInfo(list);
        printlnInfo("{}", list);
        printlnInfo(FMT_STRING("{}"), list);
        printlnInfo(FMT_COMPILE("{}"), list);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), list);
#endif
    }

    void infoTorrent() {
        const Torrent value(0, {}, nullptr);
        printlnInfo(value);
        printlnInfo("{}", value);
        printlnInfo(FMT_STRING("{}"), value);
        printlnInfo(FMT_COMPILE("{}"), value);
#if FMT_VERSION >= 80000
        printlnInfo(fmt::runtime("{}"), value);
#endif
    }
};

QTEST_MAIN(PrintlnTest)

#include "println_test.moc"
