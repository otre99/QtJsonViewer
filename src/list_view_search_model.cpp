#include <execution>
#include "list_view_search_model.h"


#include <algorithm>

#include "json_treeview_model.h"
#include "text_matcher.h"

ListViewSearchModel::ListViewSearchModel(JsonTreeViewModel *lm, QObject *parent)
    : m_jsonTreeViewModel{lm}, QAbstractListModel{parent} {}

int ListViewSearchModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_validNodes.size();
}

int ListViewSearchModel::columnCount(const QModelIndex &parent) const {
  return 2;
}

QVariant ListViewSearchModel::data(const QModelIndex &idx, int role) const {
  if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_validNodes.size()) {
    return {};
  }

  quint32 nidx = m_validNodes[idx.row()];
  switch (role) {
  case Qt::DisplayRole:
    return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // switch (m_jsonTreeViewModel->m_fastJsonTree.type(nidx)) {
    // case NodeType::Array:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::Object:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::NumFloat:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::NumInt:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::Str:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::Bool:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // case NodeType::Null:
    //   return QString::fromStdString(m_jsonTreeViewModel->m_fastJsonTree.nodePreview(nidx));
    // }
    // case Qt::ForegroundRole:
    //   switch (m_jsonTreeViewModel->m_fastJsonTree.type(nidx)) {
    //     case NodeType::Str:
    //       return m_jsonTreeViewModel->B.str;
    //     case NodeType::Num:
    //       return m_jsonTreeViewModel->B.num;
    //     case NodeType::Bool:
    //       return m_jsonTreeViewModel->B.boolean;
    //     case NodeType::Null:
    //       return m_jsonTreeViewModel->B.nullish;
    //     case NodeType::Object:
    //       return m_jsonTreeViewModel->B.containerObject;
    //     case NodeType::Array:
    //       return m_jsonTreeViewModel->B.containerArray;
    //   }

  case Qt::FontRole:
    return m_baseFont;

  default:
    return {};
  }
}

void ListViewSearchModel::doSearch(SearchMode mode, const QString &searchText, bool caseSensitive, bool wholeWord,
                                   bool useRegex) {
  vector<unsigned char> matches(m_jsonTreeViewModel->m_fastJsonTree.nodesCount());

  const vector<quint32> &p = m_jsonTreeViewModel->m_fastJsonTree.m_parentPosition;

  // Build text matcher for modes that need text
  TextMatcher textMatcher(searchText.toStdString(), caseSensitive, wholeWord, useRegex);

  // Parse number/bool once
  double snum = 0.0;
  if (mode == SearchMode::ValueNum) {
    bool ok = false;
    snum = searchText.toDouble(&ok);
  }

  bool sbool = false;
  if (mode == SearchMode::ValueBool) {
    const QString s = searchText.trimmed().toLower();
    sbool = s == "true";
  }

  // Numeric tolerance (tweak or expose in UI)
  const double epsAbs = 1e-9; // absolute tol
  const double epsRel = 1e-6; // relative tol

  auto numEqual = [&](double a, double b) {
    const double diff = std::fabs(a - b);
    if (diff <= epsAbs)
      return true;
    const double scale = std::max({1.0, std::fabs(a), std::fabs(b)});
    return diff <= epsRel * scale;
  };

  const FastJsonTree &ft = m_jsonTreeViewModel->m_fastJsonTree;

  for_each(execution::par_unseq, p.cbegin(), p.cend(),
           [&](const quint32 &pn) {
             const quint32 nodeId = static_cast<quint32>(&pn - p.data());

             switch (mode) {
             case SearchMode::KeyOrValue:
               matches[nodeId] = textMatcher.match(ft.key(nodeId)) || textMatcher.match(ft.valueAsStr(nodeId));
               break;

             case SearchMode::Key:
               matches[nodeId] = textMatcher.match(ft.key(nodeId));
               break;

             case SearchMode::ValueAny:
               matches[nodeId] = textMatcher.match(ft.valueAsStr(nodeId));
               break;

             case SearchMode::ValueStr:
               matches[nodeId] = (ft.type(nodeId) == NodeType::Str) && textMatcher.match(ft.valueAsStr(nodeId));
               break;

             case SearchMode::ValueBool:
               if (ft.type(nodeId) == NodeType::Bool) {
                 const bool v = bool(ft.m_nodeValueOrPos[nodeId].index); // prefer an accessor
                 matches[nodeId] = (v == sbool);
               }
               break;

             case SearchMode::ValueNum: {

               const auto t = ft.type(nodeId);
               if (t == NodeType::NumFloat) {
                 const double v = ft.m_nodeValueOrPos[nodeId].num; // prefer an accessor
                 matches[nodeId] = numEqual(v, snum);
               }
               if (t == NodeType::NumInt) {
                 const quint64 v = ft.m_nodeValueOrPos[nodeId].index; // prefer an accessor
                 matches[nodeId] = numEqual(v, snum);
               }

               break;
             }

             case SearchMode::ValueNull:
               matches[nodeId] = (ft.type(nodeId) == NodeType::Null);
               break;
             }
           }

  );

  m_validNodes.clear();
  beginResetModel();
  for (quint32 i = 0; i < matches.size(); ++i) {
    if (matches[i])
      m_validNodes.push_back(i);
  }
  qDebug() << "Found " << m_validNodes.size();
  endResetModel();
}

QModelIndex ListViewSearchModel::toTreeView(const QModelIndex &idx) const {
  if (idx.isValid()) {
    quint32 n = m_validNodes[idx.row()];
    quint32 pn = m_jsonTreeViewModel->m_fastJsonTree.parent(n);
    qDebug() << "PARENT" << pn << m_jsonTreeViewModel->m_fastJsonTree.key(pn);

    int row = m_jsonTreeViewModel->m_fastJsonTree.rowInParent(n);
    return m_jsonTreeViewModel->createIndex(row, 0, static_cast<quintptr>(n));
  }
  return {};
}

void ListViewSearchModel::clear() {
  beginResetModel();
  m_validNodes.clear();
  endResetModel();
}

void ListViewSearchModel::setFontSize(const double pt) {
  if (qFuzzyCompare(pt, m_baseFont.pointSizeF()) == false) {
    m_baseFont.setPointSizeF(pt);
    const QModelIndex a = index(0, 0);
    const QModelIndex b = index(rowCount() - 1, 0);
    if (a.isValid() && b.isValid()) {
      emit dataChanged(a, b, {Qt::FontRole});
    }
  }
}
