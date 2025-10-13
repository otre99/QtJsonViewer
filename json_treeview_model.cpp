#include "json_treeview_model.h"

#include <qprogressdialog.h>

#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonArray>
#include <QWidget>

JsonTreeViewModel::JsonTreeViewModel(QObject *parent)
    : QAbstractItemModel(parent) {
  m_baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
}

void JsonTreeViewModel::populateFromJson(const QJsonDocument &doc,
                                         QProgressDialog &progressDialog) {
  beginResetModel();

  m_fastJsonTree.clear();

  progressDialog.setValue(50);
  if (doc.isArray()) {
    auto value = QJsonValue(doc.array());
    m_fastJsonTree.buildTree(value, &progressDialog);
  } else if (doc.isObject()) {
    auto value = QJsonValue(doc.object());
    m_fastJsonTree.buildTree(value, &progressDialog);
  }

  endResetModel();
}

JsonTreeViewModel::~JsonTreeViewModel() { m_fastJsonTree.clear(); }

QModelIndex JsonTreeViewModel::index(int row, int column,
                                     const QModelIndex &parentIdx) const {
  // qDebug() << "index()" << row << column << parentIdx;
  if (column < 0 || row < 0 || m_fastJsonTree.isEmpty()) return {};
  if (!parentIdx.isValid()) {
    return createIndex(row, column, static_cast<quintptr>(0));
  }

  Node *pn = nodeFromIndex(parentIdx);
  switch (pn->type()) {
    case NodeType::Array: {
      auto tmp = static_cast<ArrayNode *>(pn);
      return createIndex(row, column,
                         static_cast<quintptr>(tmp->m_firstchildPos + row));
    }
    case NodeType::Object: {
      auto tmp = static_cast<ObjectNode *>(pn);
      return createIndex(row, column,
                         static_cast<quintptr>(tmp->m_firstchildPos + row));
    }
    default: {
      throw runtime_error("Expected an Array or Object");
      break;
    }
  }
}

QModelIndex JsonTreeViewModel::parent(const QModelIndex &childIdx) const {
  // qDebug() << "parent()" << childIdx;
  if (!childIdx.isValid()) return {};

  Node *cn = nodeFromIndex(childIdx);

  if (cn->m_parentPosition == MAX_U32) {  // root node
    return QModelIndex();                 // root node has invalid parent
  }
  Node *pn = m_fastJsonTree.node(cn->m_parentPosition);
  return createIndex(pn->m_rowInParent, 0,
                     static_cast<quintptr>(cn->m_parentPosition));
}

int JsonTreeViewModel::rowCount(const QModelIndex &parentIndex) const {
  // qDebug() << "rowCount()" << parentIndex;
  if (m_fastJsonTree.isEmpty()) return 0;

  Node *pn = nodeFromIndex(parentIndex);
  if (!pn) return 1;
  switch (pn->type()) {
    case NodeType::Array:
      return static_cast<ArrayNode *>(pn)->m_childCount;
    case NodeType::Object:
      return static_cast<ObjectNode *>(pn)->m_childCount;
    default:
      return 0;
  }
}

int JsonTreeViewModel::columnCount(const QModelIndex &parentIndex) const {
  return 2;
}

QVariant JsonTreeViewModel::data(const QModelIndex &idx, int role) const {
  // qDebug() << "data" << idx << role;
  struct Brushes {
    QBrush key;
    QBrush str;
    QBrush num;
    QBrush boolean;
    QBrush nullish;
    QBrush containerArray;
    QBrush containerObject;
    bool inited = false;
  };
  static Brushes B;

  if (!idx.isValid()) return {};

  Node *n = nodeFromIndex(idx);
  const bool is_key_col = (idx.column() == 0);

  switch (role) {
    case Qt::DisplayRole:
      return is_key_col ? QString::fromStdString(n->key())
                        : QString::fromStdString(n->value());

    case Qt::ForegroundRole: {
      if (!B.inited) {
        B.key = QColor(0, 87, 174);
        B.str = QColor(191, 3, 3);
        B.num = QColor(176, 128, 0);
        B.boolean = QColor(31, 28, 27);
        B.nullish = QColor(31, 28, 27);
        B.containerArray = QColor(0, 110, 40);
        B.containerObject = QColor(100, 74, 155);
        B.inited = true;
      }

      if (is_key_col) {
        return B.key;
      } else {
        switch (n->type()) {
          case NodeType::Str:
            return B.str;
          case NodeType::Num:
            return B.num;
          case NodeType::Bool:
            return B.boolean;
          case NodeType::Null:
            return B.nullish;
          case NodeType::Object:
            return B.containerObject;
          case NodeType::Array:
            return B.containerArray;
        }
      }
    }

    case Qt::FontRole: {
      if (is_key_col) {
        return m_baseFont;
      } else {
        QFont f = m_baseFont;
        switch (n->type()) {
          case NodeType::Null: {
            f.setItalic(true);
            f.setBold(true);
            break;
          }
          case NodeType::Bool: {
            f.setBold(true);
            break;
          }
          default: {
          }
        }
        return f;
      }
    }

    default:
      return {};
  }
  return {};
}

// bool LabelTreeModel::setData(const QModelIndex &idx, const QVariant
// &value,
//                              int role) {
//   if (!idx.isValid()) return false;
//   auto *n = static_cast<TreeNode *>(idx.internalPointer());
//   if (n->isRootNode()) return false;

//   if (role == Qt::CheckStateRole && !n->isRootNode()) {
//     Qt::CheckState checkState =
//     static_cast<Qt::CheckState>(value.toInt()); auto
//     &&oldCheckStateIter =
//     m_currentLabels[n->annType()].find(n->label());

//     if (oldCheckStateIter.value() != checkState) {
//       *oldCheckStateIter = checkState;
//       emit dataChanged(idx, idx, {role});
//       emit labelEnableChanged(n->annType(), n->label(),
//                               checkState == Qt::Checked);
//     }
//     return true;
//   }
//   return false;
// }

Qt::ItemFlags JsonTreeViewModel::flags(const QModelIndex &idx) const {
  if (!idx.isValid()) return Qt::NoItemFlags;
  Node *n = nodeFromIndex(idx);

  Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (n->type() != NodeType::Array && n->type() != NodeType::Object)
    f |= Qt::ItemNeverHasChildren;
  return f;
}

QVariant JsonTreeViewModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      if (section == 0)
        return "Key";
      else
        return "Value";
    }
    default:
      return {};
      break;
  }
}

Node *JsonTreeViewModel::nodeFromIndex(const QModelIndex &idx) const {
  if (!idx.isValid()) return nullptr;
  quint32 pos = static_cast<quint32>(idx.internalId());
  return m_fastJsonTree.node(pos);
}

void JsonTreeViewModel::setFontSize(const double pt) {
  if (qFuzzyCompare(pt, m_baseFont.pointSizeF()) == false) {
    m_baseFont.setPointSizeF(pt);
    const QModelIndex a = index(0, 0);
    const QModelIndex b = index(rowCount() - 1, columnCount() - 1);
    if (a.isValid() && b.isValid()) {
      emit dataChanged(a, b, {Qt::FontRole});
    }
  }
}
