#include "mainwindow.h"

#include <QClipboard>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QtConcurrent/QtConcurrent>

#include "QJsonDocument"
#include "json_treeview_model.h"
#include "list_view_search_model.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setFocusPolicy(Qt::StrongFocus);

  // tree view
  ui->treeView->setModel(m_jsonTreeModel = new JsonTreeViewModel(this));
  ui->treeView->setUniformRowHeights(true);
  ui->treeView->setAnimated(false);
  ui->treeView->setSortingEnabled(false);
  ui->treeView->setExpandsOnDoubleClick(true);
  ui->treeView->setAllColumnsShowFocus(false);

  ui->treeView->setAlternatingRowColors(true);
  ui->treeView->setIndentation(16);
  ui->treeView->header()->setStretchLastSection(true);
  ui->treeView->setFont(qApp->font());
  m_basePt = ui->treeView->font().pointSizeF(); // remember starting font
  m_currentPt = m_basePt > 0 ? m_basePt : 12.0;
  m_headerView = ui->treeView->header();

  ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

  // search list view
  ui->listViewSearch->setModel(m_searchListModel = new ListViewSearchModel(m_jsonTreeModel, this));
  ui->listViewSearch->setAlternatingRowColors(true);
  connect(ui->lineEditSearchText, &QLineEdit::returnPressed, ui->pushButtonSearch, &QPushButton::click);

  // set the font size for models
  m_jsonTreeModel->setFontSize(m_currentPt);
  m_searchListModel->setFontSize(m_currentPt);

  // for recent files
  updateRecentMenu();

  // search
  on_comboBoxSearchMode_currentIndexChanged(0);
  connect(m_searchListModel, &ListViewSearchModel::modelReset, ui->pushButtonSearch, [this]() {
    // ui->pushButtonSearch->setEnabled(true);
    const int n = m_searchListModel->rowCount({});
    // ui->listViewSearch->setVisible(n != 0);
    ui->labelSearchResults->setText(QString("Results: %1").arg(n));
  });
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::on_actionOpen_JSON_triggered() {
  QString jsonFilePath = QFileDialog::getOpenFileName(this, "Json file");
  if (jsonFilePath.isEmpty())
    return;
  loadJsonFile(jsonFilePath);
}

void MainWindow::openRecent() {
  auto *a = qobject_cast<QAction *>(sender());
  if (!a)
    return;
  const QString path = a->data().toString();
  if (path.isEmpty())
    return;
  loadJsonFile(path);
}

void MainWindow::loadJsonFile(const QString &jsonFilePath) {
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString errMsg;

  if (m_jsonTreeModel->populateFromJson(jsonFilePath, &errMsg)) {
    addToRecentFiles(jsonFilePath);
    // QFont fm = font();
    const QString fileName = QFileInfo(jsonFilePath).fileName().toUpper();
    setWindowTitle(QString("QJsonViewer [ %1 ]").arg(fileName));
    clearCurrentNodeInfo();
    m_searchListModel->clear();
    ui->lineEditSearchText->clear();
    QApplication::restoreOverrideCursor();
    return;
  }
  QApplication::restoreOverrideCursor();
  QMessageBox::warning(this, "Error parsing input file", errMsg);
}

void MainWindow::setZoom(double pt) {
  pt = std::clamp(pt, 8.0, 28.0); // clamp to a sensible range
  if (qFuzzyCompare(pt, m_currentPt))
    return;
  m_currentPt = pt;
  m_jsonTreeModel->setFontSize(m_currentPt);
  m_searchListModel->setFontSize(m_currentPt);
}

void MainWindow::on_actionZoom_In_triggered() {
  setZoom(m_currentPt * 1.10);
}

void MainWindow::on_actionZoom_Out_triggered() {
  setZoom(m_currentPt / 1.10);
}

void MainWindow::on_treeView_clicked(const QModelIndex &index) {
  if (ui->dockWidgetInfo->isVisible()) {
    ui->leNodeKey->setText(m_jsonTreeModel->nodeKey(index));
    ui->leNodePath->setText(m_jsonTreeModel->nodePath(index));
    ui->leNodeValue->setPlainText(m_jsonTreeModel->nodeValueStr(index));

    switch (m_jsonTreeModel->nodeType(index)) {
    case NodeType::Array: {
      int rows = m_jsonTreeModel->rowCount(index);
      ui->lineEditNodeType->setText(QString("Array (children: %1)").arg(rows));
      break;
    }
    case NodeType::Object: {
      int rows = m_jsonTreeModel->rowCount(index);
      ui->lineEditNodeType->setText(QString("Object (children: %1)").arg(rows));
      break;
    }
    case NodeType::Null:
      ui->lineEditNodeType->setText("Null");
      break;
    case NodeType::NumInt:
    case NodeType::NumFloat:
      ui->lineEditNodeType->setText("Number");
      break;
    case NodeType::Bool:
      ui->lineEditNodeType->setText("Bool");
      break;
    case NodeType::Str:
      ui->lineEditNodeType->setText("String");
      break;
    }
  }
}

void MainWindow::on_pushButtonSearch_clicked() {
  auto mode = static_cast<ListViewSearchModel::SearchMode>(ui->comboBoxSearchMode->currentIndex());

  QString searchText = ui->lineEditSearchText->text();
  const bool wholeWord = ui->checkBoxValueWW->isChecked();
  const bool caseSensitive = ui->checkBoxValueCS->isChecked();

  switch (mode) {
  case ListViewSearchModel::SearchMode::ValueBool:
    searchText = ui->comboBoxBoolTF->currentText();
    break;

  case ListViewSearchModel::SearchMode::ValueNum: {
    bool ok;
    std::ignore = searchText.toDouble(&ok);
    if (!ok) {
      QMessageBox::warning(this, "Wrong search text", "Text pattern must be convertible to a number");
      return;
    }
    break;
  }

  case ListViewSearchModel::SearchMode::ValueStr:
  case ListViewSearchModel::SearchMode::Key:
  case ListViewSearchModel::SearchMode::ValueAny:
  case ListViewSearchModel::SearchMode::KeyOrValue:
    if (wholeWord == false && searchText.isEmpty()) {
      QMessageBox::warning(this, "Wrong search text", "Empty text is not supported in this search mode");
      return;
    }
    break;

  default:
    break;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_searchListModel->doSearch(mode, searchText, caseSensitive, wholeWord, false);
  QApplication::restoreOverrideCursor();
}

void MainWindow::on_listViewSearch_clicked(const QModelIndex &index) {
  auto idx = m_searchListModel->toTreeView(index);
  if (!idx.isValid())
    return;
  QModelIndex p = idx.parent();
  while (p.isValid()) {
    ui->treeView->expand(p);
    p = p.parent();
  }
  ui->treeView->setCurrentIndex(idx);
  ui->treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
}

void MainWindow::addToRecentFiles(const QString &path) {
  QSettings s; // make sure QCoreApplication::setOrganizationName/AppName set
               // at startup
  QStringList list = s.value("recentFiles").toStringList();

  list.removeAll(path); // de-dup
  list.prepend(path);   // most recent first

  // prune non-existing + cap
  QStringList cleaned;
  cleaned.reserve(kMaxRecent);
  for (auto &&p : list) {
    if (QFileInfo::exists(p)) {
      cleaned.push_back(p);
      if (cleaned.size() == kMaxRecent)
        break;
    }
  }
  s.setValue("recentFiles", cleaned);
  updateRecentMenu();
}

void MainWindow::updateRecentMenu() {
  QSettings s;
  QStringList list = s.value("recentFiles").toStringList();

  auto recent_actions = ui->menuRecent_files->actions();
  const int nr = list.size();
  QFontMetrics fm(font());
  const int maxTextPx = 1024; // tweak: elide long paths
  int i = 0;
  for (i = 0; i < nr; ++i) {
    QAction *a;
    if (i < recent_actions.size()) { // use existing action
      a = recent_actions[i];
    } else {
      a = new QAction(this);
      connect(a, &QAction::triggered, this, &MainWindow::openRecent);
      ui->menuRecent_files->addAction(a);
    }
    const QString path = list.at(i);
    const QString basename = QFileInfo(path).baseName();
    const QString shown = fm.elidedText(path, Qt::ElideMiddle, maxTextPx);
    a->setText(QString::fromLatin1("&%1 [%2]  %3").arg(i + 1).arg(basename, shown));
    a->setData(path);
  }
  for (; i < recent_actions.size(); ++i) {
    ui->menuRecent_files->removeAction(recent_actions[i]);
  }
}

void MainWindow::on_comboBoxSearchMode_currentIndexChanged(int index) {
  auto mode = static_cast<ListViewSearchModel::SearchMode>(index);
  bool txt_search = true;
  bool null_search = false;
  bool bool_search = false;
  bool num_search = false;
  switch (mode) {
  case ListViewSearchModel::SearchMode::ValueNum: {
    num_search = true;
    txt_search = true;
    break;
  }
  case ListViewSearchModel::SearchMode::ValueBool: {
    bool_search = true;
    txt_search = false;
    break;
  }
  case ListViewSearchModel::SearchMode::ValueNull: {
    // null_search = true;
    txt_search = false;
    break;
  }
  default:
    break;
  }
  ui->lineEditSearchText->setVisible(txt_search);
  ui->comboBoxBoolTF->setVisible(bool_search);
  ui->checkBoxValueCS->setVisible(txt_search && !num_search);
  ui->checkBoxValueWW->setVisible(txt_search && !num_search);
}

void MainWindow::clearCurrentNodeInfo() {
  ui->leNodeKey->clear();
  ui->leNodePath->clear();
  ui->leNodeValue->clear();
  ui->lineEditNodeType->clear();
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos) {
  const QModelIndex index = ui->treeView->indexAt(pos);
  //  const QPoint global_point = ui->treeView->mapToGlobal(pos);

  QMenu menu(ui->treeView);
  setupTreeViewMenu(menu, index);
  if (menu.actions().size())
    menu.exec(QCursor::pos());
}

void MainWindow::setupTreeViewMenu(QMenu &menu, const QModelIndex &index) {
  NodeType t = m_jsonTreeModel->nodeType(index);
  QAction *act{nullptr};

  const bool nodeIsContainer = t == NodeType::Object || t == NodeType::Array;
  if (nodeIsContainer) {
    quint32 subnodes = m_jsonTreeModel->subTreeNodesCount(index);
    quint32 rows = m_jsonTreeModel->rowCount(index);

    if (subnodes <= kMaxExpandCollapseNodesCount || subnodes == rows) {
      act = menu.addAction("Collapse all");
      connect(act, &QAction::triggered, ui->treeView, [&]() { collapseRecursively(ui->treeView, index); });

      act = menu.addAction("Expand all");
      connect(act, &QAction::triggered, this, [&]() { ui->treeView->expandRecursively(index); });
    }

    act = menu.addAction("Export to file");
    connect(act, &QAction::triggered, this, [&]() { this->exportToJsonFile(index); });
  }

  if (!nodeIsContainer) {
    act = menu.addAction("Copy Key");
    connect(act, &QAction::triggered, this, [&]() {
      QString key_text = m_jsonTreeModel->nodeKey(index);
      QClipboard *clipboard = QGuiApplication::clipboard();
      clipboard->setText(key_text);
    });

    act = menu.addAction("Copy Value");
    connect(act, &QAction::triggered, this, [&]() {
      QString key_text = m_jsonTreeModel->nodeValueStr(index);
      QClipboard *clipboard = QGuiApplication::clipboard();
      clipboard->setText(key_text);
    });
  }
}

void MainWindow::collapseRecursively(QTreeView *v, const QModelIndex &root) {
  static int max_level = 2;
  v->setExpanded(root, false);

  const int rows = m_jsonTreeModel->rowCount(root);
  for (int r = 0; r < rows; ++r) {
    auto idx = m_jsonTreeModel->index(r, 0, root);
    collapseRecursively(v, idx);
  }
}

void MainWindow::expandRecursively(QTreeView *v, const QModelIndex &root, int lev) {
  static int max_level = 2;
  if (lev > max_level)
    return;
  v->setExpanded(root, true);

  const int rows = m_jsonTreeModel->rowCount(root);
  for (int r = 0; r < rows; ++r) {
    auto idx = m_jsonTreeModel->index(r, 0, root);
    expandRecursively(v, idx, lev + 1);
  }
}

void MainWindow::exportToJsonFile(const QModelIndex &rootIndex) {
  const QString jsonOutputFile = QFileDialog::getSaveFileName(this, "Export JSON file");
  if (jsonOutputFile.isEmpty()) {
    return;
  }
  m_jsonTreeModel->exportToJsonFile(rootIndex, jsonOutputFile);
}