#ifndef SYSTEMCOLORSPROVIDER_H
#define SYSTEMCOLORSPROVIDER_H

#include <memory>
#include <QColor>
#include <QObject>

namespace tremotesf {

class SystemColorsProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool darkThemeEnabled READ isDarkThemeEnabled NOTIFY darkThemeEnabledChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)
public:
    explicit SystemColorsProvider(QObject* parent = nullptr) : QObject{parent} {}
    static std::unique_ptr<SystemColorsProvider> createInstance();
    virtual bool isDarkThemeEnabled() const { return false; };
    virtual QColor accentColor() const { return {}; };

signals:
    void darkThemeEnabledChanged();
    void accentColorChanged();
};

}

#endif // SYSTEMCOLORSPROVIDER_H
