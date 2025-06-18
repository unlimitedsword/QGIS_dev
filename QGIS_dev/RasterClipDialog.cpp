// RasterClipDialog.cpp
#include "RasterClipDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QProcess> // +++ 用于执行命令行 +++

#include <qgsproject.h>
#include <qgsrasterlayer.h>
#include <qgsvectorlayer.h>
#include <QgsApplication.h>
#include "Output_Manager.h"
#include <QDebug>

// GDAL (可能不需要直接包含，因为我们用命令行)
// #include "gdal_priv.h"

RasterClipDialog::RasterClipDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("栅格裁剪 (按矢量边界)");
    setMinimumWidth(550);
    setupUI();
    populateInputRasterComboBox();
    populateClipFeatureComboBox();
}

RasterClipDialog::~RasterClipDialog()
{
}

void RasterClipDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    // 1. 输入栅格
    gridLayout->addWidget(new QLabel("输入栅格图层:", this), 0, 0);
    m_inputRasterCombo = new QComboBox(this);
    gridLayout->addWidget(m_inputRasterCombo, 0, 1);
    m_btnSelectInputRasterFile = new QPushButton("从文件...", this);
    gridLayout->addWidget(m_btnSelectInputRasterFile, 0, 2);
    m_lblSelectedRasterInfo = new QLabel("当前选择: 无", this);
    gridLayout->addWidget(m_lblSelectedRasterInfo, 1, 0, 1, 3);

    // 2. 裁剪矢量边界
    gridLayout->addWidget(new QLabel("裁剪边界 (矢量面):", this), 2, 0);
    m_clipFeatureCombo = new QComboBox(this);
    gridLayout->addWidget(m_clipFeatureCombo, 2, 1);
    m_btnSelectClipFeatureFile = new QPushButton("从文件...", this);
    gridLayout->addWidget(m_btnSelectClipFeatureFile, 2, 2);
    m_lblSelectedClipFeatureInfo = new QLabel("当前选择: 无", this);
    gridLayout->addWidget(m_lblSelectedClipFeatureInfo, 3, 0, 1, 3);

    // 3. 输出栅格路径
    gridLayout->addWidget(new QLabel("输出裁剪后栅格:", this), 4, 0);
    m_editOutputRasterPath = new QLineEdit(this);
    m_editOutputRasterPath->setPlaceholderText("选择输出文件路径...");
    gridLayout->addWidget(m_editOutputRasterPath, 4, 1);
    m_btnSelectOutputRaster = new QPushButton("浏览...", this);
    gridLayout->addWidget(m_btnSelectOutputRaster, 4, 2);

    // ====================== 新增NoData处理选项 ======================
    m_lblOutputNoDataHandling = new QLabel("边界外区域处理:", this);
    gridLayout->addWidget(m_lblOutputNoDataHandling, 5, 0); // 行号调整为5

    m_comboNoDataHandling = new QComboBox(this);
    m_comboNoDataHandling->addItem("填充为黑色 (NoData=0)", "black"); // "black" 是我们内部用的标识
    m_comboNoDataHandling->addItem("设为透明 (Alpha波段)", "alpha");
    gridLayout->addWidget(m_comboNoDataHandling, 5, 1, 1, 2);
    // ===============================================================

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    // 4. 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnOk = new QPushButton("确定", this);
    m_btnCancel = new QPushButton("取消", this);
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // 连接信号
    connect(m_inputRasterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_currentInputRasterPath.clear(); // 清除文件路径选择
        if (index > 0) {
            QVariant data = m_inputRasterCombo->itemData(index);
            QgsRasterLayer* layer = qvariant_cast<QgsRasterLayer*>(data);
            if (layer && QgsProject::instance()->mapLayer(layer->id())) {
                m_currentInputRasterPath = layer->source();
                m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (项目)").arg(layer->name()));
            }
        }
        else {
            m_lblSelectedRasterInfo->setText("当前选择: 无");
        }
        // (可以添加更新默认输出名的逻辑)
        });
    connect(m_btnSelectInputRasterFile, &QPushButton::clicked, this, &RasterClipDialog::onSelectInputRasterClicked);

    connect(m_clipFeatureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_currentClipFeaturePath.clear();
        if (index > 0) {
            QVariant data = m_clipFeatureCombo->itemData(index);
            QgsVectorLayer* layer = qvariant_cast<QgsVectorLayer*>(data);
            if (layer && QgsProject::instance()->mapLayer(layer->id())) {
                m_currentClipFeaturePath = layer->source();
                m_lblSelectedClipFeatureInfo->setText(QString("当前选择: %1 (项目)").arg(layer->name()));
            }
        }
        else {
            m_lblSelectedClipFeatureInfo->setText("当前选择: 无");
        }
        // (更新默认输出名)
        });
    connect(m_btnSelectClipFeatureFile, &QPushButton::clicked, this, &RasterClipDialog::onSelectClipFeatureClicked);
    connect(m_btnSelectOutputRaster, &QPushButton::clicked, this, &RasterClipDialog::onSelectOutputRasterClicked);
    connect(m_btnOk, &QPushButton::clicked, this, &RasterClipDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void RasterClipDialog::populateInputRasterComboBox()
{
    m_inputRasterCombo->clear();
    m_inputRasterCombo->addItem("--- 从项目中选择栅格 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto rasterLayer = qobject_cast<QgsRasterLayer*>(layer)) {
                m_inputRasterCombo->addItem(rasterLayer->name(), QVariant::fromValue(rasterLayer));
            }
    }
}

void RasterClipDialog::populateClipFeatureComboBox()
{
    m_clipFeatureCombo->clear();
    m_clipFeatureCombo->addItem("--- 从项目中选择矢量面 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
                if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon) { // 只显示面图层
                    m_clipFeatureCombo->addItem(vectorLayer->name(), QVariant::fromValue(vectorLayer));
                }
            }
    }
}

void RasterClipDialog::onSelectInputRasterClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择输入栅格文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "栅格文件 (*.tif *.tiff *.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_currentInputRasterPath = filePath;
        m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (文件)").arg(QFileInfo(filePath).fileName()));
        m_inputRasterCombo->setCurrentIndex(0);
    }
}

void RasterClipDialog::onSelectClipFeatureClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择裁剪边界矢量文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "Shapefile (*.shp);;GeoPackage (*.gpkg);;所有矢量文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_currentClipFeaturePath = filePath;
        m_lblSelectedClipFeatureInfo->setText(QString("当前选择: %1 (文件)").arg(QFileInfo(filePath).fileName()));
        m_clipFeatureCombo->setCurrentIndex(0);
    }
}

void RasterClipDialog::onSelectOutputRasterClicked()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (!m_currentInputRasterPath.isEmpty()) {
        QFileInfo fi(m_currentInputRasterPath);
        defaultPath = QDir(fi.absolutePath()).filePath(fi.completeBaseName() + "_clipped.tif");
    }

    QString filePath = QFileDialog::getSaveFileName(this, "选择输出裁剪后栅格路径",
        defaultPath,
        "GeoTIFF (*.tif *.tiff)");
    if (!filePath.isEmpty()) {
        if (QFileInfo(filePath).suffix().compare("tif", Qt::CaseInsensitive) != 0 &&
            QFileInfo(filePath).suffix().compare("tiff", Qt::CaseInsensitive) != 0) {
            filePath += ".tif";
        }
        m_editOutputRasterPath->setText(filePath);
    }
}

void RasterClipDialog::onOkClicked()
{
    QString inputRaster = m_currentInputRasterPath;
    QString clipFeatures = m_currentClipFeaturePath;
    QString outputRaster = m_editOutputRasterPath->text();

    if (inputRaster.isEmpty()) { QMessageBox::warning(this, "输入错误", "请选择输入栅格图层。"); return; }
    if (clipFeatures.isEmpty()) { QMessageBox::warning(this, "输入错误", "请选择裁剪边界矢量图层。"); return; }
    if (outputRaster.isEmpty()) { QMessageBox::warning(this, "输入错误", "请输入输出栅格路径。"); return; }

    qDebug() << "Starting raster clip operation:";
    qDebug() << "  Input Raster:" << inputRaster;
    qDebug() << "  Clip Features:" << clipFeatures;
    qDebug() << "  Output Raster:" << outputRaster;

    // ====================== 执行GDAL Warp命令行 ======================
    QStringList arguments;
    arguments << "-cutline" << clipFeatures; // 指定裁剪矢量
    arguments << "-crop_to_cutline";         // 选项：输出范围严格按照cutline的范围

    // ====================== 根据用户选择添加NoData处理参数 ======================
    QString noDataHandling = m_comboNoDataHandling->currentData().toString();
    qDebug() << "  NoData Handling selection:" << noDataHandling;

    if (noDataHandling == "alpha") {
        arguments << "-dstalpha"; // 创建alpha波段使边界外透明
        // 当使用 -dstalpha 时，通常不需要再明确设置 -dstnodata，
        // 因为透明区域自然就是没有数据。
        // 如果原始栅格有NoData值，gdalwarp会尝试保留它们。
        // 如果想强制所有非裁剪区都是一个特定的NoData值（除了透明），可以考虑：
        // arguments << "-srcnodata" << " profiled_nodata_value_from_source "; // 如果源有明确NoData
        // arguments << "-dstnodata" << "0"; // 并将输出的NoData设为0 (但会被alpha覆盖)
    }
    else if (noDataHandling == "black") {
        // 填充为黑色，意味着我们将NoData值设为0
        // 我们需要告诉gdalwarp，输出栅格的NoData值是0。
        // 如果源栅格有不同的NoData值，我们可能需要先用-srcnodata告诉gdalwarp源的NoData是什么，
        // 然后用-dstnodata指定输出的NoData。
        // 为了简单起见，我们直接设置输出的NoData为0。
        // 如果源栅格本身有些区域是0，并且0不是它的NoData值，那么这些区域会被保留为0。
        // 裁剪边界之外的区域，gdalwarp在-crop_to_cutline时会用目标NoData值填充。
        arguments << "-dstnodata" << "0";
        // arguments << "-srcnodata" << " profiled_nodata_value_from_source "; // 可选，如果需要更精确控制
    }
    // =======================================================================


    // arguments << "-of" << "GTiff";        // 可选：指定输出格式 (gdalwarp通常能从输出文件名推断)
    // arguments << "-co" << "COMPRESS=LZW"; // 可选：创建选项
    // arguments << "-co" << "TILED=YES";

    arguments << inputRaster;                // 输入栅格文件
    arguments << outputRaster;               // 输出栅格文件

    // 找到 gdalwarp.exe 的路径
    // 理想情况下，这个路径应该通过环境变量或配置获得
    // 为了简单，我们假设它在应用程序的prefixPath的bin目录下（OSGeo4W环境）
    QString gdalwarpPath = QDir(QgsApplication::prefixPath()).filePath("bin/gdalwarp.exe");
    if (!QFile::exists(gdalwarpPath)) {
        // 尝试直接调用，希望它在系统PATH中
        gdalwarpPath = "gdalwarp";
        qWarning() << "gdalwarp.exe not found in prefix/bin, trying system PATH.";
    }

    QProcess process;
    qDebug() << "Executing:" << gdalwarpPath << arguments;
    process.start(gdalwarpPath, arguments);

    // 等待完成 (可以设置超时)
    if (!process.waitForFinished(-1)) { // -1 表示无限等待
        QMessageBox::critical(this, "错误", "gdalwarp进程执行失败: " + process.errorString());
        OutputManager::instance()->logError("gdalwarp error: " + process.readAllStandardError());
        return;
    }

    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "裁剪失败", "gdalwarp操作返回错误码: " + QString::number(process.exitCode()) +
            "\n\n错误输出:\n" + process.readAllStandardError() +
            "\n\n标准输出:\n" + process.readAllStandardOutput());
        return;
    }
    // =================================================================

    QMessageBox::information(this, "成功", "栅格裁剪完成！\n输出文件: " + outputRaster);

    // 自动加载结果图层到项目中
    QgsRasterLayer* newLayer = new QgsRasterLayer(outputRaster, QFileInfo(outputRaster).baseName());
    if (newLayer->isValid()) {
        QgsProject::instance()->addMapLayer(newLayer);
        // 可以让主窗口的地图缩放到新图层
        // emit operationCompletedAndLayerAdded(newLayer); // 如果需要通知主窗口
    }
    else {
        QMessageBox::warning(this, "警告", "裁剪结果已生成，但加载到地图失败。\n" + newLayer->error().message());
        delete newLayer;
    }
    accept();
}