// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LGPL-3.0-only
#include "recoloringsvgiconengine.h"

// clang-format off

#include "qpainter.h"
#include "qpixmap.h"
#include "qsvgrenderer.h"
#include "qpixmapcache.h"
#include "qfileinfo.h"
#if QT_CONFIG(mimetype)
#include <qmimedatabase.h>
#include <qmimetype.h>
#endif
#include <QAtomicInt>
#include "qdebug.h"
#include <private/qguiapplication_p.h>
#include <private/qhexstring_p.h>

#include <QApplication>
#include <QBuffer>
#include <QPalette>

namespace tremotesf {

namespace {

QString STYLESHEET_TEMPLATE()
{
    return QStringLiteral(".ColorScheme-Text {\
color:%1;\
}\
.ColorScheme-Background{\
color:%2;\
}\
.ColorScheme-Highlight{\
color:%3;\
}\
.ColorScheme-HighlightedText{\
color:%4;\
}\
.ColorScheme-PositiveText{\
color:%5;\
}\
.ColorScheme-NeutralText{\
color:%6;\
}\
.ColorScheme-NegativeText{\
color:%7;\
}");
}

enum FileType { OtherFile, SvgFile, CompressedSvgFile };

static FileType fileType(const QFileInfo &fi)
{
    const QString &suffix = fi.completeSuffix();
    if (suffix.endsWith(QLatin1String("svg"), Qt::CaseInsensitive))
        return SvgFile;
    if (suffix.endsWith(QLatin1String("svgz"), Qt::CaseInsensitive)
        || suffix.endsWith(QLatin1String("svg.gz"), Qt::CaseInsensitive)) {
        return CompressedSvgFile;
        }
        #if QT_CONFIG(mimetype)
        const QString &mimeTypeName = QMimeDatabase().mimeTypeForFile(fi).name();
    if (mimeTypeName == QLatin1String("image/svg+xml"))
        return SvgFile;
    if (mimeTypeName == QLatin1String("image/svg+xml-compressed"))
        return CompressedSvgFile;
    #endif
    return OtherFile;
}

}

class RecoloringSvgIconEnginePrivate : public QSharedData
{
public:
    RecoloringSvgIconEnginePrivate()
    {
        stepSerialNum();
    }

    static int hashKey(QIcon::Mode mode, QIcon::State state)
    {
        return ((mode << 4) | state);
    }

    QString pmcKey(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale, const QPalette& pal) const
    {
        return QLatin1String("$qt_svgicon_")
                % HexString<int>(serialNum)
                % HexString<qint8>(mode)
                % HexString<qint8>(state)
                % HexString<int>(size.width())
                % HexString<int>(size.height())
                % HexString<qint16>(qRound(scale * 1000))
                % QLatin1Char('_')
                % pal.windowText().color().name()
                % QLatin1Char('_')
                % pal.highlight().color().name()
                % QLatin1Char('_')
                % pal.highlightedText().color().name();
    }

    void stepSerialNum()
    {
        serialNum = lastSerialNum.fetchAndAddRelaxed(1);
    }

    bool tryLoad(QSvgRenderer *renderer, QIcon::Mode tryMode, QIcon::State tryState, QIcon::Mode actualMode);
    QIcon::Mode loadDataForModeAndState(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state);

    QHash<int, QString> svgFiles;
    QHash<int, QByteArray> svgBuffers;
    QMultiHash<int, QPixmap> addedPixmaps;
    int serialNum = 0;
    static QAtomicInt lastSerialNum;
};

QAtomicInt RecoloringSvgIconEnginePrivate::lastSerialNum;

RecoloringSvgIconEngine::RecoloringSvgIconEngine()
    : d(new RecoloringSvgIconEnginePrivate)
{
}

RecoloringSvgIconEngine::RecoloringSvgIconEngine(const RecoloringSvgIconEngine &other)
    : QIconEngine(other), d(new RecoloringSvgIconEnginePrivate)
{
    d->svgFiles = other.d->svgFiles;
    d->svgBuffers = other.d->svgBuffers;
    d->addedPixmaps = other.d->addedPixmaps;
}


RecoloringSvgIconEngine::~RecoloringSvgIconEngine()
{
}


QSize RecoloringSvgIconEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    if (!d->addedPixmaps.isEmpty()) {
        const auto key = d->hashKey(mode, state);
        auto it = d->addedPixmaps.constFind(key);
        while (it != d->addedPixmaps.end() && it.key() == key) {
            const auto &pm = it.value();
            if (!pm.isNull() && pm.size() == size)
                return size;
            ++it;
        }
    }

    QPixmap pm = pixmap(size, mode, state);
    if (pm.isNull())
        return QSize();
    return pm.size();
}

static inline QByteArray maybeUncompress(const QByteArray &ba)
{
#ifndef QT_NO_COMPRESS
    return qUncompress(ba);
#else
    return ba;
#endif
}

bool RecoloringSvgIconEnginePrivate::tryLoad(QSvgRenderer *renderer, QIcon::Mode tryMode, QIcon::State tryState, QIcon::Mode actualMode)
{
    const auto key = hashKey(tryMode, tryState);
    QByteArray buf = svgBuffers.value(key);
    if (!buf.isEmpty()) {
        if (renderer->load(maybeUncompress(buf)))
            return true;
        svgBuffers.remove(key);
    }
    QString svgFile = svgFiles.value(key);
    if (!svgFile.isEmpty()) {
        if (fileType(QFileInfo(svgFile)) == CompressedSvgFile) {
            qWarning() << "Can't recolor compressed svg" << svgFile;
            return renderer->load(svgFile);
        }
        const auto pal = QApplication::palette("QMenu");
        const QString styleSheet = STYLESHEET_TEMPLATE().arg(
            actualMode == QIcon::Selected ? pal.highlightedText().color().name() : pal.windowText().color().name(),
            actualMode == QIcon::Selected ? pal.highlight().color().name() : pal.window().color().name(),
            actualMode == QIcon::Selected ? pal.highlightedText().color().name() : pal.highlight().color().name(),
            actualMode == QIcon::Selected ? pal.highlight().color().name() : pal.highlightedText().color().name(),
            actualMode == QIcon::Selected ? pal.highlightedText().color().name() : pal.windowText().color().name(),
            actualMode == QIcon::Selected ? pal.highlightedText().color().name() : pal.windowText().color().name(),
            actualMode == QIcon::Selected ? pal.highlightedText().color().name() : pal.windowText().color().name());
        QFile file(svgFile);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        QByteArray processedContents;
        QXmlStreamReader reader(&file);

        QBuffer buffer(&processedContents);
        buffer.open(QIODevice::WriteOnly);
        QXmlStreamWriter writer(&buffer);
        while (!reader.atEnd()) {
            if (reader.readNext() == QXmlStreamReader::StartElement //
                && reader.qualifiedName() == QLatin1String("style") //
                && reader.attributes().value(QLatin1String("id")) == QLatin1String("current-color-scheme")) {
                writer.writeStartElement(QStringLiteral("style"));
                writer.writeAttributes(reader.attributes());
                writer.writeCharacters(styleSheet);
                writer.writeEndElement();
                while (reader.tokenType() != QXmlStreamReader::EndElement) {
                    reader.readNext();
                }
            } else if (reader.tokenType() != QXmlStreamReader::Invalid) {
                writer.writeCurrentToken(reader);
            }
        }

        return renderer->load(processedContents);
    }
    return false;
}

QIcon::Mode RecoloringSvgIconEnginePrivate::loadDataForModeAndState(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state)
{
    if (tryLoad(renderer, mode, state, mode))
        return mode;

    const QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
    if (mode == QIcon::Disabled || mode == QIcon::Selected) {
        const QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
        if (tryLoad(renderer, QIcon::Normal, state, mode))
            return QIcon::Normal;
        if (tryLoad(renderer, QIcon::Active, state, mode))
            return QIcon::Active;
        if (tryLoad(renderer, mode, oppositeState, mode))
            return mode;
        if (tryLoad(renderer, QIcon::Normal, oppositeState, mode))
            return QIcon::Normal;
        if (tryLoad(renderer, QIcon::Active, oppositeState, mode))
            return QIcon::Active;
        if (tryLoad(renderer, oppositeMode, state, mode))
            return oppositeMode;
        if (tryLoad(renderer, oppositeMode, oppositeState, mode))
            return oppositeMode;
    } else {
        const QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
        if (tryLoad(renderer, oppositeMode, state, mode))
            return oppositeMode;
        if (tryLoad(renderer, mode, oppositeState, mode))
            return mode;
        if (tryLoad(renderer, oppositeMode, oppositeState, mode))
            return oppositeMode;
        if (tryLoad(renderer, QIcon::Disabled, state, mode))
            return QIcon::Disabled;
        if (tryLoad(renderer, QIcon::Selected, state, mode))
            return QIcon::Selected;
        if (tryLoad(renderer, QIcon::Disabled, oppositeState, mode))
            return QIcon::Disabled;
        if (tryLoad(renderer, QIcon::Selected, oppositeState, mode))
            return QIcon::Selected;
    }
    return QIcon::Normal;
}

QPixmap RecoloringSvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                               QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap RecoloringSvgIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state,
                                     qreal scale)
{
    QPixmap pm;

    QString pmckey(d->pmcKey(size, mode, state, scale, QGuiApplication::palette()));
    if (QPixmapCache::find(pmckey, &pm))
        return pm;

    if (!d->addedPixmaps.isEmpty()) {
        const auto realSize = size * scale;
        const auto key = d->hashKey(mode, state);
        auto it = d->addedPixmaps.constFind(key);
        while (it != d->addedPixmaps.end() && it.key() == key) {
            const auto &pm = it.value();
            if (!pm.isNull()) {
                // we don't care about dpr here - don't use QSvgIconEngine when
                // there are a lot of raster images are to handle.
                if (pm.size() == realSize)
                    return pm;
            }
            ++it;
        }
    }

    QSvgRenderer renderer;
    const QIcon::Mode loadmode = d->loadDataForModeAndState(&renderer, mode, state);
    if (!renderer.isValid())
        return pm;

    QSize actualSize = renderer.defaultSize();
    if (!actualSize.isNull())
        actualSize.scale(size * scale, Qt::KeepAspectRatio);

    if (actualSize.isEmpty())
        return pm;

    pm = QPixmap(actualSize);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    p.end();
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        if (loadmode != mode && mode != QIcon::Normal) {
            const QPixmap generated = QGuiApplicationPrivate::instance()->applyQIconStyleHelper(mode, pm);
            if (!generated.isNull())
                pm = generated;
        }
    }

    if (!pm.isNull()) {
        pm.setDevicePixelRatio(scale);
        QPixmapCache::insert(pmckey, pm);
    }

    return pm;
}


void RecoloringSvgIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                               QIcon::State state)
{
    d->stepSerialNum();
    d->addedPixmaps.insert(d->hashKey(mode, state), pixmap);
}

void RecoloringSvgIconEngine::addFile(const QString &fileName, const QSize &,
                                      QIcon::Mode mode, QIcon::State state)
{
    if (!fileName.isEmpty()) {
         const QFileInfo fi(fileName);
         const QString abs = fi.absoluteFilePath();
         const FileType type = fileType(fi);
#ifndef QT_NO_COMPRESS
         if (type == SvgFile || type == CompressedSvgFile) {
#else
         if (type == SvgFile) {
#endif
             QSvgRenderer renderer(abs);
             if (renderer.isValid()) {
                 d->stepSerialNum();
                 d->svgFiles.insert(d->hashKey(mode, state), abs);
             }
         } else if (type == OtherFile) {
             QPixmap pm(abs);
             if (!pm.isNull())
                 addPixmap(pm, mode, state);
         }
    }
}

void RecoloringSvgIconEngine::paint(QPainter *painter, const QRect &rect,
                           QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
    if (painter->device())
        pixmapSize *= painter->device()->devicePixelRatioF();
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

bool RecoloringSvgIconEngine::isNull()
{
    return d->svgFiles.isEmpty() && d->addedPixmaps.isEmpty() && d->svgBuffers.isEmpty();
}

QString RecoloringSvgIconEngine::key() const
{
    return QLatin1String("svg");
}

QIconEngine *RecoloringSvgIconEngine::clone() const
{
    return new RecoloringSvgIconEngine(*this);
}


bool RecoloringSvgIconEngine::read(QDataStream &in)
{
    d = new RecoloringSvgIconEnginePrivate;

    if (in.version() >= QDataStream::Qt_4_4) {
        int isCompressed;
        QHash<int, QString> fileNames;  // For memoryoptimization later
        in >> fileNames >> isCompressed >> d->svgBuffers;
#ifndef QT_NO_COMPRESS
        if (!isCompressed) {
            for (auto &svgBuf : d->svgBuffers)
                svgBuf = qCompress(svgBuf);
        }
#else
        if (isCompressed) {
            qWarning("RecoloringSvgIconEngine: Can not decompress SVG data");
            d->svgBuffers.clear();
        }
#endif
        int hasAddedPixmaps;
        in >> hasAddedPixmaps;
        if (hasAddedPixmaps) {
            in >> d->addedPixmaps;
        }
    }
    else {
        QPixmap pixmap;
        QByteArray data;
        uint mode;
        uint state;
        int num_entries;

        in >> data;
        if (!data.isEmpty()) {
#ifndef QT_NO_COMPRESS
            data = qUncompress(data);
#endif
            if (!data.isEmpty())
                d->svgBuffers.insert(d->hashKey(QIcon::Normal, QIcon::Off), data);
        }
        in >> num_entries;
        for (int i=0; i<num_entries; ++i) {
            if (in.atEnd())
                return false;
            in >> pixmap;
            in >> mode;
            in >> state;
            // The pm list written by 4.3 is buggy and/or useless, so ignore.
            //addPixmap(pixmap, QIcon::Mode(mode), QIcon::State(state));
        }
    }

    return true;
}


bool RecoloringSvgIconEngine::write(QDataStream &out) const
{
    if (out.version() >= QDataStream::Qt_4_4) {
        int isCompressed = 0;
#ifndef QT_NO_COMPRESS
        isCompressed = 1;
#endif
        QHash<int, QByteArray> svgBuffers = d->svgBuffers;
        for (const auto &it : d->svgFiles.asKeyValueRange()) {
            QByteArray buf;
            QFile f(it.second);
            if (f.open(QIODevice::ReadOnly))
                buf = f.readAll();
#ifndef QT_NO_COMPRESS
            buf = qCompress(buf);
#endif
            svgBuffers.insert(it.first, buf);
        }
        out << d->svgFiles << isCompressed << svgBuffers;
        if (d->addedPixmaps.isEmpty())
            out << 0;
        else
            out << 1 << d->addedPixmaps;
    }
    else {
        const auto key = d->hashKey(QIcon::Normal, QIcon::Off);
        QByteArray buf = d->svgBuffers.value(key);
        if (buf.isEmpty()) {
            QString svgFile = d->svgFiles.value(key);
            if (!svgFile.isEmpty()) {
                QFile f(svgFile);
                if (f.open(QIODevice::ReadOnly))
                    buf = f.readAll();
            }
        }
#ifndef QT_NO_COMPRESS
        buf = qCompress(buf);
#endif
        out << buf;
        // 4.3 has buggy handling of added pixmaps, so don't write any
        out << (int)0;
    }
    return true;
}

}
