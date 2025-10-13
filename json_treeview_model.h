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
 public:
  JsonTreeViewModel(QObject *parent);
  ~JsonTreeViewModel();
  void populateFromJson(const QJsonDocument &doc,
                        QProgressDialog &progressDialog);

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = {}) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent = {}) const override;
  int columnCount(const QModelIndex &parentIndex = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  // bool setData(const QModelIndex &idx, const QVariant &value,
  //              int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

 signals:

 public slots:
  void setFontSize(const double pt);

 private:
  Node *nodeFromIndex(const QModelIndex &idx) const;
  QFont m_baseFont;
  FastJsonTree m_fastJsonTree;
};

#endif  // JSON_TREEVIEW_MODEL_H
