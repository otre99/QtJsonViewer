#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHeaderView>
#include <QMainWindow>

#include "json_treeview_model.h"

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

 private:
  void setZoom(double pt);
  Ui::MainWindow *ui;
  JsonTreeViewModel *m_jsonTreeModel{nullptr};
  QHeaderView *m_headerView{};
  double m_basePt{};
  double m_currentPt{10.0};
};
#endif  // MAINWINDOW_H
