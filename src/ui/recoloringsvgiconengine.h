// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef TREMOTESF_RECOLORINGSVGICONENGINE_H
#define TREMOTESF_RECOLORINGSVGICONENGINE_H

#include <QtGui/qiconengine.h>
#include <QtCore/qshareddata.h>

// clang-format off

namespace tremotesf {

class RecoloringSvgIconEnginePrivate;

// QSvgIconEngine fork that injects CSS derived from current QPalette
class RecoloringSvgIconEngine : public QIconEngine
{
public:
    RecoloringSvgIconEngine();
    RecoloringSvgIconEngine(const RecoloringSvgIconEngine&other);
    ~RecoloringSvgIconEngine();
    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode,
                     QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode,
                         QIcon::State state, qreal scale) override;

    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                   QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size,
                 QIcon::Mode mode, QIcon::State state) override;

    bool isNull() override;
    QString key() const override;
    QIconEngine *clone() const override;
    bool read(QDataStream &in) override;
    bool write(QDataStream &out) const override;
private:
    QSharedDataPointer<RecoloringSvgIconEnginePrivate> d;
};

}

#endif
