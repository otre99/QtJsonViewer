#include "json_nodes.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QProgressDialog>
FastJsonTree::FastJsonTree() { m_nodes.reserve(PREALLOCATION_SIZE); }

void FastJsonTree::buildTree(const QJsonValue &jsonValue,
                             QProgressDialog *progressDialog) {
  m_nodes.clear();

  Node *n;
  if (jsonValue.isArray()) {
    n = new ArrayNode("<root>", MAX_U32, 0);
  } else if (jsonValue.isObject()) {
    n = new ObjectNode("<root>", MAX_U32, 0);
  } else {
    return;
  }
  m_nodes.push_back(n);
  m_progressDlg = progressDialog;
  _buildTreeRecursive(jsonValue, 0);
  // auto s = jsonValue.toVariant();
  // qDebug() << "Nodes count " << m_nodes.size() << sizeof(s);
}

Node *FastJsonTree::node(quint32 pos) const { return m_nodes[pos]; }

void FastJsonTree::clear() {
  for (auto ptr : m_nodes) {
    delete ptr;
  }
  m_nodes.clear();
}

bool FastJsonTree::isEmpty() const { return m_nodes.empty(); }

Node *FastJsonTree::createNodeFromJsonValue(
    const QString &key, const QJsonValue &value, quint32 parentPos,
    quint32 rowInParent, quint32 firstChildPos, quint32 childCount) {
  switch (value.type()) {
    case QJsonValue::Array:
      return new ArrayNode(key.toStdString(), parentPos, rowInParent);

    case QJsonValue::Object:
      return new ObjectNode(key.toStdString(), parentPos, rowInParent);

    case QJsonValue::Null:
    case QJsonValue::Undefined:
      return new NullNode(key.toStdString(), parentPos, rowInParent);

    case QJsonValue::Double:
      return new NumNode(key.toStdString(), parentPos, rowInParent,
                         value.toDouble());

    case QJsonValue::Bool:
      return new BoolNode(key.toStdString(), parentPos, rowInParent,
                          value.toBool());

    case QJsonValue::String:
      return new StrNode(key.toStdString(), parentPos, rowInParent,
                         value.toString().toStdString());
  }
}

void FastJsonTree::_buildTreeRecursive(const QJsonValue &jsonValue,
                                       quint32 parentIndex) {
  const quint32 firstChildPosition = m_nodes.size();

  if (m_progressDlg && m_nodes.size() % 16384 * 4 == 0) {
    m_progressDlg->setLabelText(
        QString("Number of nodes: %1").arg(m_nodes.size()));
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
  }

  vector<pair<QJsonValue, quint32>> pending;
  // Make sure the child nodes are arranged consecutively
  if (jsonValue.isArray()) {
    const QJsonArray array = jsonValue.toArray();
    pending.reserve(array.size());
    int index = 0;
    for (auto &&value : array) {
      const QString key = QString("[%1]").arg(index);
      Node *n = createNodeFromJsonValue(key, value, parentIndex, index);
      m_nodes.push_back(n);
      ++index;
      if (value.isArray() || value.isObject()) {
        pending.emplace_back(value, m_nodes.size() - 1);
      }
    }

    // update childrem info in parent node
    auto ni = static_cast<ArrayNode *>(m_nodes[parentIndex]);
    ni->m_firstchildPos = firstChildPosition;
    ni->m_childCount = m_nodes.size() - firstChildPosition;

  } else {  // object
    const QJsonObject object = jsonValue.toObject();
    const QStringList keys = object.keys();
    pending.reserve(object.size());

    int index = 0;
    for (auto &&key : keys) {
      const QJsonValue &value = object[key];
      Node *n = createNodeFromJsonValue(key, value, parentIndex, index);
      m_nodes.push_back(n);
      ++index;
      if (value.isArray() || value.isObject()) {
        pending.emplace_back(value, m_nodes.size() - 1);
      }
    }

    // update childrem info in parent node
    auto ni = static_cast<ObjectNode *>(m_nodes[parentIndex]);
    ni->m_firstchildPos = firstChildPosition;
    ni->m_childCount = m_nodes.size() - firstChildPosition;
  }

  // Process each child node
  for (auto &&[value, index] : pending) {
    _buildTreeRecursive(value, index);
  }
}
