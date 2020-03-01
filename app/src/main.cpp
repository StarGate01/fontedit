#include "mainwindow.h"
#include "global.h"

#include <QApplication>
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setOrganizationName(Global::organization_name);
    QApplication::setOrganizationDomain(Global::organization_domain);
    QApplication::setApplicationName(Global::application_name);
    QApplication::setApplicationVersion(Global::application_version);

    qDebug() << "high DPI?" << qApp->testAttribute(Qt::AA_UseHighDpiPixmaps);
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    qDebug() << "high DPI?" << qApp->testAttribute(Qt::AA_UseHighDpiPixmaps);

    QTranslator myappTranslator;
    myappTranslator.load("fontedit_en");
    a.installTranslator(&myappTranslator);

    MainWindow w;
    w.show();
    return QApplication::exec();
}
