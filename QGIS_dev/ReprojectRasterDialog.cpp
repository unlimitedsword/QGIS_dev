#include "ReprojectRasterDialog.h"
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

#include <qgsproject.h>
#include <qgsrasterlayer.h>
// #include <qgsprojectionselectionwidget.h> // 不再需要
// #include <qgsprojectionselectiondialog.h> // 不再需要
#include "Output_Manager.h"
#include <QDebug>

// GDAL
#include "gdal_priv.h"
#include "gdalwarper.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"


ReprojectRasterDialog::ReprojectRasterDialog(QWidget* parent)
    : QDialog(parent),
    // +++ 明确初始化所有指针成员为 nullptr +++
    m_inputRasterCombo(nullptr),
    m_btnSelectInputRasterFile(nullptr),
    m_lblSelectedRasterInfo(nullptr),
    m_lblCurrentCrsInfo(nullptr),
    m_lblTargetCrs(nullptr),
    m_targetCrsCombo(nullptr),
    m_editOutputRasterPath(nullptr),
    m_btnSelectOutputRaster(nullptr),
    m_btnOk(nullptr),
    m_btnCancel(nullptr)
    // m_currentTargetCrs 会被默认构造
{
    setWindowTitle("栅格投影转换");
    setMinimumWidth(550);

    setupUI(); // 创建并布局所有UI控件

    populateInputRasterComboBox();
    populateTargetCrsComboBox();

    // --- 将所有 connect 语句集中到构造函数的末尾 ---
    // 确保此时所有UI控件都已创建完毕
    if (m_inputRasterCombo) { // 防御性检查
        connect(m_inputRasterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReprojectRasterDialog::onInputRasterChanged);
    }
    if (m_targetCrsCombo) { // 防御性检查
        connect(m_targetCrsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReprojectRasterDialog::onTargetCrsComboChanged);
    }
    if (m_btnSelectInputRasterFile) { // 防御性检查
        connect(m_btnSelectInputRasterFile, &QPushButton::clicked, this, &ReprojectRasterDialog::onSelectInputRasterClicked);
    }
    if (m_btnSelectOutputRaster) { // 防御性检查
        connect(m_btnSelectOutputRaster, &QPushButton::clicked, this, &ReprojectRasterDialog::onSelectOutputRasterClicked);
    }
    if (m_btnOk) { // 防御性检查
        connect(m_btnOk, &QPushButton::clicked, this, &ReprojectRasterDialog::onOkClicked);
    }
    if (m_btnCancel) { // 防御性检查
        connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }
}

ReprojectRasterDialog::~ReprojectRasterDialog()
{
    // 如果有资源需要释放，可以在这里添加代码
}

void ReprojectRasterDialog::setupUI()
{
    // === 1. 创建布局 ===
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    // === 2. 创建所有UI控件 (严格按照先new再使用的顺序) ===
    // 输入栅格
    QLabel* lblInputRaster = new QLabel("输入栅格图层:", this);
    m_inputRasterCombo = new QComboBox(this);
    m_btnSelectInputRasterFile = new QPushButton("从文件...", this);
    m_lblSelectedRasterInfo = new QLabel("当前选择: 无", this);
    m_lblCurrentCrsInfo = new QLabel("源CRS: 未知", this);

    // 目标CRS选择
    m_lblTargetCrs = new QLabel("选择目标CRS:", this);
    m_targetCrsCombo = new QComboBox(this);

    // 输出栅格路径
    QLabel* lblOutputPath = new QLabel("输出栅格路径:", this);
    m_editOutputRasterPath = new QLineEdit(this);
    m_editOutputRasterPath->setPlaceholderText("选择输出文件路径...");
    m_btnSelectOutputRaster = new QPushButton("浏览...", this);

    // 确定和取消按钮
    m_btnOk = new QPushButton("确定", this);
    m_btnCancel = new QPushButton("取消", this);

    // === 3. 将控件添加到布局中 ===
    gridLayout->addWidget(lblInputRaster, 0, 0);
    gridLayout->addWidget(m_inputRasterCombo, 0, 1);
    gridLayout->addWidget(m_btnSelectInputRasterFile, 0, 2);
    gridLayout->addWidget(m_lblSelectedRasterInfo, 1, 0, 1, 3);
    gridLayout->addWidget(m_lblCurrentCrsInfo, 2, 0, 1, 3);

    gridLayout->addWidget(m_lblTargetCrs, 3, 0);
    gridLayout->addWidget(m_targetCrsCombo, 3, 1, 1, 2);

    gridLayout->addWidget(lblOutputPath, 4, 0);
    gridLayout->addWidget(m_editOutputRasterPath, 4, 1);
    gridLayout->addWidget(m_btnSelectOutputRaster, 4, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // **注意：将connect语句移到构造函数的末尾，在所有控件创建之后**
}

void ReprojectRasterDialog::populateInputRasterComboBox()
{
    m_inputRasterCombo->clear();
    m_inputRasterCombo->addItem("--- 从项目中选择栅格 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto rasterLayer = qobject_cast<QgsRasterLayer*>(layer)) {
                m_inputRasterCombo->addItem(rasterLayer->name() + QString(" (%1)").arg(rasterLayer->crs().authid().isEmpty() ? "未知CRS" : rasterLayer->crs().authid()),
                    QVariant::fromValue(rasterLayer));
            }
    }
    onInputRasterChanged(0); // 更新初始状态
}


// +++ 新增：填充目标CRS下拉框的函数 +++
void ReprojectRasterDialog::populateTargetCrsComboBox()
{
    m_targetCrsCombo->clear();
    // 添加一些常见的CRS
    // 格式: 显示名称, QVariant(EPSG字符串)
    m_targetCrsCombo->addItem("WGS 84 (EPSG:4326)", "EPSG:4326");
    m_targetCrsCombo->addItem("Web Mercator (EPSG:3857)", "EPSG:3857");
    m_targetCrsCombo->addItem("CGCS2000 (EPSG:4490)", "EPSG:4490");
    m_targetCrsCombo->addItem("UTM Zone 48N (WGS84, EPSG:32648)", "EPSG:32648");
    m_targetCrsCombo->addItem("UTM Zone 49N (WGS84, EPSG:32649)", "EPSG:32649");
    m_targetCrsCombo->addItem("UTM Zone 50N (WGS84, EPSG:32650)", "EPSG:32650");
    m_targetCrsCombo->addItem("UTM Zone 51N (WGS84, EPSG:32651)", "EPSG:32651");
    // 可以根据需要添加更多

    // 默认选中第一个，并更新成员变量
    if (m_targetCrsCombo->count() > 0) {
        onTargetCrsComboChanged(0); // 触发更新 m_currentTargetCrs 和默认输出路径
    }
}


void ReprojectRasterDialog::onSelectInputRasterClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择输入栅格文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "栅格文件 (*.tif *.tiff *.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_currentInputRasterPath = filePath;
        m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (文件)").arg(QFileInfo(filePath).fileName()));
        m_inputRasterCombo->setCurrentIndex(0);

        // 尝试读取文件CRS
        QgsRasterLayer tempLayer(filePath);
        if (tempLayer.isValid()) {
            QgsCoordinateReferenceSystem crs = tempLayer.crs();
            m_lblCurrentCrsInfo->setText(QString("源CRS: %1").arg(crs.isValid() ? crs.description() : "未知/无法读取"));
        }
        else {
            m_lblCurrentCrsInfo->setText("源CRS: 无法读取");
        }
        updateDefaultOutputPath();
    }
}

void ReprojectRasterDialog::onInputRasterChanged(int index)
{
    if (index > 0) { // 从项目中选择
        QVariant data = m_inputRasterCombo->itemData(index);
        QgsRasterLayer* layer = qvariant_cast<QgsRasterLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentInputRasterPath = layer->source();
            m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (项目)").arg(layer->name()));
            QgsCoordinateReferenceSystem crs = layer->crs();
            m_lblCurrentCrsInfo->setText(QString("源CRS: %1").arg(crs.isValid() ? crs.description() : "未知"));
        }
    }
    else {
        // 如果选择了 "---选择---" 但之前文件路径有值，保留文件路径的信息
        if (m_currentInputRasterPath.isEmpty() || !m_lblSelectedRasterInfo->text().contains("(文件)")) {
            m_currentInputRasterPath.clear();
            m_lblSelectedRasterInfo->setText("当前选择: 无");
            m_lblCurrentCrsInfo->setText("源CRS: 未知");
        }
    }
    updateDefaultOutputPath();
}


// +++ 新增：当目标CRS下拉框选项改变时 +++
void ReprojectRasterDialog::onTargetCrsComboChanged(int index)
{
    if (index >= 0) {
        QString epsgString = m_targetCrsCombo->itemData(index).toString();
        m_currentTargetCrs.createFromString(epsgString); // 更新成员变量
        if (!m_currentTargetCrs.isValid()) {
            qWarning() << "Failed to create target CRS from string:" << epsgString;
        }
    }
    else {
        m_currentTargetCrs = QgsCoordinateReferenceSystem(); // 设为无效
    }
    updateDefaultOutputPath(); // 目标CRS变了，默认输出名也可能要变
}


void ReprojectRasterDialog::updateDefaultOutputPath()
{
    if (m_currentInputRasterPath.isEmpty()) {
        m_editOutputRasterPath->clear();
        return;
    }

    QFileInfo fi(m_currentInputRasterPath);
    QString baseName = fi.completeBaseName();
    QString dirPath = fi.absolutePath();
    QString suffix = fi.suffix();

    QString crsSuffix = "reprojected";
    if (m_currentTargetCrs.isValid() && !m_currentTargetCrs.authid().isEmpty()) {
        crsSuffix = m_currentTargetCrs.authid().replace(":", "_");
    }
    else if (m_currentTargetCrs.isValid()) {
        // 如果没有authid但有效，可以用描述的一部分（需要处理特殊字符）
        QString desc = m_currentTargetCrs.description().simplified().replace(" ", "_").remove(QRegExp("[^a-zA-Z0-9_]"));
        crsSuffix = desc.left(15); // 取前15个字符
    }


    QString defaultOutputName = QString("%1_%2.%3").arg(baseName).arg(crsSuffix).arg(suffix.isEmpty() ? "tif" : suffix);
    m_editOutputRasterPath->setText(QDir(dirPath).filePath(defaultOutputName));
}

void ReprojectRasterDialog::onSelectOutputRasterClicked()
{
    // ... (与ResampleDialog中的实现类似，确保有默认后缀) ...
    QString defaultPath = m_editOutputRasterPath->text();
    // ... (获取并设置路径) ...
    QString filePath = QFileDialog::getSaveFileName(this, "选择输出栅格文件路径",
        defaultPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation) : defaultPath,
        "GeoTIFF (*.tif *.tiff);;Erdas Imagine (*.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        if (QFileInfo(filePath).suffix().isEmpty()) {
            if (!filePath.endsWith(".")) filePath += ".";
            filePath += "tif";
        }
        m_editOutputRasterPath->setText(filePath);
    }
}

void ReprojectRasterDialog::onOkClicked()
{
    QString inputPath = m_currentInputRasterPath;
    // === 修改：从QComboBox获取目标CRS ===
    QgsCoordinateReferenceSystem targetCrs = m_currentTargetCrs;
    // ==================================
    QString outputPath = m_editOutputRasterPath->text();

    // --- 参数验证 ---
    if (inputPath.isEmpty()) { /* ... */ return; }
    if (!targetCrs.isValid()) { QMessageBox::warning(this, "输入错误", "请选择一个有效的目标坐标参考系。"); return; }
    if (outputPath.isEmpty()) { /* ... */ return; }

    qDebug() << "Reprojecting Raster (using GDALCreateAndReprojectImage):";
    qDebug() << "  Input:" << inputPath;
    qDebug() << "  Target CRS AuthID:" << targetCrs.authid() << "Desc:" << targetCrs.description();
    qDebug() << "  Output:" << outputPath;

    GDALAllRegister();
    GDALDatasetH hSrcDS = GDALOpen(inputPath.toUtf8().constData(), GA_ReadOnly);
    if (!hSrcDS) { /* ... */ return; }

    const char* pszSrcWKT = GDALGetProjectionRef(hSrcDS);
    if (pszSrcWKT == nullptr || strlen(pszSrcWKT) == 0) { /* ...警告并返回... */ GDALClose(hSrcDS); return; }

    // --- 获取目标WKT (从QgsCoordinateReferenceSystem对象) ---
    std::string targetWktStdString = targetCrs.toWkt().toStdString();
    // 或者 QgsCoordinateReferenceSystem::Wkt वन (更兼容旧GDAL)
    // std::string targetWktStdString = targetCrs.toWkt().toStdString(); 

    const char* pszDstWKT = targetWktStdString.c_str();
    // ----------------------------------------------------

    if (strlen(pszDstWKT) == 0) { /* ...错误并返回... */ GDALClose(hSrcDS); return; }
    qDebug() << "Source WKT:" << pszSrcWKT;
    qDebug() << "Target WKT:" << pszDstWKT;

    GDALResampleAlg eResampleAlg = GRA_Bilinear;
    GDALDriverH hDstDriver = GDALGetDriverByName("GTiff");
    if (!hDstDriver) { /* ... */ GDALClose(hSrcDS); return; }
    char** papszCreateOptions = nullptr;
    papszCreateOptions = CSLSetNameValue(papszCreateOptions, "TILED", "YES");
    papszCreateOptions = CSLSetNameValue(papszCreateOptions, "COMPRESS", "LZW");

    GDALWarpOptions* psOptions = GDALCreateWarpOptions(); // 仍然创建它，因为 GDALCreateAndReprojectImage 会用到

    CPLErr eErr = GDALCreateAndReprojectImage(
        hSrcDS, pszSrcWKT,
        outputPath.toUtf8().constData(), pszDstWKT,
        hDstDriver, papszCreateOptions,
        eResampleAlg,
        0.0, 0.125,
        GDALTermProgress, nullptr,
        psOptions
    );

    CSLDestroy(papszCreateOptions);
    GDALDestroyWarpOptions(psOptions);
    GDALClose(hSrcDS);

    if (eErr == CE_None) {
        QMessageBox::information(this, "成功",
            QString("栅格投影转换完成！\n输出文件已保存到:\n%1").arg(outputPath));
        // =====================================================================

        // (可选) 如果您希望转换成功后，自动将新生成的图层添加到项目中
        // QgsRasterLayer* newLayer = new QgsRasterLayer(outputPath, QFileInfo(outputPath).baseName());
        // if (newLayer->isValid()) {
        //     QgsProject::instance()->addMapLayer(newLayer);
        //     // 也可以在这里让主窗口的图层树刷新，或者缩放到新图层
        // } else {
        //     qWarning() << "Failed to load reprojected layer:" << outputPath << newLayer->error().message();
        //     delete newLayer;
        // }

        accept(); // 关闭对话框
    }
    else { QMessageBox::critical(this, "投影转换失败", "GDAL操作失败: " + QString(CPLGetLastErrorMsg())); }
}