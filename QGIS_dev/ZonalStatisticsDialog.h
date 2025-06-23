// ZonalStatisticsDialog.h
#ifndef ZONALSTATISTICSDIALOG_H
#define ZONALSTATISTICSDIALOG_H

#include <QDialog>
#include <qgsfeature.h> // 包含QgsFeatureId
#include <QVariant>     // 包含QVariant
#include <qgscoordinatereferencesystem.h> // 包含QgsCoordinateReferenceSystem
#include "gdal_priv.h"
// ++ C++ 标准库线程相关头文件 ++
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
// ++ QTimer 的前向声明 ++
class QTimer;

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QSpinBox;
class QgsVectorLayer;
class QgsRasterLayer;
class QProgressDialog;

// 结构体用于存储每个分区的统计结果 (保持不变)
struct ZonalStatResult {
    QgsFeatureId featureId;
    QVariant originalZoneFieldValue;
    double min = 0.0;
    double max = 0.0;
    double sum = 0.0;
    double mean = 0.0;
    long count = 0;
    bool success = false;
    QString errorMessage;
};
// Q_DECLARE_METATYPE(ZonalStatResult) // 如果不在信号槽中使用，可以不注册

class ZonalStatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZonalStatisticsDialog(QWidget* parent = nullptr);
    ~ZonalStatisticsDialog();

private slots:
    void onSelectVectorLayerClicked();
    void onSelectRasterLayerClicked();
    void onSelectOutputTableClicked();
    void onVectorLayerChanged();
    void onRasterLayerChanged();
    void onOkClicked();
    // ++ 用于管理线程操作的新槽函数 ++
    void onCancelTaskClicked();     // 连接到 QProgressDialog 的取消按钮
    void checkTaskProgress();       // 由 QTimer 调用

private:
    void setupUI();
    void populateVectorLayerComboBox();
    void populateRasterLayerComboBox();
    void populateZoneIdFieldComboBox();
    // void updateDefaultOutputPath(); // 这个函数可能仍然有用

    ZonalStatResult calculateStatsForSingleFeature_thread_safe(
        const QgsGeometry& featureGeomConst,
        const QgsCoordinateReferenceSystem& featureCrs,
        GDALRasterBand* poBand, // ++ 接收共享的波段 ++
        const double adfGeoTransform[6],
        const QgsCoordinateReferenceSystem& rasterCrs,
        int bandNumForLog,
        QgsFeatureId featureId,
        const QVariant& zoneValue,
        std::atomic<bool>& cancelFlag
    );

    // worker_function 的签名需要改回接收共享的 poBand
    void worker_function(
        int threadId,
        const std::vector<QgsFeature>* features,
        size_t startIndex,
        size_t endIndex,
        GDALRasterBand* poBand, // ++ 接收共享的波段 ++
        // const QString* inputRasterPath, // 移除
        const double* adfGeoTransform,
        const QgsCoordinateReferenceSystem* rasterCrs,
        const QgsCoordinateReferenceSystem* vectorCrs,
        int bandNum, // 这个 bandNum 是 selectedBandNum
        const QString* zoneIdFieldNameStr
    );

    void finalizeAndWriteResults();

    // UI 控件
    QComboBox* m_vectorLayerCombo;
    QPushButton* m_btnSelectVectorFile; // 可以保留，但实现会简单些
    QLabel* m_lblSelectedVectorInfo;

    QComboBox* m_rasterLayerCombo;
    QPushButton* m_btnSelectRasterFile; // 可以保留
    QLabel* m_lblSelectedRasterInfo;

    QLabel* m_lblRasterBand;
    QSpinBox* m_spinRasterBand;

    QLabel* m_lblZoneIdField;
    QComboBox* m_zoneIdFieldCombo;

    QLineEdit* m_editOutputTablePath;
    QPushButton* m_btnSelectOutputTable;

    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;
    // --- 移除进度条和任务取消按钮 ---
    // QPushButton* m_btnCancelTask;
    // QProgressBar* m_progressBar;

    // ++ 线程和进度相关成员 ++
    std::vector<std::thread> m_workerThreads;
    std::vector<ZonalStatResult> m_allResults; // 共享的结果列表
    std::mutex m_resultsMutex;               // 用于 m_allResults 的互斥锁
    std::atomic<bool> m_cancelRequested;
    std::atomic<int> m_processedFeatureCount;
    std::atomic<int> m_completedThreadCount;
    int m_totalFeaturesToProcess;

    QProgressDialog* m_progressDialog; // 用于 UI 反馈
    QTimer* m_progressTimer;           // 定时器，用于从主线程更新进度

    // 数据和状态
    QgsVectorLayer* m_currentVectorLayer;
    QgsRasterLayer* m_currentRasterLayer;
    QString m_currentZoneIdFieldName;

    // 存储这些以传递给线程，因为从工作线程访问 QgsRasterLayer 可能不安全
    GDALDataset* m_gdalDataset; // 由主线程打开一次
    double m_adfGeoTransform[6];
    QgsCoordinateReferenceSystem m_vectorCrs; // 添加 m_vectorCrs 成员变量
    QgsCoordinateReferenceSystem m_rasterCrs; // 由主线程复制一次
    int m_selectedBandNum;

    // ++ 新增一个互斥锁专门用于保护 GDAL 的 RasterIO 操作 ++
    std::mutex m_gdalIoMutex;

    // 避免通过值向线程传递大型 QList<QgsFeature>
    std::vector<QgsFeature> m_featuresToProcessStdVec;
};

#endif // ZONALSTATISTICSDIALOG_H