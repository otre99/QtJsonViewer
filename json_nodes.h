#ifndef JSON_NODES_H
#define JSON_NODES_H
#include <QJsonValue>
#include <QString>
#include <QVariant>
#include <QVector>
#include <limits>
using namespace std;
constexpr long PREALLOCATION_SIZE = 256000000;
constexpr quint32 MAX_U32 = numeric_limits<quint32>::max();

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

 public:
  FastJsonTree();
  void buildTree(const QJsonValue &jsonValue, QProgressDialog *progressDialog);

  string key(quint32 n) const;
  QVariant value(quint32 n) const;
  quint32 parent(quint32 n) const;
  quint32 rows(quint32 n) const;
  quint32 firstChild(quint32 n) const;
  quint32 rowInParent(quint32 n) const;
  NodeType type(quint32 n) const;
  string nodePath(quint32 n) const;

  void clear();
  bool isEmpty() const;

 private:
  void _buildTreeRecursive(const QJsonValue &jsonValue, quint32 parentIndex);
  quint32 _addKeyAndGetPos(const QString &key);
  void _addNode(const quint32 pIdx, const quint32 cIdx, const quint32 cCount,
                const quint32 row, const NodeType type, const quint32 kIdx,
                const Data d);
  NodeType _typeFromJson(const QJsonValue &value, Data &d);

  QProgressDialog *m_progressDlg{nullptr};

  vector<quint32> m_parentPosition;
  vector<quint32> m_firstChildPosition;
  vector<quint32> m_childCount;
  vector<quint32> m_rowInParent;
  vector<NodeType> m_nodeType;
  vector<quint32> m_nodeKeyPos;
  vector<Data> m_nodeValueOrPos;

  vector<string> m_nodeStringValueTable;
  vector<string> m_nodeKeyTable;
  QHash<QString, int> m_allKeys;
  qint32 m_currentKeyPos{-1};
};

#endif  // JSON_NODES_H
