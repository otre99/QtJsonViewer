#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHeaderView>
#include <QMainWindow>

class JsonTreeViewModel;
class ListViewSearchModel;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QTreeView;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  void loadJsonFile(const QString &jsonFilePath);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void on_actionOpen_JSON_triggered();
  void on_actionZoom_In_triggered();
  void on_actionZoom_Out_triggered();
  void on_treeView_clicked(const QModelIndex &index);
  void on_pushButtonSearch_clicked();
  void on_listViewSearch_clicked(const QModelIndex &index);
  void openRecent();
  void on_comboBoxSearchMode_currentIndexChanged(int index);
  void clearCurrentNodeInfo();
  void on_treeView_customContextMenuRequested(const QPoint &pos);

private:
  void setZoom(double pt);
  void setupTreeViewMenu(QMenu &menu, const QModelIndex &index);
  void collapseRecursively(QTreeView *v, const QModelIndex &root);
  void expandRecursively(QTreeView *v, const QModelIndex &root, int lev = 0);
  void exportToJsonFile(const QModelIndex &rootIndex);

  Ui::MainWindow *ui;
  JsonTreeViewModel *m_jsonTreeModel{nullptr};
  ListViewSearchModel *m_searchListModel{nullptr};
  QHeaderView *m_headerView{};
  double m_basePt{};
  double m_currentPt{10.0};

  void addToRecentFiles(const QString &path);
  void updateRecentMenu();
  static constexpr int kMaxRecent = 16;
  static constexpr int kMaxExpandCollapseNodesCount = 1 << 20;
};
#endif // MAINWINDOW_H
