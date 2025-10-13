#include "mainwindow.h"

#include <QFileDialog>
#include <QJsonObject>
#include <QProgressDialog>

#include "QJsonDocument"
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

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
  setFocusPolicy(Qt::StrongFocus);

  m_basePt = ui->treeView->font().pointSizeF();  // remember starting font
  m_currentPt = m_basePt > 0 ? m_basePt : 10.0;
  m_headerView = ui->treeView->header();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_actionOpen_JSON_triggered() {
  QString jsonFilePath = QFileDialog::getOpenFileName(this, "Json file");
  if (jsonFilePath.isEmpty()) return;
  loadJsonFile(jsonFilePath);
}

void MainWindow::loadJsonFile(const QString &jsonFilePath) {
  QFile ifile(jsonFilePath);
  if (!ifile.open(QFile::ReadOnly)) {
    qDebug() << "Failed reading annotations file: " << jsonFilePath;
    return;
  }

  QProgressDialog progress("Loading Json file", "Cancel", 0, 0, this);
  // progress.setMinimumDuration(0);
  progress.setWindowModality(Qt::WindowModal);

  const QByteArray json = ifile.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(json);

  progress.setLabelText("Creating JSON tree...");
  m_jsonTreeModel->populateFromJson(doc, progress);
}

void MainWindow::setZoom(double pt) {
  pt = std::clamp(pt, 8.0, 28.0);  // clamp to a sensible range
  if (qFuzzyCompare(pt, m_currentPt)) return;
  m_currentPt = pt;

  QFont f = m_headerView->font();
  f.setPointSizeF(m_currentPt);
  // ui->treeView->setFont(f);
  m_headerView->setFont(f);

  m_jsonTreeModel->setFontSize(m_currentPt);
  // Ask view to recompute row heights & layout after font change
  // ui->treeView->doItemsLayout();
  // ui->treeView->viewport()->update();

  // Optionally resize columns to contents after a zoom step (can be heavy on
  // huge models) view_->resizeColumnToContents(0);
  // view_->resizeColumnToContents(1);
}

void MainWindow::on_actionZoom_In_triggered() { setZoom(m_currentPt * 1.10); }

void MainWindow::on_actionZoom_Out_triggered() { setZoom(m_currentPt / 1.10); }
