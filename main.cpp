#include <QApplication>
#include <QJsonValue>
#include <QString>
#include <QVariant>

#include "json_nodes.h"
#include "mainwindow.h"

union Data {
  double num;
  quint32 index;
};

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  MainWindow w;
  auto args = a.arguments();
  if (args.size() == 2) {
    w.loadJsonFile(args[1]);
  }
  w.show();

  qDebug() << "QJsonValue" << sizeof(QJsonValue);
  qDebug() << "QVariant" << sizeof(QVariant);
  qDebug() << "QString" << sizeof(QString);
  qDebug() << "string" << sizeof(string);
  qDebug() << "data" << sizeof(Data);
  return a.exec();
}
