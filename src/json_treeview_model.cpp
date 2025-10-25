#include "json_treeview_model.h"

#include <qprogressdialog.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <QApplication>
#include <QBrush>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonArray>
#include <QWidget>
#include <fstream>

using rapidjson::Document;
using rapidjson::IStreamWrapper;

JsonTreeViewModel::JsonTreeViewModel(QObject *parent)
    : QAbstractItemModel(parent) {
  m_baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

  B.key = QColor(0, 87, 174);
  B.str = QColor(191, 3, 3);
  B.num = QColor(176, 128, 0);
  B.boolean = QColor(31, 28, 27);
  B.nullish = QColor(31, 28, 27);
  B.containerArray = QColor(0, 110, 40);
  B.containerObject = QColor(100, 74, 155);
}

bool JsonTreeViewModel::populateFromJson(const QString jsonFilePath) {
  auto utf8_path = jsonFilePath.toUtf8();
  std::ifstream ifs(utf8_path.constData(), std::ios::binary);
  if (!ifs.is_open()) {
    return false;
  }

  IStreamWrapper isw(ifs);
  Document doc;
  // You can add flags like kParseCommentsFlag, kParseTrailingCommasFlag if
  // needed
  doc.ParseStream(isw);
  if (doc.HasParseError()) {
    return false;
  }

  beginResetModel();

  m_fastJsonTree.clear();
  m_fastJsonTree.buildTree(&doc);

  endResetModel();
  return true;
}

JsonTreeViewModel::~JsonTreeViewModel() { m_fastJsonTree.clear(); }

QModelIndex JsonTreeViewModel::index(int row, int column,
                                     const QModelIndex &parentIdx) const {
  // qDebug() << "index()" << row << column << parentIdx;
  if (column < 0 || row < 0 || m_fastJsonTree.isEmpty()) return {};
  if (!parentIdx.isValid()) {
    return createIndex(row, column, static_cast<quintptr>(0));
  }
  quint32 pn_idx = nodeFromIndex(parentIdx);
  quint32 n = m_fastJsonTree.firstChild(pn_idx) + row;
  return createIndex(row, column, static_cast<quintptr>(n));
}

QModelIndex JsonTreeViewModel::parent(const QModelIndex &childIdx) const {
  // qDebug() << "parent()" << childIdx;
  if (!childIdx.isValid()) return {};

  quint32 cn_idx = nodeFromIndex(childIdx);
  quint32 pn_idx = m_fastJsonTree.parent(cn_idx);

  if (pn_idx == MAX_U32) {  // root node
    return QModelIndex();   // root node has invalid parent
  }
  return createIndex(m_fastJsonTree.rowInParent(pn_idx), 0,
                     static_cast<quintptr>(pn_idx));
}

int JsonTreeViewModel::rowCount(const QModelIndex &parentIndex) const {
  // qDebug() << "rowCount()" << parentIndex;
  if (m_fastJsonTree.isEmpty()) return 0;

  quint32 pn_idx = nodeFromIndex(parentIndex);
  if (pn_idx == MAX_U32) {
    return 1;
  }
  return m_fastJsonTree.rows(pn_idx);
}

int JsonTreeViewModel::columnCount(const QModelIndex &parentIndex) const {
  return 2;
}

QVariant JsonTreeViewModel::data(const QModelIndex &idx, int role) const {
  // qDebug() << "data" << idx << role;

  if (!idx.isValid()) return {};

  const quint32 n_idx = nodeFromIndex(idx);
  // qDebug() << "data()" << n_idx << m_fastJsonTree.key(n_idx) <<
  // m_fastJsonTree.valueAsStr(n_idx);
  const bool is_key_col = (idx.column() == 0);

  switch (role) {
    case Qt::DisplayRole:
      return is_key_col
                 ? QString::fromStdString(m_fastJsonTree.key(n_idx) + ":")
                 : m_fastJsonTree.value(n_idx);

    case Qt::ForegroundRole: {
      if (is_key_col) {
        return B.key;
      } else {
        switch (m_fastJsonTree.type(n_idx)) {
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
        switch (m_fastJsonTree.type(n_idx)) {
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

Qt::ItemFlags JsonTreeViewModel::flags(const QModelIndex &idx) const {
  if (!idx.isValid()) return Qt::NoItemFlags;
  quint32 n_idx = nodeFromIndex(idx);

  Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  NodeType type = m_fastJsonTree.type(n_idx);
  if (type != NodeType::Array && type != NodeType::Object)
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

QString JsonTreeViewModel::nodePath(const QModelIndex &idx) const {
  return m_fastJsonTree.nodePath(nodeFromIndex(idx)).c_str();
}

QString JsonTreeViewModel::nodeKey(const QModelIndex &idx) const {
  return m_fastJsonTree.key(nodeFromIndex(idx)).c_str();
}
QString JsonTreeViewModel::nodeValueStr(const QModelIndex &idx) const {
  quint32 n = nodeFromIndex(idx);
  switch (m_fastJsonTree.type(n)) {
    case NodeType::Array:
    case NodeType::Object:
    case NodeType::Str:
      return m_fastJsonTree.value(n).toString();
    case NodeType::Num:
      return QString("%1").arg(m_fastJsonTree.value(n).toDouble());
    case NodeType::Bool:
      return QString("%1").arg(m_fastJsonTree.value(n).toBool());
    default:
      return "null";
  }
}

NodeType JsonTreeViewModel::nodeType(const QModelIndex &idx) const {
  return m_fastJsonTree.type(nodeFromIndex(idx));
}

quint32 JsonTreeViewModel::nodeFromIndex(const QModelIndex &idx) const {
  if (!idx.isValid()) return MAX_U32;
  return static_cast<quint32>(idx.internalId());
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
