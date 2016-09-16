#ifndef TORRENTFILETREEVIEW_H
#define TORRENTFILETREEVIEW_H

#include <QStyledItemDelegate>
#include <QTreeView>

class QPainter;
class QSortFilterProxyModel;

class TorrentFileModel;

class TorrentFileTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TorrentFileTreeDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class TorrentFileTreeView : public QTreeView
{
    Q_OBJECT
public:
    TorrentFileTreeView(TorrentFileModel *model, QWidget *parent = 0);
private:
    TorrentFileModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
private slots:
    void enableSorting();
};

#endif // TORRENTFILETREEVIEW_H
