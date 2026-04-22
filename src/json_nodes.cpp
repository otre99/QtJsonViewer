#include "json_nodes.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QProgressDialog>
#include <format>

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

void FastJsonTree::buildTree(const SimdJsonElement &doc) {
  clear();

  m_parentPosition.push_back(MAX_U32);
  m_firstChildPosition.push_back(MAX_U32);
  m_childCount.push_back(0);
  m_rowInParent.push_back(0);
  m_nodeKeyPos.push_back(MAX_U32);
  m_nodeValueOrPos.push_back({});

  const auto docType = doc.type();
  if (docType == simdjson::dom::element_type::ARRAY) {
    m_nodeType.push_back(NodeType::Array);
  } else if (docType == simdjson::dom::element_type::OBJECT) {
    m_nodeType.push_back(NodeType::Object);
  } else {
    clear();
    return;
  }
  _buildTreeRecursive(doc, 0);

  m_nodeKeyTable.resize(m_allKeys.size());
  const auto keys = m_allKeys.keys();
  for (auto &&k : keys) {
    m_nodeKeyTable[m_allKeys[k]] = k;
  }
  m_allKeys.clear();

  qDebug() << " No. nodes: " << m_parentPosition.size() << "nodes";
  qDebug() << " Str nodes: " << m_nodeStringValueTable.size();
  qDebug() << " Num + Bool + Null nodes: " << m_parentPosition.size() - m_nodeStringValueTable.size();
  qDebug() << "Unique keys: " << m_nodeKeyTable.size() << "nodes";
}

string FastJsonTree::key(quint32 n) const {
  if (n == 0)
    return "<root>";
  quint32 pn_idx = parent(n);

  switch (type(pn_idx)) {
  case NodeType::Array:
    return format("[{}]", rowInParent(n));
  default:
    return m_nodeKeyTable[m_nodeKeyPos[n]];
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
  throw std::logic_error("FastJsonTree::value(): Unknow type for value: " + to_string(n));
}

string FastJsonTree::valueAsStr(quint32 n) const {
  switch (type(n)) {
  case NodeType::Array:
    return rows(n) ? "[...]" : "[]";
  case NodeType::Object:
    return rows(n) ? "{...}" : "{}";
  case NodeType::Num:
    return format("{}", m_nodeValueOrPos[n].num);
  case NodeType::Bool:
    return m_nodeValueOrPos[n].index ? "true" : "false";
  case NodeType::Str:
    return m_nodeStringValueTable[m_nodeValueOrPos[n].index];
  case NodeType::Null:
    return "null";
  }
  throw std::logic_error("FastJsonTree::valueAsStr(): Unknow type for value: " + to_string(n));
}

string FastJsonTree::nodePreview(quint32 n) const {
  string k = key(n);
  string v = valueAsStr(n);
  switch (type(n)) {
  case NodeType::Array:
  case NodeType::Object:
  case NodeType::Num:
  case NodeType::Bool:
  case NodeType::Null: {
    return format("\"{}\": {}", k, v);
  }
  case NodeType::Str: {
    if (v.size() > 64) {
      v = v.substr(0, 60) + "...";
    }
    return format("\"{}\": \"{}\"", k, v);
  }
  }
  throw std::logic_error("FastJsonTree::nodePreview(): Unknow type for value: " + to_string(n));
}

quint32 FastJsonTree::parent(quint32 n) const {
  return m_parentPosition[n];
}
quint32 FastJsonTree::rows(quint32 n) const {
  return m_childCount[n];
}
quint32 FastJsonTree::firstChild(quint32 n) const {
  return m_firstChildPosition[n];
}
quint32 FastJsonTree::rowInParent(quint32 n) const {
  return m_rowInParent[n];
}
NodeType FastJsonTree::type(quint32 n) const {
  return m_nodeType[n];
}

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

quint32 FastJsonTree::nodesCount() const {
  return m_parentPosition.size();
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

bool FastJsonTree::isEmpty() const {
  return /*true*/ m_parentPosition.empty();
}

quint32 FastJsonTree::_buildTreeRecursive(const SimdJsonElement &jsonValue, quint32 parentIndex) {
  const auto jsonType = jsonValue.type();
  const quint32 firstChildPosition = m_parentPosition.size();
  quint32 childrenCount = 0;

  if (jsonType == simdjson::dom::element_type::ARRAY) {
    simdjson::dom::array array;
    if (jsonValue.get(array)) {
      return 0;
    }
    childrenCount = static_cast<quint32>(array.size());
  } else {
    simdjson::dom::object object;
    if (jsonValue.get(object)) {
      return 0;
    }
    childrenCount = static_cast<quint32>(object.size());
  }

  if (childrenCount) {
    m_parentPosition.resize(firstChildPosition + childrenCount);
    m_firstChildPosition.resize(firstChildPosition + childrenCount);
    m_childCount.resize(firstChildPosition + childrenCount);
    m_rowInParent.resize(firstChildPosition + childrenCount);
    m_nodeType.resize(firstChildPosition + childrenCount);
    m_nodeKeyPos.resize(firstChildPosition + childrenCount);
    m_nodeValueOrPos.resize(firstChildPosition + childrenCount);
  }

  int child_count = childrenCount;
  quint32 row = 0;
  Data d;
  if (jsonType == simdjson::dom::element_type::ARRAY) {
    simdjson::dom::array array;
    if (jsonValue.get(array)) {
      return 0;
    }

    for (const simdjson::dom::element val : array) {
      const quint32 idx = firstChildPosition + row;
      const NodeType type = _typeFromJson(val, d);

      m_parentPosition[idx] = parentIndex;
      m_firstChildPosition[idx] = MAX_U32;
      m_childCount[idx] = MAX_U32;
      m_rowInParent[idx] = row;
      m_nodeType[idx] = type;
      m_nodeKeyPos[idx] = MAX_U32;
      m_nodeValueOrPos[idx] = d;

      if (type == NodeType::Array || type == NodeType::Object) {
        child_count += _buildTreeRecursive(val, idx);
      }
      ++row;
    }
  } else {
    simdjson::dom::object object;
    if (jsonValue.get(object)) {
      return 0;
    }

    for (const simdjson::dom::key_value_pair field : object) {
      const quint32 idx = firstChildPosition + row;
      const string key(field.key.data(), field.key.size());
      const SimdJsonElement val = field.value;

      const NodeType type = _typeFromJson(val, d);
      const quint32 kIdx = _addKeyAndGetPos(key);

      m_parentPosition[idx] = parentIndex;
      m_firstChildPosition[idx] = MAX_U32;
      m_childCount[idx] = MAX_U32;
      m_rowInParent[idx] = row;
      m_nodeType[idx] = type;
      m_nodeKeyPos[idx] = kIdx;
      m_nodeValueOrPos[idx] = d;

      if (type == NodeType::Array || type == NodeType::Object) {
        child_count += _buildTreeRecursive(val, idx);
      }
      ++row;
    }
  }
  m_firstChildPosition[parentIndex] = firstChildPosition;
  m_childCount[parentIndex] = childrenCount;
  m_nodeValueOrPos[parentIndex].index = child_count;
  return child_count;
}

quint32 FastJsonTree::_addKeyAndGetPos(const string &key) {
  auto iter = m_allKeys.find(key);
  if (iter != m_allKeys.end()) {
    return iter.value();
  }
  m_allKeys.insert(key, m_currentKeyPos++);
  return m_currentKeyPos - 1;
}

NodeType FastJsonTree::_typeFromJson(const SimdJsonElement &value, Data &d) {
  switch (value.type()) {
  case simdjson::dom::element_type::NULL_VALUE:
    return NodeType::Null;

  case simdjson::dom::element_type::BOOL: {
    bool parsed = false;
    std::ignore = value.get(parsed);
    d.index = parsed;
    return NodeType::Bool;
  }

  case simdjson::dom::element_type::STRING: {
    std::string_view parsed;
    std::ignore = value.get(parsed);
    m_nodeStringValueTable.emplace_back(parsed);
    d.index = m_nodeStringValueTable.size() - 1;
    return NodeType::Str;
  }

  case simdjson::dom::element_type::DOUBLE: {
    double parsed = 0;
    std::ignore = value.get(parsed);
    d.num = static_cast<double>(parsed);
    return NodeType::Num;
  }

  case simdjson::dom::element_type::INT64: {
    int64_t parsed = 0;
    std::ignore = value.get(parsed);
    d.num = static_cast<double>(parsed);
    return NodeType::Num;
  }

  case simdjson::dom::element_type::OBJECT:
    return NodeType::Object;

  case simdjson::dom::element_type::ARRAY:
    return NodeType::Array;

  case simdjson::dom::element_type::UINT64:
  case simdjson::dom::element_type::BIGINT: {
    qWarning() << "BIGINT or UINT64 is parsed as 0";
    d.num = 0;
    return NodeType::Num;
  }
  }

  return NodeType::Null;
}
