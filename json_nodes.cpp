#include "json_nodes.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QProgressDialog>
FastJsonTree::FastJsonTree() {
  m_parentPosition.reserve(PREALLOCATION_SIZE);
  m_firstChildPosition.reserve(PREALLOCATION_SIZE);
  m_childCount.reserve(PREALLOCATION_SIZE);
  m_rowInParent.reserve(PREALLOCATION_SIZE);
  m_nodeType.reserve(PREALLOCATION_SIZE);
  m_nodeKeyPos.reserve(PREALLOCATION_SIZE);
  m_nodeValueOrPos.reserve(PREALLOCATION_SIZE);

  m_nodeStringValueTable.reserve(PREALLOCATION_SIZE);
  m_allKeys.clear();
  m_nodeKeyTable.clear();
}

void FastJsonTree::buildTree(const QJsonValue &jsonValue,
                             QProgressDialog *progressDialog) {
  clear();

  Data d;
  if (jsonValue.isArray()) {
    _addNode(MAX_U32, 0, 0, 0, NodeType::Array, _addKeyAndGetPos("<root>"), d);
  } else if (jsonValue.isObject()) {
    _addNode(MAX_U32, 0, 0, 0, NodeType::Array, _addKeyAndGetPos("<root>"), d);
  } else {
    return;
  }
  m_progressDlg = progressDialog;
  _buildTreeRecursive(jsonValue, 0);

  m_nodeKeyTable.resize(m_allKeys.size());
  const QStringList keys = m_allKeys.keys();
  for (auto &&k : keys) {
    m_nodeKeyTable[m_allKeys[k]] = k.toStdString();
  }
  m_allKeys.clear();

  qDebug() << " No. nodes: " << m_parentPosition.size() << "nodes";
  qDebug() << " Str nodes: " << m_nodeStringValueTable.size();
  qDebug() << " Num + Bool + Null nodes: "
           << m_parentPosition.size() - m_nodeStringValueTable.size();
  qDebug() << "Unique keys: " << m_nodeKeyTable.size() << "nodes";
}

string FastJsonTree::key(quint32 n) const {
  quint32 pn_idx = parent(n);
  switch (pn_idx) {
    case MAX_U32:
      return "<root>";
    default: {
      switch (type(pn_idx)) {
        case NodeType::Array:
          return format("[{}]", rowInParent(n));
        default:
          return m_nodeKeyTable[m_nodeKeyPos[n]];
          break;
      }
    }
  }
}

QVariant FastJsonTree::value(quint32 n) const {
  NodeType t = type(n);

  switch (t) {
    case NodeType::Array:
      return rows(n) ? "[...]" : "[]";
    case NodeType::Object:
      return rows(n) ? "{...}" : "{}";
    case NodeType::Num:
      return m_nodeValueOrPos[n].num;
    case NodeType::Bool:
      return bool(m_nodeValueOrPos[n].index);
    case NodeType::Str:
      return m_nodeStringValueTable[m_nodeValueOrPos[n].index].c_str();
    case NodeType::Null:
      return "null";
  }
}

quint32 FastJsonTree::parent(quint32 n) const { return m_parentPosition[n]; }
quint32 FastJsonTree::rows(quint32 n) const { return m_childCount[n]; }
quint32 FastJsonTree::firstChild(quint32 n) const {
  return m_firstChildPosition[n];
}
quint32 FastJsonTree::rowInParent(quint32 n) const { return m_rowInParent[n]; }
NodeType FastJsonTree::type(quint32 n) const { return m_nodeType[n]; }

string FastJsonTree::nodePath(quint32 n) const {
  quint32 idx = n;
  vector<string> pathParts;
  while ((idx = parent(n)) != MAX_U32) {
    if (type(idx) == NodeType::Array) {
      pathParts.push_back(key(n));
    } else {
      pathParts.push_back(format("[\"{}\"]", key(n)));
    }
    n = idx;
  }
  string result = "root";
  for (auto iter = pathParts.rbegin(); iter < pathParts.rend(); ++iter) {
    result += *iter;
  }
  return result;
}

void FastJsonTree::clear() {
  m_parentPosition.clear();
  m_firstChildPosition.clear();
  m_childCount.clear();
  m_rowInParent.clear();
  m_nodeType.clear();
  m_nodeKeyPos.clear();
  m_nodeValueOrPos.clear();

  m_nodeStringValueTable.clear();
  m_currentKeyPos = 0;
}

bool FastJsonTree::isEmpty() const { return /*true*/ m_parentPosition.empty(); }

// Node *FastJsonTree::createNodeFromJsonValue(
//     const QString &key, const QJsonValue &value, quint32 parentPos,
//     quint32 rowInParent, quint32 firstChildPos, quint32 childCount) {
//   switch (value.type()) {
//     case QJsonValue::Array:
//       return new ArrayNode(key.toStdString(), parentPos, rowInParent);

//     case QJsonValue::Object:
//       return new ObjectNode(key.toStdString(), parentPos, rowInParent);

//     case QJsonValue::Null:
//     case QJsonValue::Undefined:
//       return new NullNode(key.toStdString(), parentPos, rowInParent);

//     case QJsonValue::Double:
//       return new NumNode(key.toStdString(), parentPos, rowInParent,
//                          value.toDouble());

//     case QJsonValue::Bool:
//       return new BoolNode(key.toStdString(), parentPos, rowInParent,
//                           value.toBool());

//     case QJsonValue::String:
//       return new StrNode(key.toStdString(), parentPos, rowInParent,
//                          value.toString().toStdString());
//   }
// }

void FastJsonTree::_buildTreeRecursive(const QJsonValue &jsonValue,
                                       quint32 parentIndex) {
  const quint32 firstChildPosition = m_parentPosition.size();

  vector<pair<QJsonValue, quint32>> pending;
  // Make sure the child nodes are arranged consecutively
  if (jsonValue.isArray()) {
    const QJsonArray array = jsonValue.toArray();
    pending.reserve(array.size());
    quint32 row = 0;
    for (auto &&value : array) {
      Data d;
      const NodeType type = _typeFromJson(value, d);
      _addNode(parentIndex, 0, 0, row++, type, 0, d);
      if (type == NodeType::Array || type == NodeType::Object) {
        pending.emplace_back(value, m_parentPosition.size() - 1);
      }
    }

    // update childrem info in parent node
    m_firstChildPosition[parentIndex] = firstChildPosition;
    m_childCount[parentIndex] = array.size();

  } else {  // object
    const QJsonObject object = jsonValue.toObject();
    const QStringList keys = object.keys();
    pending.reserve(object.size());

    int row = 0;
    for (auto &&key : keys) {
      const QJsonValue &value = object[key];

      Data d;
      const NodeType type = _typeFromJson(value, d);
      const quint32 kIdx = _addKeyAndGetPos(key);
      _addNode(parentIndex, 0, 0, row++, type, kIdx, d);

      if (type == NodeType::Array || type == NodeType::Object) {
        pending.emplace_back(value, m_parentPosition.size() - 1);
      }
    }

    // update childrem info in parent node
    m_firstChildPosition[parentIndex] = firstChildPosition;
    m_childCount[parentIndex] = object.size();
  }

  // Process each child node
  for (auto &&[value, index] : pending) {
    _buildTreeRecursive(value, index);
  }
}

quint32 FastJsonTree::_addKeyAndGetPos(const QString &key) {
  auto iter = m_allKeys.find(key);
  if (iter != m_allKeys.end()) {
    return iter.value();
  }
  m_allKeys.insert(key, m_currentKeyPos++);
  return m_currentKeyPos - 1;
}

void FastJsonTree::_addNode(const quint32 pIdx, const quint32 cIdx0,
                            const quint32 cCount, const quint32 row,
                            const NodeType type, const quint32 kIdx,
                            const Data d) {
  m_parentPosition.push_back(pIdx);
  m_firstChildPosition.push_back(cIdx0);
  m_childCount.push_back(cCount);
  m_rowInParent.push_back(row);
  m_nodeType.push_back(type);
  m_nodeKeyPos.push_back(kIdx);
  m_nodeValueOrPos.push_back(d);
}

NodeType FastJsonTree::_typeFromJson(const QJsonValue &value, Data &d) {
  switch (value.type()) {
    case QJsonValue::Array:
      return NodeType::Array;
    case QJsonValue::Object:
      return NodeType::Object;

    case QJsonValue::Null:
    case QJsonValue::Undefined:
      return NodeType::Null;

    case QJsonValue::Double: {
      d.num = value.toDouble();
      return NodeType::Num;
    }
    case QJsonValue::Bool: {
      d.index = value.toBool();
      return NodeType::Bool;
    }

    case QJsonValue::String: {
      m_nodeStringValueTable.push_back(value.toString().toStdString());
      d.index = m_nodeStringValueTable.size() - 1;
      return NodeType::Str;
    }
  }
}
