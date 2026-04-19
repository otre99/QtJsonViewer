#ifndef JSON_TREEVIEW_MODEL_H
#define JSON_TREEVIEW_MODEL_H
#include <QAbstractItemModel>
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QList>
#include <QProgressDialog>

#include "json_nodes.h"

class JsonTreeViewModel : public QAbstractItemModel {
  Q_OBJECT
  friend class ListViewSearchModel;
  struct Brushes {
    QBrush key;
    QBrush str;
    QBrush num;
    QBrush boolean;
    QBrush nullish;
    QBrush containerArray;
    QBrush containerObject;
  } B;

public:
  JsonTreeViewModel(QObject *parent);
  ~JsonTreeViewModel();
  bool populateFromJson(const QString jsonFilePath, QString *errorMsg = nullptr);

  QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent = {}) const override;
  int columnCount(const QModelIndex &parentIndex = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  QString nodePath(const QModelIndex &idx) const;
  QString nodeKey(const QModelIndex &idx) const;
  QString nodeValueStr(const QModelIndex &idx) const;
  NodeType nodeType(const QModelIndex &idx) const;
  quint32 subTreeNodesCount(const QModelIndex &idx) const;

signals:

public slots:
  void setFontSize(const double pt);

private:
  quint32 nodeFromIndex(const QModelIndex &idx) const;
  QFont m_baseFont;
  FastJsonTree m_fastJsonTree;
};

#endif // JSON_TREEVIEW_MODEL_H
