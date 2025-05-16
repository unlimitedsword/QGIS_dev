#pragma warning(disable:4996) 
#include "QGIS_dev.h"
#include <QtWidgets/QApplication>
#include <QTextCodec> // Qt5需要此头文件

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QGIS_dev w;
    w.setMinimumSize(600, 400);
    w.show();
    return a.exec();
}
