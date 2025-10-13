#ifndef JSON_NODES_H
#define JSON_NODES_H
#include <QJsonValue>
#include <QString>
#include <QVariant>
#include <QVector>
#include <format>
#include <limits>
using namespace std;
constexpr long PREALLOCATION_SIZE = 128000000;
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

struct Node {
  string m_key;
  quint32 m_parentPosition{MAX_U32};
  quint32 m_rowInParent{MAX_U32};
  virtual ~Node() {}

  Node(const string &key, quint32 parentPos, quint32 rowInParent)
      : m_key{key}, m_parentPosition{parentPos}, m_rowInParent{rowInParent} {}
  virtual NodeType type() const = 0;
  virtual string value() const = 0;
  string key() const {
    return m_key.empty() ? format("[{}]:", m_rowInParent) : m_key + ":";
  }
};

struct NullNode : public Node {
  NullNode(const string &key, quint32 parentPos, quint32 rowInParent)
      : Node{key, parentPos, rowInParent} {}
  virtual NodeType type() const { return NodeType::Null; }
  virtual string value() const { return "null"; }
};

struct ArrayNode : public Node {
  quint32 m_firstchildPos{0};
  quint32 m_childCount{0};

  ArrayNode(const string &key, quint32 parentPos, quint32 rowInParent)
      : Node{key, parentPos, rowInParent} {}
  virtual NodeType type() const { return NodeType::Array; }
  virtual string value() const { return m_childCount ? "[...]" : "[]"; }
};

struct ObjectNode : public Node {
  quint32 m_firstchildPos{0};
  quint32 m_childCount{0};

  ObjectNode(const string &key, quint32 parentPos, quint32 rowInParent)
      : Node{key, parentPos, rowInParent} {}
  virtual NodeType type() const { return NodeType::Object; }
  virtual string value() const { return m_childCount ? "{...}" : "{}"; }
};

struct NumNode : public Node {
  double m_value;

  NumNode(const string &key, quint32 parentPos, quint32 rowInParent,
          double value)
      : Node{key, parentPos, rowInParent}, m_value{value} {}
  virtual NodeType type() const { return NodeType::Num; }
  virtual string value() const { return format("{}", m_value); }
};

struct StrNode : public Node {
  string m_value;

  StrNode(const string &key, quint32 parentPos, quint32 rowInParent,
          const string &value)
      : Node{key, parentPos, rowInParent}, m_value{value} {}
  virtual NodeType type() const { return NodeType::Str; }
  virtual string value() const { return m_value; }
};

struct BoolNode : public Node {
  bool m_value;

  BoolNode(const string &key, quint32 parentPos, quint32 rowInParent,
           bool value)
      : Node{key, parentPos, rowInParent}, m_value{value} {}
  virtual NodeType type() const { return NodeType::Bool; }
  virtual string value() const { return format("{}", m_value); }
};

class FastJsonTree {
 public:
  FastJsonTree();
  void buildTree(const QJsonValue &jsonValue, QProgressDialog *progressDialog);
  Node *node(quint32 pos) const;
  void clear();
  bool isEmpty() const;

 private:
  vector<Node *> m_nodes{};
  Node *createNodeFromJsonValue(const QString &key,
                                const QJsonValue &value = {},
                                quint32 parentPos = 0, quint32 rowInParent = 0,
                                quint32 firstChildPos = 0,
                                quint32 childCount = 0);
  void _buildTreeRecursive(const QJsonValue &jsonValue, quint32 parentIndex);
  QProgressDialog *m_progressDlg{nullptr};
};

#endif  // JSON_NODES_H
