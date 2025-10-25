#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHeaderView>
#include <QMainWindow>

class JsonTreeViewModel;
class ListViewSearchModel;

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

 private:
  void setZoom(double pt);
  Ui::MainWindow *ui;
  JsonTreeViewModel *m_jsonTreeModel{nullptr};
  ListViewSearchModel *m_searchListModel{nullptr};
  QHeaderView *m_headerView{};
  double m_basePt{};
  double m_currentPt{10.0};

  void addToRecentFiles(const QString &path);
  void updateRecentMenu();
  static constexpr int kMaxRecent = 16;
};
#endif  // MAINWINDOW_H
