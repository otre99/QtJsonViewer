#ifndef LIST_VIEW_SEARCH_MODEL_H
#define LIST_VIEW_SEARCH_MODEL_H

#include <QAbstractListModel>
#include <QFont>
#include <QObject>
class JsonTreeViewModel;

class ListViewSearchModel : public QAbstractListModel {
  Q_OBJECT
 public:
  enum class SearchMode : unsigned char {
    KeyOrValue = 0,
    Key,
    ValueAny,
    ValueStr,
    ValueNum,
    ValueBool,
    ValueNull,
  };
  explicit ListViewSearchModel(JsonTreeViewModel *lm,
                               QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
  int columnCount(
      const QModelIndex &parent = QModelIndex()) const override final;
  QVariant data(const QModelIndex &idx, int role) const override final;
  void doSearch(SearchMode mode, const QString &searchText, bool caseSensitive,
                bool wholeWord, bool useRegex);
  QModelIndex toTreeView(const QModelIndex &idx) const;

 public slots:
  void clear();
  void setFontSize(const double pt);

 private:
  JsonTreeViewModel *m_jsonTreeViewModel{nullptr};
  QVector<quint32> m_validNodes;
  QFont m_baseFont;
};

#endif  // LIST_VIEW_SEARCH_MODEL_H
