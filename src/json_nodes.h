#ifndef JSON_NODES_H
#define JSON_NODES_H
#include <simdjson.h>

#include <QJsonValue>
#include <QString>
#include <QVariant>
#include <QVector>
#include <limits>

using namespace std;
constexpr long PREALLOCATION_SIZE = 256000000;
constexpr quint32 MAX_U32 = numeric_limits<quint32>::max();

using SimdJsonElement = simdjson::dom::element;

class QProgressDialog;

enum class NodeType : unsigned char {
  Null,
  Bool,
  Num,
  Str,
  Array,
  Object,
};

class FastJsonTree {
  union Data {
    double num;
    quint32 index;
  };
  friend class ListViewSearchModel;
  friend class JsonTreeViewModel;

public:
  FastJsonTree();
  void buildTree(const SimdJsonElement &doc);

  string key(quint32 n) const;
  QVariant value(quint32 n) const;
  string valueAsStr(quint32 n) const;
  string nodePreview(quint32 n) const;

  quint32 parent(quint32 n) const;
  quint32 rows(quint32 n) const;
  quint32 firstChild(quint32 n) const;
  quint32 rowInParent(quint32 n) const;
  NodeType type(quint32 n) const;
  string nodePath(quint32 n) const;
  quint32 nodesCount() const;

  void clear();
  bool isEmpty() const;

private:
  quint32 _buildTreeRecursive(const SimdJsonElement &jsonValue, quint32 parentIndex);
  quint32 _addKeyAndGetPos(const string &key);
  NodeType _typeFromJson(const SimdJsonElement &value, Data &d);

  vector<quint32> m_parentPosition;
  vector<quint32> m_firstChildPosition;
  vector<quint32> m_childCount;
  vector<quint32> m_rowInParent;
  vector<NodeType> m_nodeType;
  vector<quint32> m_nodeKeyPos;
  vector<Data> m_nodeValueOrPos;

  vector<string> m_nodeStringValueTable;
  vector<string> m_nodeKeyTable;
  QHash<string, int> m_allKeys;
  qint32 m_currentKeyPos{-1};
};

#endif // JSON_NODES_H
