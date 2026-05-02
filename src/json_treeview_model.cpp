#include "json_treeview_model.h"

#include <qprogressdialog.h>
#include <simdjson.h>

#include <QApplication>
#include <QBrush>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QIODevice>
#include <QIcon>
#include <QJsonArray>
#include <QWidget>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace {

QString prepareStr(const string &str) {
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  writer.String(str.data(), str.length());
  return QString::fromStdString(sb.GetString());
}

} // namespace

JsonTreeViewModel::JsonTreeViewModel(QObject *parent) : QAbstractItemModel(parent) {
  m_baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

  B.key = QColor(0, 87, 174);
  B.str = QColor(191, 3, 3);
  B.num = QColor(176, 128, 0);
  B.boolean = QColor(31, 28, 27);
  B.nullish = QColor(31, 28, 27);
  B.containerArray = QColor(0, 110, 40);
  B.containerObject = QColor(100, 74, 155);
}

bool JsonTreeViewModel::populateFromJson(const QString jsonFilePath, QString *errorMsg) {
  auto t1 = std::chrono::steady_clock::now();
  auto utf8_path = jsonFilePath.toUtf8();
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  if (const auto error = parser.load(utf8_path.constData()).get(doc); error) {
    QString msg;
    switch (error) {
    case simdjson::CAPACITY:
      msg = "This parser can't support a document that big";
      break;
    case simdjson::MEMALLOC:
      msg = "Error allocating memory, most likely out of memory";
      break;
    case simdjson::TAPE_ERROR:
      msg = "Something went wrong, this is a generic error. "
            "Fatal/unrecoverable error   ";
      break;
    case simdjson::DEPTH_ERROR:
      msg = "Your document exceeds the user-specified depth limitation";
      break;
    case simdjson::STRING_ERROR:
      msg = "Problem while parsing a string";
      break;
    case simdjson::T_ATOM_ERROR:
      msg = "Problem while parsing an atom starting with the letter 't'";
      break;
    case simdjson::F_ATOM_ERROR:
      msg = "Problem while parsing an atom starting with the letter 'f'";
      break;
    case simdjson::N_ATOM_ERROR:
      msg = "Problem while parsing an atom starting with the letter 'n'";
      break;
    case simdjson::NUMBER_ERROR:
      msg = "Problem while parsing a number";
      break;
    case simdjson::BIGINT_ERROR:
      msg = "The integer value exceeds 64 bits";
      break;
    case simdjson::UTF8_ERROR:
      msg = "The input is not valid UTF-8";
      break;
    case simdjson::UNINITIALIZED:
      msg = "Unknown error, or uninitialized document";
      break;
    case simdjson::EMPTY:
      msg = "No structural element found";
      break;
    case simdjson::UNESCAPED_CHARS:
      msg = "Found unescaped characters in a string";
      break;
    case simdjson::UNCLOSED_STRING:
      msg = "Missing quote at the end of a string";
      break;
    case simdjson::UNSUPPORTED_ARCHITECTURE:
      msg = "Unsupported architecture";
      break;
    case simdjson::INCORRECT_TYPE:
      msg = "JSON element has a different type than user expected";
      break;
    case simdjson::NUMBER_OUT_OF_RANGE:
      msg = "JSON number does not fit in 64 bits";
      break;
    case simdjson::INDEX_OUT_OF_BOUNDS:
      msg = "JSON array index too large";
      break;
    case simdjson::NO_SUCH_FIELD:
      msg = "JSON field not found in object";
      break;
    case simdjson::IO_ERROR:
      msg = "Error reading a file";
      break;
    case simdjson::INVALID_JSON_POINTER:
      msg = "Invalid JSON pointer syntax";
      break;
    case simdjson::INVALID_URI_FRAGMENT:
      msg = "Invalid URI fragment";
      break;
    case simdjson::UNEXPECTED_ERROR:
      msg = "Indicative of a bug in simdjson";
      break;
    case simdjson::PARSER_IN_USE:
      msg = "Parser is already in use.";
      break;
    case simdjson::OUT_OF_ORDER_ITERATION:
      msg = "Tried to iterate an array or object out of order (checked when "
            "SIMDJSON_DEVELOPMENT_CHECKS=1)";
      break;
    case simdjson::INSUFFICIENT_PADDING:
      msg = "The JSON doesn't have enough padding for simdjson to safely parse "
            "it.";
      break;
    case simdjson::INCOMPLETE_ARRAY_OR_OBJECT:
      msg = "The document ends early. Fatal/unrecoverable error.";
      break;
    case simdjson::SCALAR_DOCUMENT_AS_VALUE:
      msg = "A scalar document is treated as a value.";
      break;
    case simdjson::OUT_OF_BOUNDS:
      msg = "Attempted to access location outside of document.";
      break;
    case simdjson::TRAILING_CONTENT:
      msg = "Unexpected trailing content in the JSON input";
      break;
    case simdjson::OUT_OF_CAPACITY:
      msg = "The capacity was exceeded, we cannot allocate enough memory.";
    default:
      msg = "Unknown error";
      break;
    }

    qWarning() << "Error parsing JSON file:" << jsonFilePath << "\n" << msg;
    if (errorMsg)
      errorMsg->swap(msg);
    return false;
  }

  beginResetModel();

  m_fastJsonTree.clear();
  m_fastJsonTree.buildTree(doc);

  endResetModel();
  auto t2 = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
  qDebug() << "Load JSON time: " << ms.count() << " ms";
  return true;
}

JsonTreeViewModel::~JsonTreeViewModel() {
  m_fastJsonTree.clear();
}

QModelIndex JsonTreeViewModel::index(int row, int column, const QModelIndex &parentIdx) const {
  // qDebug() << "index()" << row << column << parentIdx;
  if (column < 0 || row < 0 || m_fastJsonTree.isEmpty())
    return {};
  if (!parentIdx.isValid()) {
    return createIndex(row, column, static_cast<quintptr>(0));
  }
  quint32 pn_idx = nodeFromIndex(parentIdx);
  quint32 n = m_fastJsonTree.firstChild(pn_idx) + row;
  return createIndex(row, column, static_cast<quintptr>(n));
}

QModelIndex JsonTreeViewModel::parent(const QModelIndex &childIdx) const {
  // qDebug() << "parent()" << childIdx;
  if (!childIdx.isValid())
    return {};

  quint32 cn_idx = nodeFromIndex(childIdx);
  quint32 pn_idx = m_fastJsonTree.parent(cn_idx);

  if (pn_idx == MAX_U32) { // root node
    return QModelIndex();  // root node has invalid parent
  }
  return createIndex(m_fastJsonTree.rowInParent(pn_idx), 0, static_cast<quintptr>(pn_idx));
}

int JsonTreeViewModel::rowCount(const QModelIndex &parentIndex) const {
  // qDebug() << "rowCount()" << parentIndex;
  if (m_fastJsonTree.isEmpty())
    return 0;

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

  if (!idx.isValid())
    return {};

  const quint32 n_idx = nodeFromIndex(idx);
  // qDebug() << "data()" << n_idx << m_fastJsonTree.key(n_idx) <<
  // m_fastJsonTree.valueAsStr(n_idx);
  const bool is_key_col = (idx.column() == 0);

  switch (role) {
  case Qt::DisplayRole:
    return is_key_col ? QString::fromStdString(m_fastJsonTree.key(n_idx) + ":")
                      : QString::fromStdString(m_fastJsonTree.valueAsStr(n_idx));

  case Qt::ForegroundRole: {
    if (is_key_col) {
      return B.key;
    } else {
      switch (m_fastJsonTree.type(n_idx)) {
      case NodeType::Str:
        return B.str;
      case NodeType::NumFloat:
      case NodeType::NumInt:
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
  if (!idx.isValid())
    return Qt::NoItemFlags;
  quint32 n_idx = nodeFromIndex(idx);

  Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  NodeType type = m_fastJsonTree.type(n_idx);
  if (type != NodeType::Array && type != NodeType::Object)
    f |= Qt::ItemNeverHasChildren;
  return f;
}

QVariant JsonTreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
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
  case NodeType::NumFloat:
    return QString::number(m_fastJsonTree.value(n).toDouble(), 'g', 12);
  case NodeType::NumInt:
    return QString::number(m_fastJsonTree.value(n).toLongLong());
  case NodeType::Bool:
    return QString("%1").arg(m_fastJsonTree.value(n).toBool());
  default:
    return "null";
  }
}

NodeType JsonTreeViewModel::nodeType(const QModelIndex &idx) const {
  return m_fastJsonTree.type(nodeFromIndex(idx));
}

quint32 JsonTreeViewModel::subTreeNodesCount(const QModelIndex &idx) const {
  quint32 n = nodeFromIndex(idx);
  switch (m_fastJsonTree.type(n)) {
  case NodeType::Array:
  case NodeType::Object:
    return m_fastJsonTree.m_nodeValueOrPos[n].index; // this stores the total number of subnodes
  default:
    return 0;
  }
}

quint32 JsonTreeViewModel::nodeFromIndex(const QModelIndex &idx) const {
  if (!idx.isValid())
    return MAX_U32;
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

void JsonTreeViewModel::writeNode(quint32 node, QTextStream &stream) {
  switch (m_fastJsonTree.type(node)) {
  case NodeType::Null: {
    stream << "null";
    break;
  }
  case NodeType::Bool: {
    const QString str = m_fastJsonTree.m_nodeValueOrPos[node].index ? "false" : "true";
    stream << str;
    break;
  }
  case NodeType::NumFloat: {
    stream << m_fastJsonTree.m_nodeValueOrPos[node].num;
    break;
  }

  case NodeType::NumInt: {
    stream << m_fastJsonTree.m_nodeValueOrPos[node].index;
    break;
  }

  case NodeType::Str: {
    const string str = m_fastJsonTree.m_nodeStringValueTable[m_fastJsonTree.m_nodeValueOrPos[node].index];
    stream << prepareStr(str);
    break;
  }

  case NodeType::Array: {
    stream << '[';
    const quint32 childCount = m_fastJsonTree.m_childCount[node];
    const quint32 firstChild = m_fastJsonTree.m_firstChildPosition[node];
    for (quint32 row = 0; row < childCount; ++row) {
      if (row) {
        stream << ',';
      }
      writeNode(firstChild + row, stream);
    }
    stream << ']';
    break;
  }

  case NodeType::Object: {
    stream << '{';
    const quint32 childCount = m_fastJsonTree.m_childCount[node];
    const quint32 firstChild = m_fastJsonTree.m_firstChildPosition[node];
    for (quint32 row = 0; row < childCount; ++row) {
      if (row) {
        stream << ',';
      }
      const quint32 n = firstChild + row;
      stream << prepareStr(m_fastJsonTree.m_nodeKeyTable[m_fastJsonTree.m_nodeKeyPos[n]]) + ":";
      writeNode(n, stream);
    }
    stream << '}';
  }
  }
}

bool JsonTreeViewModel::exportToJsonFile(const QModelIndex &rootIndex, const QString &outputFilePath) {

  quint32 n = nodeFromIndex(rootIndex);
  NodeType t = m_fastJsonTree.type(n);
  const bool nodeIsContainer = t == NodeType::Object || t == NodeType::Array;
  if (!nodeIsContainer) {
    qWarning() << "The root node must be a container, e.g.: List or Dict";
    return false;
  }

  QFile outputFile(outputFilePath);
  if (!outputFile.open(QIODevice::WriteOnly)) {
    qWarning() << "Could not open JSON export file:" << outputFilePath << outputFile.errorString();
    return false;
  }
  QTextStream stream(&outputFile);
  writeNode(n, stream);
  return true;
}
