// ZonalStatisticsDialog.cpp
#include "ZonalStatisticsDialog.h"
#include <algorithm>
#include <limits>
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
#include <QSpinBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer> // 用于 QTimer

#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgsrectangle.h>
#include <qgsrasterblock.h>
#include <qgsfields.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsunittypes.h>
#include <qgsdistancearea.h>
#include "Output_Manager.h"
#include <QDebug>

#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "ogr_geometry.h"
#include "cpl_conv.h"
#include "cpl_string.h"

ZonalStatisticsDialog::ZonalStatisticsDialog(QWidget* parent)
    : QDialog(parent),
    m_currentVectorLayer(nullptr),
    m_currentRasterLayer(nullptr),
    m_cancelRequested(false),
    m_processedFeatureCount(0),
    m_completedThreadCount(0),
    m_totalFeaturesToProcess(0),
    m_progressDialog(nullptr),
    m_progressTimer(nullptr),
    m_gdalDataset(nullptr)
{
    setWindowTitle("分区统计 (多线程)");
    setMinimumWidth(600);
    setupUI(); // setupUI 基本保持不变，QProgressDialog 按需创建
    populateVectorLayerComboBox();
    populateRasterLayerComboBox();

    connect(m_vectorLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ZonalStatisticsDialog::onVectorLayerChanged);
    connect(m_rasterLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ZonalStatisticsDialog::onRasterLayerChanged);
    connect(m_btnOk, &QPushButton::clicked, this, &ZonalStatisticsDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject); // 标准对话框取消

    connect(m_btnSelectVectorFile, &QPushButton::clicked, this, &ZonalStatisticsDialog::onSelectVectorLayerClicked);
    connect(m_btnSelectRasterFile, &QPushButton::clicked, this, &ZonalStatisticsDialog::onSelectRasterLayerClicked);
    connect(m_btnSelectOutputTable, &QPushButton::clicked, this, &ZonalStatisticsDialog::onSelectOutputTableClicked);

    // 初始化 QTimer
    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, &ZonalStatisticsDialog::checkTaskProgress);
}

ZonalStatisticsDialog::~ZonalStatisticsDialog()
{
    // 确保线程已加入（尽管我们使用的是 detach）
    // 并且 GDAL 数据集已关闭
    if (m_gdalDataset) {
        OutputManager::instance()->logMessage("在析构函数中关闭 GDAL 数据集。");
        GDALClose(m_gdalDataset);
        m_gdalDataset = nullptr;
    }
    // QProgressDialog 是一个子对象，Qt 会处理其删除。
    // m_workerThreads 是分离的，因此如果应用程序关闭，此处不需要 join。
}

void ZonalStatisticsDialog::setupUI() // 基本不变
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();
    int gridRow = 0;

    // ... (矢量图层、栅格图层、波段、区域ID字段、输出路径的 UI 设置如前) ...
    // 1. 输入矢量分区图层
    gridLayout->addWidget(new QLabel("分区图层 (矢量面):", this), gridRow, 0);
    m_vectorLayerCombo = new QComboBox(this);
    gridLayout->addWidget(m_vectorLayerCombo, gridRow, 1);
    m_btnSelectVectorFile = new QPushButton("从文件...", this);
    gridLayout->addWidget(m_btnSelectVectorFile, gridRow, 2);
    gridRow++;
    m_lblSelectedVectorInfo = new QLabel("当前选择: 无", this);
    m_lblSelectedVectorInfo->setWordWrap(true);
    gridLayout->addWidget(m_lblSelectedVectorInfo, gridRow, 0, 1, 3);

    gridRow++;
    // 2. 输入值栅格图层
    gridLayout->addWidget(new QLabel("值栅格图层:", this), gridRow, 0);
    m_rasterLayerCombo = new QComboBox(this);
    gridLayout->addWidget(m_rasterLayerCombo, gridRow, 1);
    m_btnSelectRasterFile = new QPushButton("从文件...", this);
    gridLayout->addWidget(m_btnSelectRasterFile, gridRow, 2);
    gridRow++;
    m_lblSelectedRasterInfo = new QLabel("当前选择: 无", this);
    m_lblSelectedRasterInfo->setWordWrap(true);
    gridLayout->addWidget(m_lblSelectedRasterInfo, gridRow, 0, 1, 3);

    gridRow++;
    // 3. 统计栅格波段
    m_lblRasterBand = new QLabel("统计栅格波段:", this);
    gridLayout->addWidget(m_lblRasterBand, gridRow, 0);
    m_spinRasterBand = new QSpinBox(this);
    m_spinRasterBand->setMinimum(1);
    m_spinRasterBand->setValue(1);
    m_spinRasterBand->setEnabled(false);
    gridLayout->addWidget(m_spinRasterBand, gridRow, 1, 1, 2);

    gridRow++;
    // 4. 区域标识字段 (可选)
    m_lblZoneIdField = new QLabel("区域标识字段 (可选):", this);
    gridLayout->addWidget(m_lblZoneIdField, gridRow, 0);
    m_zoneIdFieldCombo = new QComboBox(this);
    m_zoneIdFieldCombo->setToolTip("选择分区图层中用于在输出表中唯一标识区域的字段。\n如果未选择，将使用要素的内部FID。");
    gridLayout->addWidget(m_zoneIdFieldCombo, gridRow, 1, 1, 2);

    gridRow++;
    // 5. 输出统计表格路径
    gridLayout->addWidget(new QLabel("输出统计表格:", this), gridRow, 0);
    m_editOutputTablePath = new QLineEdit(this);
    m_editOutputTablePath->setPlaceholderText("例如: C:/output/zonal_stats.csv");
    gridLayout->addWidget(m_editOutputTablePath, gridRow, 1);
    m_btnSelectOutputTable = new QPushButton("浏览...", this);
    gridLayout->addWidget(m_btnSelectOutputTable, gridRow, 2);


    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnOk = new QPushButton("确定", this);
    m_btnCancel = new QPushButton("取消", this); // 标准 QDialog 取消按钮
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}


// --- populate*ComboBox, onSelect*Clicked, on*LayerChanged 方法基本保持不变 ---
// （如果添加它们，请确保它们正确更新 m_currentVectorLayer、m_currentRasterLayer 和路径变量）
void ZonalStatisticsDialog::populateVectorLayerComboBox()
{
    m_vectorLayerCombo->clear();
    m_vectorLayerCombo->addItem("--- 从项目中选择分区图层 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
        if (auto vlayer = qobject_cast<QgsVectorLayer*>(layer)) {
            Qgis::WkbType wkbType = vlayer->wkbType();
            Qgis::GeometryType geomType = QgsWkbTypes::geometryType(wkbType);
            if (geomType == Qgis::GeometryType::Polygon) {
                m_vectorLayerCombo->addItem(vlayer->name(), QVariant::fromValue(vlayer));
            }
        }
    }
    onVectorLayerChanged();
}

void ZonalStatisticsDialog::populateRasterLayerComboBox()
{
    m_rasterLayerCombo->clear();
    m_rasterLayerCombo->addItem("--- 从项目中选择栅格图层 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
        if (auto rlayer = qobject_cast<QgsRasterLayer*>(layer)) {
            m_rasterLayerCombo->addItem(rlayer->name(), QVariant::fromValue(rlayer));
        }
    }
    onRasterLayerChanged();
}

void ZonalStatisticsDialog::populateZoneIdFieldComboBox()
{
    m_zoneIdFieldCombo->clear();
    m_zoneIdFieldCombo->addItem("--- 使用要素FID ---", QString());
    if (m_currentVectorLayer) {
        const QgsFields fields = m_currentVectorLayer->fields();
        for (int i = 0; i < fields.count(); ++i) {
            m_zoneIdFieldCombo->addItem(fields.field(i).name() + " (" + fields.field(i).typeName() + ")",
                fields.field(i).name());
        }
    }
}

void ZonalStatisticsDialog::onSelectVectorLayerClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择分区矢量文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "Shapefile (*.shp);;GeoPackage (*.gpkg);;所有矢量文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_lblSelectedVectorInfo->setText(QString("文件待加载: %1 (路径: %2)").arg(QFileInfo(filePath).fileName()).arg(filePath));
        m_vectorLayerCombo->setCurrentIndex(0);
        m_currentVectorLayer = nullptr;
        populateZoneIdFieldComboBox();
    }
}

void ZonalStatisticsDialog::onSelectRasterLayerClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择值栅格文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "GeoTIFF (*.tif *.tiff);;Erdas Imagine (*.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_lblSelectedRasterInfo->setText(QString("文件待加载: %1 (路径: %2)").arg(QFileInfo(filePath).fileName()).arg(filePath));
        m_rasterLayerCombo->setCurrentIndex(0);
        m_currentRasterLayer = nullptr;
        m_spinRasterBand->setEnabled(false);
    }
}


void ZonalStatisticsDialog::onVectorLayerChanged()
{
    m_currentVectorLayer = nullptr;
    m_currentZoneIdFieldName.clear(); // 重置此项
    if (m_vectorLayerCombo->currentIndex() > 0) {
        QVariant data = m_vectorLayerCombo->currentData();
        QgsVectorLayer* layer = qvariant_cast<QgsVectorLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentVectorLayer = layer;
            m_lblSelectedVectorInfo->setText(QString("当前选择: %1").arg(layer->name()));
        }
    }
    else {
        // 检查之前是否通过“从文件...”设置了文件路径
        QString labelText = m_lblSelectedVectorInfo->text();
        if (!labelText.startsWith("文件待加载:")) {
            m_lblSelectedVectorInfo->setText("当前选择: 无");
        }
    }
    populateZoneIdFieldComboBox();
}

void ZonalStatisticsDialog::onRasterLayerChanged()
{
    m_currentRasterLayer = nullptr;
    m_spinRasterBand->setEnabled(false);
    if (m_rasterLayerCombo->currentIndex() > 0) {
        QVariant data = m_rasterLayerCombo->itemData(m_rasterLayerCombo->currentIndex());
        QgsRasterLayer* layer = qvariant_cast<QgsRasterLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentRasterLayer = layer;
            m_lblSelectedRasterInfo->setText(QString("当前选择: %1").arg(layer->name()));
            m_spinRasterBand->setMaximum(layer->bandCount());
            m_spinRasterBand->setEnabled(true);
        }
    }
    else {
        QString labelText = m_lblSelectedRasterInfo->text();
        if (!labelText.startsWith("文件待加载:")) {
            m_lblSelectedRasterInfo->setText("当前选择: 无");
        }
        else {
            // 如果设置了文件路径，则尝试从中获取波段数
            QString pathFromFileSelection = labelText.mid(labelText.indexOf("(路径: ") + 7);
            pathFromFileSelection.chop(1); // 移除尾随的 ')'
            if (!pathFromFileSelection.isEmpty()) {
                GDALDataset* tempDS = (GDALDataset*)GDALOpen(pathFromFileSelection.toUtf8().constData(), GA_ReadOnly);
                if (tempDS) {
                    m_spinRasterBand->setMaximum(tempDS->GetRasterCount());
                    m_spinRasterBand->setEnabled(true);
                    GDALClose(tempDS);
                }
            }
        }
    }
}


void ZonalStatisticsDialog::onSelectOutputTableClicked()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    // 尝试生成一个更智能的默认名称
    QString baseName = "zonal_stats";
    if (m_currentVectorLayer) {
        baseName = QFileInfo(m_currentVectorLayer->name()).completeBaseName() + "_stats";
    }
    else if (m_lblSelectedVectorInfo->text().startsWith("文件待加载:")) {
        QString path = m_lblSelectedVectorInfo->text();
        path = path.mid(path.indexOf("(路径: ") + 7);
        path.chop(1);
        if (!path.isEmpty()) baseName = QFileInfo(path).completeBaseName() + "_stats";
    }

    defaultPath = QDir(QgsProject::instance()->homePath().isEmpty() ? defaultPath : QgsProject::instance()->homePath()).filePath(baseName + ".csv");


    QString filePath = QFileDialog::getSaveFileName(this, "选择输出统计表格路径",
        defaultPath,
        "CSV 文件 (*.csv)");
    if (!filePath.isEmpty()) {
        if (QFileInfo(filePath).suffix().compare("csv", Qt::CaseInsensitive) != 0) {
            filePath += ".csv";
        }
        m_editOutputTablePath->setText(filePath);
    }
}


// 恢复 calculateStatsForSingleFeature_thread_safe 以接收共享的 poBand
ZonalStatResult ZonalStatisticsDialog::calculateStatsForSingleFeature_thread_safe(
    const QgsGeometry& featureGeomConst,
    const QgsCoordinateReferenceSystem& featureCrs,
    GDALRasterBand* poBand_shared,      // ++ 接收共享的波段 ++
    const double adfGeoTransform_shared[6],
    const QgsCoordinateReferenceSystem& rasterCrs_shared,
    int bandNumForLog,
    QgsFeatureId featureId,
    const QVariant& zoneValue,
    std::atomic<bool>& cancelFlag)
{
    ZonalStatResult result;
    result.featureId = featureId;
    result.originalZoneFieldValue = zoneValue;
    result.success = false;

    if (cancelFlag.load()) {
        result.errorMessage = "操作已被用户取消。";
        return result;
    }

    // OGR Geometry and CRS Transformation (这部分逻辑不变，线程局部)
    // ... (如前一个版本所示，创建 ogrGeom, poTransformLtd 等) ...
    OGRGeometry* ogrGeom = nullptr;
    OGRSpatialReference ogrFeatureSRSLtd, ogrRasterSRSLtd;
    OGRCoordinateTransformation* poTransformLtd = nullptr;

    // (确保这里正确创建和转换 ogrGeom，并在出错或结束时清理)
    // (代码与上一个回答中的 calculateStatsForSingleFeature_thread_safe 的 OGR 部分相同)
    QString wktString = featureGeomConst.asWkt();
    if (wktString.isEmpty()) { result.errorMessage = "..."; return result; }
    QByteArray wktUtf8 = wktString.toUtf8();
    char* pszWktFromQgsFeature_geom = CPLStrdup(wktUtf8.constData());
    if (!pszWktFromQgsFeature_geom) { result.errorMessage = "..."; return result; }
    char* tempWktPtr_geom = pszWktFromQgsFeature_geom;
    OGRErr ogrErr = OGRGeometryFactory::createFromWkt(&tempWktPtr_geom, nullptr, &ogrGeom);
    CPLFree(pszWktFromQgsFeature_geom);
    if (ogrErr != OGRERR_NONE || !ogrGeom) { result.errorMessage = "..."; if (ogrGeom) OGRGeometryFactory::destroyGeometry(ogrGeom); return result; }
    if (ogrGeom->IsEmpty() || !ogrGeom->IsValid()) { result.errorMessage = "..."; OGRGeometryFactory::destroyGeometry(ogrGeom); return result; }

    QByteArray featureCrsWktU = featureCrs.toWkt().toUtf8();
    char* pszWktFeatCrs = CPLStrdup(featureCrsWktU.constData());
    QByteArray rasterCrsWktU = rasterCrs_shared.toWkt().toUtf8();
    char* pszWktRastCrs = CPLStrdup(rasterCrsWktU.constData());
    if (!pszWktFeatCrs || !pszWktRastCrs) { result.errorMessage = "..."; OGRGeometryFactory::destroyGeometry(ogrGeom); if (pszWktFeatCrs) CPLFree(pszWktFeatCrs); if (pszWktRastCrs) CPLFree(pszWktRastCrs); return result; }
    char* tempFeat = pszWktFeatCrs;
    char* tempRast = pszWktRastCrs;
    if (ogrFeatureSRSLtd.importFromWkt(&tempFeat) != OGRERR_NONE ||
        ogrRasterSRSLtd.importFromWkt(&tempRast) != OGRERR_NONE) {
        result.errorMessage = "..."; /* 清理 */ OGRGeometryFactory::destroyGeometry(ogrGeom); CPLFree(pszWktFeatCrs); CPLFree(pszWktRastCrs); return result;
    }
    CPLFree(pszWktFeatCrs); CPLFree(pszWktRastCrs);

    if (!ogrFeatureSRSLtd.IsSame(&ogrRasterSRSLtd)) {
        poTransformLtd = OGRCreateCoordinateTransformation(&ogrFeatureSRSLtd, &ogrRasterSRSLtd);
        if (!poTransformLtd) { result.errorMessage = "..."; /* 清理 */ OGRGeometryFactory::destroyGeometry(ogrGeom); return result; }
        if (ogrGeom->transform(poTransformLtd) != OGRERR_NONE) { result.errorMessage = "..."; /* 清理 */ OGRGeometryFactory::destroyGeometry(ogrGeom); OGRCoordinateTransformation::DestroyCT(poTransformLtd); return result; }
    }


    if (cancelFlag.load()) { /* 清理 OGR, return */ OGRGeometryFactory::destroyGeometry(ogrGeom); if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd); return result; }

    int bGotNoData = 0;
    // 使用共享的 poBand_shared
    double noDataValue = poBand_shared->GetNoDataValue(&bGotNoData);

    double invGeoTransform[6];
    if (!GDALInvGeoTransform(const_cast<double*>(adfGeoTransform_shared), invGeoTransform)) {
        result.errorMessage = "GDALInvGeoTransform 失败。";
        OGRGeometryFactory::destroyGeometry(ogrGeom); if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd);
        return result;
    }

    // ... (像素窗口计算逻辑不变，使用 invGeoTransform 和 ogrGeom->getEnvelope()) ...
    OGREnvelope ogrEnv;
    ogrGeom->getEnvelope(&ogrEnv);
    int pix_ulx = static_cast<int>(std::floor(invGeoTransform[0] + invGeoTransform[1] * ogrEnv.MinX + invGeoTransform[2] * ogrEnv.MaxY));
    int pix_uly = static_cast<int>(std::floor(invGeoTransform[3] + invGeoTransform[4] * ogrEnv.MinX + invGeoTransform[5] * ogrEnv.MaxY));
    int pix_lrx = static_cast<int>(std::ceil(invGeoTransform[0] + invGeoTransform[1] * ogrEnv.MaxX + invGeoTransform[2] * ogrEnv.MinY));
    int pix_lry = static_cast<int>(std::ceil(invGeoTransform[3] + invGeoTransform[4] * ogrEnv.MaxX + invGeoTransform[5] * ogrEnv.MinY));
    int xOff = std::min(pix_ulx, pix_lrx);
    int yOff = std::min(pix_uly, pix_lry);
    int reqXSize = std::abs(pix_lrx - pix_ulx) + 1;
    int reqYSize = std::abs(pix_lry - pix_uly) + 1;
    int rasterWidth = poBand_shared->GetXSize(); // 使用共享的波段
    int rasterHeight = poBand_shared->GetYSize(); // 使用共享的波段
    int readXOff = std::max(0, xOff);
    int readYOff = std::max(0, yOff);
    int readXSize = std::min(xOff + reqXSize, rasterWidth) - readXOff;
    int readYSize = std::min(yOff + reqYSize, rasterHeight) - readYOff;

    if (readXSize <= 0 || readYSize <= 0) {
        result.errorMessage = "要素与栅格无有效重叠或超出栅格范围。";
        result.success = true; result.count = 0;
        OGRGeometryFactory::destroyGeometry(ogrGeom); if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd);
        return result;
    }

    if (cancelFlag.load()) { /* 清理 OGR, return */ OGRGeometryFactory::destroyGeometry(ogrGeom); if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd); return result; }

    std::vector<float> rasterData(static_cast<size_t>(readXSize) * readYSize);
    CPLErr ioErr = CE_Failure; // 初始化为失败

    { // ++ 用互斥锁保护 RasterIO 调用 ++
        std::lock_guard<std::mutex> ioLock(m_gdalIoMutex);
        if (!cancelFlag.load()) { // 再次检查取消标志，以防在等待锁时被取消
            ioErr = poBand_shared->RasterIO(GF_Read, readXOff, readYOff, readXSize, readYSize,
                rasterData.data(), readXSize, readYSize, GDT_Float32,
                0, 0, nullptr);
        }
        else {
            result.errorMessage = "操作在 RasterIO 前被取消。";
            // 清理 OGR 对象
            OGRGeometryFactory::destroyGeometry(ogrGeom);
            if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd);
            return result;
        }
    } // -- 互斥锁作用域结束 --

    if (ioErr != CE_None) {
        result.errorMessage = "GDAL RasterIO 失败。错误: " + QString(CPLGetLastErrorMsg());
        // 清理 OGR 对象
        OGRGeometryFactory::destroyGeometry(ogrGeom);
        if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd);
        return result;
    }

    // ... (统计循环逻辑不变，使用 rasterData) ...
    double minVal = std::numeric_limits<double>::max();
    double maxVal = -std::numeric_limits<double>::max();
    double sum = 0.0;
    long count = 0;
    for (int y = 0; y < readYSize; ++y) {
        if (cancelFlag.load()) break;
        for (int x = 0; x < readXSize; ++x) {
            double geoX = adfGeoTransform_shared[0] + (readXOff + x + 0.5) * adfGeoTransform_shared[1] + (readYOff + y + 0.5) * adfGeoTransform_shared[2];
            double geoY = adfGeoTransform_shared[3] + (readXOff + x + 0.5) * adfGeoTransform_shared[4] + (readYOff + y + 0.5) * adfGeoTransform_shared[5];
            OGRPoint pt;
            pt.assignSpatialReference(&ogrRasterSRSLtd);
            pt.setX(geoX);
            pt.setY(geoY);
            if (ogrGeom->Intersects(&pt)) {
                float val = rasterData[static_cast<size_t>(y) * readXSize + x];
                if (bGotNoData && qAbs(static_cast<double>(val) - noDataValue) < 1e-9) continue;
                if (std::isnan(val) || std::isinf(val)) continue;
                if (count == 0) { minVal = maxVal = val; }
                else { if (val < minVal) minVal = val; if (val > maxVal) maxVal = val; }
                sum += val;
                count++;
            }
        }
    }
    if (cancelFlag.load()) { result.errorMessage = "在统计计算过程中操作被取消。"; }
    else if (count > 0) { result.sum = sum; result.min = minVal; result.max = maxVal; result.mean = sum / count; result.success = true; }
    else { double nanOrNoData = bGotNoData ? noDataValue : std::numeric_limits<double>::quiet_NaN(); result.min = result.max = result.sum = result.mean = nanOrNoData; result.errorMessage = "在要素区域内未找到有效的栅格像元。"; result.success = true; }
    result.count = count;

    // 清理局部 OGR 对象
    OGRGeometryFactory::destroyGeometry(ogrGeom);
    if (poTransformLtd) OGRCoordinateTransformation::DestroyCT(poTransformLtd);

    return result;
}



// 修改 worker_function 以接收共享的 poBand
void ZonalStatisticsDialog::worker_function(
    int threadId,
    const std::vector<QgsFeature>* features,
    size_t startIndex,
    size_t endIndex,
    GDALRasterBand* poBand_shared,         // ++ 接收共享的波段 ++
    // const QString* inputRasterPath,    // 移除
    const double* adfGeoTransform_shared,
    const QgsCoordinateReferenceSystem* rasterCrs_shared,
    const QgsCoordinateReferenceSystem* vectorCrs_shared,
    int bandNum, // 这是 selectedBandNum
    const QString* zoneIdFieldNameStr
)
{
    OutputManager::instance()->logMessage(QString("工作线程 %1: 已启动。正在处理要素 %2 到 %3。")
        .arg(threadId).arg(startIndex).arg(endIndex - 1));

    for (size_t i = startIndex; i < endIndex; ++i) {
        if (m_cancelRequested.load()) { /* ... log and break ... */ break; }

        const QgsFeature& feat = (*features)[i];
        QVariant zoneIdVal = zoneIdFieldNameStr->isEmpty() ? QVariant(feat.id()) : feat.attribute(*zoneIdFieldNameStr);

        ZonalStatResult singleResult = calculateStatsForSingleFeature_thread_safe(
            feat.geometry(),
            *vectorCrs_shared, // 使用传递的矢量CRS
            poBand_shared,     // ++ 传递共享的波段 ++
            adfGeoTransform_shared,
            *rasterCrs_shared, // 使用传递的栅格CRS
            bandNum,           // 日志用波段号 (与 selectedBandNum 相同)
            feat.id(),
            zoneIdVal,
            m_cancelRequested
        );

        {
            std::lock_guard<std::mutex> lock(m_resultsMutex);
            m_allResults.push_back(singleResult);
        }
        m_processedFeatureCount.fetch_add(1);
    }
    m_completedThreadCount.fetch_add(1);
    OutputManager::instance()->logMessage(QString("工作线程 %1: 已完成。已处理 %2 个要素。")
        .arg(threadId).arg(endIndex - startIndex));
}


void ZonalStatisticsDialog::onOkClicked()
{
    // --- 1. 参数获取与初步验证 ---
    QString vectorPath, rasterPath;
    bool isVectorFromProject = (m_vectorLayerCombo->currentIndex() > 0 && m_currentVectorLayer != nullptr);
    bool isRasterFromProject = (m_rasterLayerCombo->currentIndex() > 0 && m_currentRasterLayer != nullptr);

    if (isVectorFromProject) {
        vectorPath = m_currentVectorLayer->source();
    }
    else if (m_lblSelectedVectorInfo->text().startsWith("文件待加载:")) {
        vectorPath = m_lblSelectedVectorInfo->text();
        vectorPath = vectorPath.mid(vectorPath.indexOf("(路径: ") + 7);
        vectorPath.chop(1); // 移除末尾的 ')'
        if (vectorPath.isEmpty() || !QFile::exists(vectorPath)) {
            QMessageBox::warning(this, "输入错误", "选择的分区矢量文件路径无效或文件不存在。"); return;
        }
    }
    else {
        QMessageBox::warning(this, "输入错误", "请选择分区图层（项目或文件）。"); return;
    }

    if (isRasterFromProject) {
        rasterPath = m_currentRasterLayer->source();
    }
    else if (m_lblSelectedRasterInfo->text().startsWith("文件待加载:")) {
        rasterPath = m_lblSelectedRasterInfo->text();
        rasterPath = rasterPath.mid(rasterPath.indexOf("(路径: ") + 7);
        rasterPath.chop(1); // 移除末尾的 ')'
        if (rasterPath.isEmpty() || !QFile::exists(rasterPath)) {
            QMessageBox::warning(this, "输入错误", "选择的值栅格文件路径无效或文件不存在。"); return;
        }
    }
    else {
        QMessageBox::warning(this, "输入错误", "请选择值栅格图层（项目或文件）。"); return;
    }

    QString outputPath = m_editOutputTablePath->text();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入输出统计表格的路径。"); return;
    }
    m_selectedBandNum = m_spinRasterBand->value(); // 存储波段号供线程使用
    m_currentZoneIdFieldName = m_zoneIdFieldCombo->currentData().toString(); // 区域ID字段名

    // --- 2. 准备图层对象 (可能是临时的) 和 CRS ---
    std::unique_ptr<QgsVectorLayer> tempVecLayerOwner;
    std::unique_ptr<QgsRasterLayer> tempRasLayerOwner;

    QgsVectorLayer* vecLayerToProcess = nullptr;
    QgsRasterLayer* rasLayerToProcess = nullptr;

    if (isVectorFromProject) {
        vecLayerToProcess = m_currentVectorLayer;
    }
    else {
        tempVecLayerOwner = std::make_unique<QgsVectorLayer>(vectorPath, QFileInfo(vectorPath).baseName(), "ogr");
        if (!tempVecLayerOwner || !tempVecLayerOwner->isValid()) {
            QMessageBox::warning(this, "加载错误", "无法加载选择的矢量文件: " + vectorPath + "\n错误: " + (tempVecLayerOwner ? tempVecLayerOwner->error().message() : "未知错误"));
            return;
        }
        vecLayerToProcess = tempVecLayerOwner.get();
    }

    if (isRasterFromProject) {
        rasLayerToProcess = m_currentRasterLayer;
    }
    else {
        tempRasLayerOwner = std::make_unique<QgsRasterLayer>(rasterPath, QFileInfo(rasterPath).baseName(), "gdal");
        if (!tempRasLayerOwner || !tempRasLayerOwner->isValid()) {
            QMessageBox::warning(this, "加载错误", "无法加载选择的栅格文件: " + rasterPath + "\n错误: " + (tempRasLayerOwner ? tempRasLayerOwner->error().message() : "未知错误"));
            return;
        }
        rasLayerToProcess = tempRasLayerOwner.get();
    }

    if (!vecLayerToProcess || !rasLayerToProcess) { // 双重检查
        QMessageBox::critical(this, "内部错误", "无法准备图层进行处理。");
        return;
    }

    // 在主线程中安全地获取 CRS
    m_vectorCrs = vecLayerToProcess->crs();
    m_rasterCrs = rasLayerToProcess->crs();

    if (m_selectedBandNum <= 0 || m_selectedBandNum > rasLayerToProcess->bandCount()) {
        QMessageBox::warning(this, "输入错误", QString("选择的波段号(%1)无效。栅格图层 '%2' 的波段数为: %3")
            .arg(m_selectedBandNum).arg(rasLayerToProcess->name()).arg(rasLayerToProcess->bandCount()));
        return;
    }

    // --- 3. 准备要处理的要素列表 (拷贝到 std::vector) ---
    m_featuresToProcessStdVec.clear();
    QgsFeatureIterator iterator = vecLayerToProcess->getFeatures();
    QgsFeature feature;
    while (iterator.nextFeature(feature)) {
        m_featuresToProcessStdVec.push_back(feature); // QgsFeature 被拷贝
    }

    if (m_featuresToProcessStdVec.empty()) {
        QMessageBox::information(this, "提示", "输入矢量图层中没有要素。");
        return;
    }
    m_totalFeaturesToProcess = m_featuresToProcessStdVec.size();

    // --- 4. UI 设置和状态重置 ---
    m_btnOk->setEnabled(false);
    m_btnCancel->setText("取消任务");
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 清理旧的进度对话框（如果存在）
    if (m_progressDialog) {
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    m_progressDialog = new QProgressDialog("正在执行分区统计...", "请求取消", 0, m_totalFeaturesToProcess, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0); // 立即显示
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);
    connect(m_progressDialog, &QProgressDialog::canceled, this, &ZonalStatisticsDialog::onCancelTaskClicked);
    m_progressDialog->setValue(0);
    m_progressDialog->show();
    QCoreApplication::processEvents(); // 确保对话框显示

    m_allResults.clear();
    m_cancelRequested.store(false);
    m_processedFeatureCount.store(0);
    m_completedThreadCount.store(0);
    m_workerThreads.clear(); // 清理旧的线程对象（尽管它们应该是 detached 且已完成）

    // --- 5. 打开 GDAL 数据集 (主线程，一次性) ---
    GDALAllRegister();
    if (m_gdalDataset) { // 关闭先前可能打开的
        OutputManager::instance()->logMessage("关闭先前打开的 GDAL 数据集。");
        GDALClose(m_gdalDataset);
        m_gdalDataset = nullptr;
    }
    m_gdalDataset = (GDALDataset*)GDALOpen(rasLayerToProcess->source().toUtf8().constData(), GA_ReadOnly);
    if (!m_gdalDataset) {
        QMessageBox::critical(this, "GDAL错误", "无法打开栅格文件: " + rasLayerToProcess->source() + "\n错误: " + CPLGetLastErrorMsg());
        finalizeAndWriteResults();
        return;
    }
    if (m_gdalDataset->GetGeoTransform(m_adfGeoTransform) != CE_None) { // m_adfGeoTransform 是成员变量
        QMessageBox::critical(this, "GDAL错误", "无法获取栅格文件的 GeoTransform。\n错误: " + QString(CPLGetLastErrorMsg()));
        finalizeAndWriteResults();
        return;
    }
    // m_rasterCrs 已在上面从 rasLayerToProcess 获取

    GDALRasterBand* poBand_main = m_gdalDataset->GetRasterBand(m_selectedBandNum); // 获取波段
    if (!poBand_main) {
        QMessageBox::critical(this, "GDAL错误", QString("无法从栅格 '%1' 获取波段 %2。\n错误: %3")
            .arg(rasLayerToProcess->name()).arg(m_selectedBandNum).arg(CPLGetLastErrorMsg()));
        finalizeAndWriteResults();
        return;
    }

    // --- 6. 启动工作线程 ---
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    numThreads = std::min(numThreads, static_cast<unsigned int>(m_totalFeaturesToProcess));
    if (m_totalFeaturesToProcess < 4 && numThreads > 1) numThreads = 1;
    OutputManager::instance()->logMessage(QString("分区统计: 为 %2 个要素启动 %1 个工作线程。").arg(numThreads).arg(m_totalFeaturesToProcess));
    size_t featuresPerThread = (m_totalFeaturesToProcess > 0 && numThreads > 0) ? (m_totalFeaturesToProcess / numThreads) : 0;
    size_t remainderFeatures = (m_totalFeaturesToProcess > 0 && numThreads > 0) ? (m_totalFeaturesToProcess % numThreads) : 0;
    size_t currentStartIndex = 0;
    if (numThreads == 0 && m_totalFeaturesToProcess > 0) { numThreads = 1; featuresPerThread = m_totalFeaturesToProcess; remainderFeatures = 0; }


    for (unsigned int i = 0; i < numThreads; ++i) {
        size_t numFeaturesForThisThread = featuresPerThread + (i < remainderFeatures ? 1 : 0);
        if (numFeaturesForThisThread == 0 && m_totalFeaturesToProcess > 0) { // 如果总要素>0但计算出的单个线程要素为0，则分配所有剩余要素
            if (i == 0) numFeaturesForThisThread = m_totalFeaturesToProcess; // 确保至少有一个线程工作
            else continue; // 其他线程不分配
        }
        if (numFeaturesForThisThread == 0) continue;


        size_t endIndex = currentStartIndex + numFeaturesForThisThread;
        if (endIndex > m_featuresToProcessStdVec.size()) { // 确保不越界
            endIndex = m_featuresToProcessStdVec.size();
        }
        if (currentStartIndex >= endIndex && m_totalFeaturesToProcess > 0) { // 如果起始已经大于等于结束，且还有要素要处理（通常不应发生）
            if (i == 0) { // 第一个线程处理所有
                currentStartIndex = 0;
                endIndex = m_featuresToProcessStdVec.size();
                numFeaturesForThisThread = m_featuresToProcessStdVec.size();
            }
            else {
                continue;
            }
        }


        // 传递 m_vectorCrs 和 m_rasterCrs 的指针
        // m_currentZoneIdFieldName 已经是成员，可以通过指针传递其地址
        m_workerThreads.emplace_back(
            &ZonalStatisticsDialog::worker_function, this,
            i + 1, // threadId
            &m_featuresToProcessStdVec,
            currentStartIndex,
            endIndex,
            poBand_main,                // ++ 传递从主数据集获取的波段指针 ++
            m_adfGeoTransform,          // 共享的地理变换 (成员)
            &m_rasterCrs,               // 指向栅格CRS (成员)
            &m_vectorCrs,               // 指向矢量CRS (成员)
            m_selectedBandNum,          // 要统计的波段号
            &m_currentZoneIdFieldName
        );
        currentStartIndex = endIndex;
    }

    // 分离线程，以便 onOkClicked 可以返回，UI 保持响应
    for (auto& t : m_workerThreads) {
        if (t.joinable()) {
            t.detach();
        }
    }
    // m_workerThreads.clear(); // Detached threads don't need to be in the vector anymore for joining
                           // But keeping them might be useful if you wanted to implement a more complex join mechanism later.
                           // For now, clearing is fine if `m_completedThreadCount` is the sole mechanism for completion.
                           // Let's keep it to check size in checkTaskProgress.

    m_progressTimer->start(100); // 每 100ms 检查一次进度

    // tempVecLayerOwner 和 tempRasLayerOwner 会在 onOkClicked 结束时自动析构，
    // 释放它们管理的临时 QgsLayer 对象。
    // 工作线程使用的是拷贝的 QgsFeature 和预先提取的 CRS 及 GDAL 对象，所以这是安全的。
}

void ZonalStatisticsDialog::onCancelTaskClicked() {
    OutputManager::instance()->logMessage("分区统计: 用户点击了取消按钮。");
    m_cancelRequested.store(true);
    if (m_progressDialog) {
        m_progressDialog->setLabelText("正在取消操作，请稍候...");
        m_progressDialog->setCancelButton(nullptr); // 禁止进一步点击
    }
}

void ZonalStatisticsDialog::checkTaskProgress() {
    if (!m_progressDialog) return;

    int processed = m_processedFeatureCount.load();
    m_progressDialog->setValue(processed);

    if (m_cancelRequested.load() && m_progressDialog->labelText() != "正在取消操作，请稍候...") {
        m_progressDialog->setLabelText("正在取消操作，请稍候...");
    }

    // 适用于单线程情况
    if (m_completedThreadCount.load() == (m_workerThreads.empty() ? (m_totalFeaturesToProcess > 0 ? 1 : 0) : m_workerThreads.size()) ||
        processed >= m_totalFeaturesToProcess) { // 所有要素已处理或所有线程已完成
        m_progressTimer->stop();
        if (m_progressDialog && m_progressDialog->isVisible()) { // 检查是否可见
            m_progressDialog->setValue(m_totalFeaturesToProcess); // 确保达到 100%
        }
        finalizeAndWriteResults();
    }
}


// ZonalStatisticsDialog.cpp - finalizeAndWriteResults()

void ZonalStatisticsDialog::finalizeAndWriteResults() {
    OutputManager::instance()->logMessage("分区统计: 正在完成结果处理。");
    if (m_progressDialog) {
        if (m_progressDialog->isVisible()) m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    QApplication::restoreOverrideCursor();
    m_btnOk->setEnabled(true);
    m_btnCancel->setText("取消");

    if (m_gdalDataset) {
        OutputManager::instance()->logMessage("正在关闭主 GDAL 数据集。");
        GDALClose(m_gdalDataset);
        m_gdalDataset = nullptr;
    }

    QString outputPath = m_editOutputTablePath->text();
    OutputManager::instance()->logMessage(QString("尝试写入结果到文件: '%1'").arg(outputPath));

    if (outputPath.isEmpty()) {
        OutputManager::instance()->logError("输出路径为空，无法写入文件。");
        QMessageBox::warning(this, "输出错误", "未指定输出文件路径。结果未保存。");
        return;
    }

    QFile outputFile(outputPath);
    QMessageBox::StandardButton fileExistsReply = QMessageBox::NoButton; // ++ 初始化 reply 变量 ++

    if (outputFile.exists()) {
        // QMessageBox::StandardButton reply; // <--- 在这里声明和赋值 reply
        fileExistsReply = QMessageBox::question(this, "文件已存在",
            QString("文件 '%1' 已存在。\n您想覆盖它吗？\n\n"
                "选择“是”将覆盖现有文件。\n"
                "选择“否”将尝试使用新名称保存 (例如 filename_1.csv)。\n"
                "选择“取消”将不保存结果。")
            .arg(QFileInfo(outputPath).fileName()),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (fileExistsReply == QMessageBox::Cancel) {
            OutputManager::instance()->logWarning("用户选择不覆盖或重命名已存在的文件，结果未保存。");
            QMessageBox::information(this, "结果未保存", "操作已取消，统计结果未保存。");
            return;
        }
        else if (fileExistsReply == QMessageBox::No) {
            QString originalPath = outputPath;
            QFileInfo fi(originalPath);
            QString baseName = fi.completeBaseName();
            QString suffix = fi.suffix();
            QString dirPath = fi.absolutePath();
            int i = 1;
            do {
                outputPath = QDir(dirPath).filePath(QString("%1_%2.%3").arg(baseName).arg(i++).arg(suffix));
                outputFile.setFileName(outputPath);
            } while (outputFile.exists());
            OutputManager::instance()->logMessage(QString("原文件已存在，结果将保存到新文件: '%1'").arg(outputPath));
        }
    }

    // 现在 fileExistsReply 已经有值了 (或者保持 NoButton 如果文件不存在)
    bool taskWasFullyCompleted = !m_cancelRequested.load() || (m_cancelRequested.load() && m_processedFeatureCount.load() >= m_totalFeaturesToProcess);
    // ++ 使用 fileExistsReply 进行判断 ++
    bool userAbortedSaveInDialog = (fileExistsReply == QMessageBox::Cancel);


    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        OutputManager::instance()->logError(QString("无法打开文件 '%1' 进行写入。错误: %2")
            .arg(outputPath).arg(outputFile.errorString()));
        QMessageBox::critical(this, "文件错误", "无法写入输出文件: " + outputPath + "\n错误: " + outputFile.errorString());
        return;
    }

    QTextStream outStream(&outputFile);
    outStream.setCodec("UTF-8");
    outStream << "FeatureID,ZoneValue,Min,Max,Sum,Mean,Count,Status,ErrorMessage\n";

    bool anyErrorsInProcessing = false;
    for (const ZonalStatResult& res : m_allResults) {
        outStream << res.featureId << ","
            << "\"" << res.originalZoneFieldValue.toString().replace("\"", "\"\"") << "\","
            << res.min << "," << res.max << "," << res.sum << "," << res.mean << "," << res.count << ","
            << (res.success ? "Success" : "Failed") << ","
            << "\"" << QString(res.errorMessage).replace("\"", "\"\"") << "\"\n";
        if (!res.success && !res.errorMessage.isEmpty() && res.errorMessage != "操作已被用户取消。") {
            anyErrorsInProcessing = true;
        }
    }
    outputFile.close();
    OutputManager::instance()->logMessage(QString("分区统计: 结果已写入 %1。总结果数: %2")
        .arg(outputPath).arg(m_allResults.size()));

    // --- 根据最终状态显示不同的消息框 ---
    if (userAbortedSaveInDialog) {
        // 用户在“文件已存在”对话框中明确点击了取消，之前已提示，这里不再重复。
        // 函数也已经因为 reply == QMessageBox::Cancel 而 return 了。
        // 这部分逻辑实际上不会执行到，因为上面的 if (fileExistsReply == QMessageBox::Cancel) 会提前返回。
        // 为了代码清晰，可以移除这里的 else if (userAbortedSaveInDialog)
    }
    else if (!taskWasFullyCompleted && m_cancelRequested.load()) {
        QMessageBox::StandardButton openReply = QMessageBox::question(
            this, "操作已部分完成并取消",
            QString("分区统计已取消。\n处理了 %1 / %2 个要素。\n\n结果已保存到:\n%3\n\n您想现在打开它吗？")
            .arg(m_processedFeatureCount.load()).arg(m_totalFeaturesToProcess).arg(outputPath),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (openReply == QMessageBox::Yes) { /* ... 打开文件 ... */
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
                QMessageBox::warning(this, "打开失败", "无法使用默认程序打开文件:\n" + outputPath);
            }
        }
    }
    else if (anyErrorsInProcessing) {
        QMessageBox::StandardButton openReply = QMessageBox::question(
            this, "统计完成（有错误）",
            QString("部分区域统计失败，详情已写入CSV文件。\n文件已保存到:\n%1\n\n您想现在打开它吗？")
            .arg(outputPath),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (openReply == QMessageBox::Yes) { /* ... 打开文件 ... */
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
                QMessageBox::warning(this, "打开失败", "无法使用默认程序打开文件:\n" + outputPath);
            }
        }
    }
    else { // 完全成功
        QMessageBox successMsgBox(this);
        successMsgBox.setWindowTitle("成功");
        successMsgBox.setIcon(QMessageBox::Information);
        successMsgBox.setText("分区统计已成功完成！");
        successMsgBox.setInformativeText(QString("所有 %1 个要素均已处理完毕。\n结果已保存到:\n%2")
            .arg(m_totalFeaturesToProcess).arg(outputPath));
        QPushButton* openButton = successMsgBox.addButton("打开文件", QMessageBox::ActionRole);
        successMsgBox.addButton("确定", QMessageBox::AcceptRole);
        successMsgBox.exec();
        if (successMsgBox.clickedButton() == openButton) { /* ... 打开文件 ... */
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
                QMessageBox::warning(this, "打开失败", "无法使用默认程序打开文件:\n" + outputPath);
            }
        }
    }

    // 只有当用户没有在“文件已存在”对话框中点击“取消”时，才关闭主对话框
    if (!userAbortedSaveInDialog) {
        accept();
    }
}