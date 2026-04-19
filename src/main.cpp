#include <QApplication>
#include <QFont>
#include <QJsonValue>
#include <QString>
#include <QVariant>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  a.setApplicationName("QtJsonViewer");
  a.setOrganizationName("RCCR");
  a.setOrganizationDomain("rccr1987.com");
  a.setStyle("Fusion");

  QFont defaultFont = a.font();
  if (defaultFont.pointSizeF() < 12) {
    defaultFont.setPointSize(12); // Set your desired font size
    a.setFont(defaultFont);
  }
  MainWindow w;
  auto args = a.arguments();
  if (args.size() == 2) {
    w.loadJsonFile(args[1]);
  }
  w.showNormal();
  return a.exec();
}
