// SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SPDX-FileCopyrightText: 2000 Geert Jansen <jansen@kde.org>
// SPDX-FileCopyrightText: 2000 Antonio Larrosa <larrosa@kde.org>
// SPDX-FileCopyrightText: 2010 Michael Pyne <mpyne@kde.org>
// SPDX-License-Identifier: LGPL-2.0-only

#include "recoloringsvgiconengine.h"

#include <QApplication>
#include <QAtomicInt>
#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QHash>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QSvgRenderer>
#include <QStyle>
#include <QStyleOption>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace tremotesf {

namespace {

QString STYLESHEET_TEMPLATE()
{
    /* clang-format off */
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
    /* clang-format on */
}

enum FileType { OtherFile, SvgFile, CompressedSvgFile };

static FileType fileType(const QFileInfo &fi)
{
    const QString &abs = fi.absoluteFilePath();
    if (abs.endsWith(QLatin1String(".svg"), Qt::CaseInsensitive))
        return SvgFile;
    if (abs.endsWith(QLatin1String(".svgz"), Qt::CaseInsensitive)
        || abs.endsWith(QLatin1String(".svg.gz"), Qt::CaseInsensitive)) {
        return CompressedSvgFile;
    }
#ifndef QT_NO_MIMETYPE
    const QString &mimeTypeName = QMimeDatabase().mimeTypeForFile(fi).name();
    if (mimeTypeName == QLatin1String("image/svg+xml"))
        return SvgFile;
    if (mimeTypeName == QLatin1String("image/svg+xml-compressed"))
        return CompressedSvgFile;
#endif // !QT_NO_MIMETYPE
    return OtherFile;
}

}

class RecoloringSvgIconEnginePrivate : public QSharedData
{
public:
    RecoloringSvgIconEnginePrivate()
        : svgBuffers(0), addedPixmaps(0)
        { stepSerialNum(); }

    ~RecoloringSvgIconEnginePrivate()
        { delete addedPixmaps; delete svgBuffers; }

    static int hashKey(QIcon::Mode mode, QIcon::State state)
        { return (((mode)<<4)|state); }

    QString pmcKey(const QSize &size, QIcon::Mode mode, QIcon::State state, const QPalette& pal)
        { return QLatin1String("$qt_svgicon_")
                 + QString::number(serialNum, 16).append(QLatin1Char('_'))
                 + QString::number((((((qint64(size.width()) << 11) | size.height()) << 11) | mode) << 4) | state, 16).append(QLatin1Char('_'))
                 + pal.windowText().color().name().append(QLatin1Char('_'))
                 + pal.highlight().color().name().append(QLatin1Char('_'))
                 + pal.highlightedText().color().name(); }

    void stepSerialNum()
        { serialNum = lastSerialNum.fetchAndAddRelaxed(1); }

    bool tryLoad(QSvgRenderer *renderer, QIcon::Mode tryMode, QIcon::State tryState, QIcon::Mode actualMode);
    QIcon::Mode loadDataForModeAndState(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state);

    QHash<int, QString> svgFiles;
    QHash<int, QByteArray> *svgBuffers;
    QHash<int, QPixmap> *addedPixmaps;
    int serialNum;
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
    if (other.d->svgBuffers)
        d->svgBuffers = new QHash<int, QByteArray>(*other.d->svgBuffers);
    if (other.d->addedPixmaps)
        d->addedPixmaps = new QHash<int, QPixmap>(*other.d->addedPixmaps);
}


RecoloringSvgIconEngine::~RecoloringSvgIconEngine()
{
}


QSize RecoloringSvgIconEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    if (d->addedPixmaps) {
        QPixmap pm = d->addedPixmaps->value(d->hashKey(mode, state));
        if (!pm.isNull() && pm.size() == size)
            return size;
    }

    QPixmap pm = pixmap(size, mode, state);
    if (pm.isNull())
        return QSize();
    return pm.size();
}

static QByteArray maybeUncompress(const QByteArray &ba)
{
#ifndef QT_NO_COMPRESS
    return qUncompress(ba);
#else
    return ba;
#endif
}

bool RecoloringSvgIconEnginePrivate::tryLoad(QSvgRenderer *renderer, QIcon::Mode tryMode, QIcon::State tryState, QIcon::Mode actualMode)
{
    if (svgBuffers) {
        QByteArray buf = svgBuffers->value(hashKey(tryMode, tryState));
        if (!buf.isEmpty()) {
            buf = maybeUncompress(buf);
            renderer->load(buf);
            return true;
        }
    }
    QString svgFile = svgFiles.value(hashKey(tryMode, tryState));
    if (!svgFile.isEmpty()) {
        if (fileType(QFileInfo(svgFile)) == CompressedSvgFile) {
            qWarning() << "Can't recolor compressed svg" << svgFile;
            renderer->load(svgFile);
            return true;
        }
        const auto pal = QGuiApplication::palette();
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

        renderer->load(processedContents);
        return true;
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
    QPixmap pm;

    QString pmckey(d->pmcKey(size, mode, state, QGuiApplication::palette()));
    if (QPixmapCache::find(pmckey, &pm))
        return pm;

    if (d->addedPixmaps) {
        pm = d->addedPixmaps->value(d->hashKey(mode, state));
        if (!pm.isNull() && pm.size() == size)
            return pm;
    }

    QSvgRenderer renderer;
    const QIcon::Mode loadmode = d->loadDataForModeAndState(&renderer, mode, state);
    if (!renderer.isValid())
        return pm;

    QSize actualSize = renderer.defaultSize();
    if (!actualSize.isNull())
        actualSize.scale(size, Qt::KeepAspectRatio);

    if (actualSize.isEmpty())
        return QPixmap();

    QImage img(actualSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    QPainter p(&img);
    renderer.render(&p);
    p.end();
    pm = QPixmap::fromImage(img);

    if (loadmode != mode && mode != QIcon::Normal) {
        QStyleOption opt(0);
        opt.palette = QGuiApplication::palette();
        const QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
        if (!generated.isNull())
            pm = generated;
    }

    if (!pm.isNull())
        QPixmapCache::insert(pmckey, pm);

    return pm;
}


void RecoloringSvgIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                               QIcon::State state)
{
    if (!d->addedPixmaps)
        d->addedPixmaps = new QHash<int, QPixmap>;
    d->stepSerialNum();
    d->addedPixmaps->insert(d->hashKey(mode, state), pixmap);
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
    d->svgBuffers = new QHash<int, QByteArray>;

    if (in.version() >= QDataStream::Qt_4_4) {
        int isCompressed;
        QHash<int, QString> fileNames;  // For memoryoptimization later
        in >> fileNames >> isCompressed >> *d->svgBuffers;
#ifndef QT_NO_COMPRESS
        if (!isCompressed) {
            for (auto it = d->svgBuffers->begin(), end = d->svgBuffers->end(); it != end; ++it)
                it.value() = qCompress(it.value());
        }
#else
        if (isCompressed) {
            qWarning("RecoloringSvgIconEngine: Can not decompress SVG data");
            d->svgBuffers->clear();
        }
#endif
        int hasAddedPixmaps;
        in >> hasAddedPixmaps;
        if (hasAddedPixmaps) {
            d->addedPixmaps = new QHash<int, QPixmap>;
            in >> *d->addedPixmaps;
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
                d->svgBuffers->insert(d->hashKey(QIcon::Normal, QIcon::Off), data);
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
        QHash<int, QByteArray> svgBuffers;
        if (d->svgBuffers)
            svgBuffers = *d->svgBuffers;
        for (auto it = d->svgFiles.cbegin(), end = d->svgFiles.cend(); it != end; ++it) {
            QByteArray buf;
            QFile f(it.value());
            if (f.open(QIODevice::ReadOnly))
                buf = f.readAll();
#ifndef QT_NO_COMPRESS
            buf = qCompress(buf);
#endif
            svgBuffers.insert(it.key(), buf);
        }
        out << d->svgFiles << isCompressed << svgBuffers;
        if (d->addedPixmaps)
            out << (int)1 << *d->addedPixmaps;
        else
            out << (int)0;
    }
    else {
        QByteArray buf;
        if (d->svgBuffers)
            buf = d->svgBuffers->value(d->hashKey(QIcon::Normal, QIcon::Off));
        if (buf.isEmpty()) {
            QString svgFile = d->svgFiles.value(d->hashKey(QIcon::Normal, QIcon::Off));
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

void RecoloringSvgIconEngine::virtual_hook(int id, void *data)
{
    if (id == QIconEngine::IsNullHook) {
        *reinterpret_cast<bool*>(data) = d->svgFiles.isEmpty() && !d->addedPixmaps && (!d->svgBuffers || d->svgBuffers->isEmpty());
    }
    QIconEngine::virtual_hook(id, data);
}

}
