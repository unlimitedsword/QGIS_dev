#pragma warning(disable:4996)
#include "QGIS_dev.h"
#include "FileLogger.h"
#include "Output_Manager.h"
#include <QTextCodec>
#include <QDir>
#include <QDateTime>
#include <qgsapplication.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsproviderregistry.h>
#include <qgsmaplayer.h>
#include <qdebug.h>
#include <QMessageBox>
#include <QMetaType>

// 一个简单的函数，用于在屏幕上显示诊断信息
void showDiagnosis(const QString& title, const QString& message) {
    qDebug() << "---" << title << "---";
    qDebug() << message;
    QMessageBox::information(nullptr, title, message);
}

int main(int argc, char* argv[])
{
    // === STAGE 1: 创建QApplication实例 ===
    // 这是所有UI操作的前提，必须是第一步。
    QgsApplication a(argc, argv, true);
    qDebug() << "STAGE 1 PASSED: QgsApplication instance created.";

    // 注册 QgsMapLayer* 类型，让Qt的元对象系统在处理 QVariant 时能更好地识别它
    qRegisterMetaType<QgsMapLayer*>("QgsMapLayer*");

    // === STAGE 2: 强制设置环境变量 ===
    // 此时 QApplication 已存在，但 QGIS 核心还未初始化。
    // 这是设置环境变量的最佳时机。
    QString prefixPath = QgsApplication::applicationDirPath();

    // 使用 Qt 的 qputenv，它更安全
    // 它在内部处理了字符串的生命周期问题
    qputenv("PROJ_LIB", QDir::toNativeSeparators(prefixPath + "/share/proj").toLocal8Bit());
    qputenv("GDAL_DATA", QDir::toNativeSeparators(prefixPath + "/share/gdal").toLocal8Bit());
    qputenv("QT_PLUGIN_PATH", QDir::toNativeSeparators(prefixPath + "/plugins").toLocal8Bit());

    // 添加核心DLL目录到PATH，以防万一
    QByteArray pathEnv = qgetenv("PATH");
    QString newPath = QDir::toNativeSeparators(prefixPath) + ";" + pathEnv;
    qputenv("PATH", newPath.toLocal8Bit());

    // === STAGE 3: 初始化QGIS核心 ===
    // 此时环境变量已设置完毕，initQgis() 会在正确的环境下运行
    QgsApplication::setPrefixPath(prefixPath, true); // 仍然建议设置，因为它影响QGIS内部的其他路径逻辑
    QgsApplication::initQgis();
    qDebug() << "STAGE 3: QGIS Init 完成";

    // === STAGE 4: 关键功能测试 - CRS创建 ===
    QgsCoordinateReferenceSystem testCrs("EPSG:4326");
    if (testCrs.isValid()) {
        qDebug() << "STAGE 4: CRS Test - SUCCESS! 'EPSG:4326' created. Problem solved!";
    }
    else {
        qDebug() << "STAGE 4: CRS Test - FAILURE! Still cannot create CRS. This indicates a fundamental DLL or data file version mismatch.";
        return -1;
    }

    // === STAGE 5: 启动主程序 ===
    qDebug() << "All checks passed. Starting main application...";

    int result = 0;
    {
        QGIS_dev w;
        w.setMinimumSize(1920, 1080);
        w.show();

        // 日志系统现在可以安全初始化并使用了
        QDir logDir(QApplication::applicationDirPath());
        logDir.cdUp(); 
        logDir.cdUp();
        logDir.mkdir("logs");
        if (logDir.exists("logs")) {
            logDir.cd("logs");
        }
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm");
        QString logFilePath = logDir.filePath(timestamp + ".log");
        FileLogger* fileLogger = new FileLogger(logFilePath, &a);
        QObject::connect(OutputManager::instance(), &OutputManager::messageLogged, fileLogger, &FileLogger::onMessageLogged);
        OutputManager::instance()->logMessage("应用程序启动成功。");

        result = a.exec();
    }

    QgsApplication::exitQgis();
    return result;
}
