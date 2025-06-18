// SpatialJoinDialog.cpp
#include "SpatialJoinDialog.h"
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
#include <qgsvectorlayer.h>
#include <qgsfields.h>
#include <qgsapplication.h>
#include <qgsprocessingregistry.h>
#include <qgsprocessingalgorithm.h>
#include <qgsprocessingcontext.h>
#include <qgsprocessingfeedback.h>
#include "Output_Manager.h"
#include <QDebug>
#include <memory> // For std::unique_ptr

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "ogr_geometry.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"

SpatialJoinDialog::SpatialJoinDialog(QWidget* parent)
    : QDialog(parent), m_currentTargetLayer(nullptr), m_currentJoinLayer(nullptr)
{
    setWindowTitle("空间连接 (点落入面)");
    setMinimumWidth(550);
    setupUI();
    populateTargetLayerComboBox();
    populateJoinLayerComboBox();

    // 连接信号
    connect(m_targetLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SpatialJoinDialog::onTargetLayerChanged);
    connect(m_joinLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SpatialJoinDialog::onJoinLayerChanged);
}

SpatialJoinDialog::~SpatialJoinDialog()
{
}

void SpatialJoinDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    int row = 0;
    // 1. 目标图层 (面)
    gridLayout->addWidget(new QLabel("目标图层 (面，如地块):", this), row, 0);
    m_targetLayerCombo = new QComboBox(this);
    gridLayout->addWidget(m_targetLayerCombo, row, 1);
    // m_btnSelectTargetLayerFile = new QPushButton("从文件...", this); // 暂不实现文件选择
    // gridLayout->addWidget(m_btnSelectTargetLayerFile, row, 2);
    row++;
    m_lblSelectedTargetLayerInfo = new QLabel("当前选择: 无", this);
    gridLayout->addWidget(m_lblSelectedTargetLayerInfo, row, 0, 1, 3);

    row++;
    // 2. 连接图层 (点，如POI)
    gridLayout->addWidget(new QLabel("连接图层 (点，如POI):", this), row, 0);
    m_joinLayerCombo = new QComboBox(this);
    gridLayout->addWidget(m_joinLayerCombo, row, 1);
    // m_btnSelectJoinLayerFile = new QPushButton("从文件...", this); // 暂不实现
    // gridLayout->addWidget(m_btnSelectJoinLayerFile, row, 2);
    row++;
    m_lblSelectedJoinLayerInfo = new QLabel("当前选择: 无", this);
    gridLayout->addWidget(m_lblSelectedJoinLayerInfo, row, 0, 1, 3);

    row++;
    // 3. POI ID 来源字段
    m_lblPoiIdSourceField = new QLabel("选择POI的ID字段:", this);
    gridLayout->addWidget(m_lblPoiIdSourceField, row, 0);
    m_poiIdSourceFieldCombo = new QComboBox(this);
    m_poiIdSourceFieldCombo->setToolTip("选择连接图层中哪个字段的值将赋给目标图层的新PoiId字段");
    gridLayout->addWidget(m_poiIdSourceFieldCombo, row, 1, 1, 2);

    row++;
    // 4. 输出图层中新字段的名称
    m_lblOutputPoiIdFieldName = new QLabel("输出图层PoiId字段名:", this);
    gridLayout->addWidget(m_lblOutputPoiIdFieldName, row, 0);
    m_editOutputPoiIdFieldName = new QLineEdit("PoiId", this); // 默认字段名
    gridLayout->addWidget(m_editOutputPoiIdFieldName, row, 1, 1, 2);

    row++;
    // 5. 输出图层路径
    gridLayout->addWidget(new QLabel("输出图层路径:", this), row, 0);
    m_editOutputLayerPath = new QLineEdit(this);
    m_editOutputLayerPath->setPlaceholderText("选择输出文件路径...");
    gridLayout->addWidget(m_editOutputLayerPath, row, 1);
    m_btnSelectOutputLayer = new QPushButton("浏览...", this);
    gridLayout->addWidget(m_btnSelectOutputLayer, row, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    // 6. 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnOk = new QPushButton("确定", this);
    m_btnCancel = new QPushButton("取消", this);
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // 连接信号
    // connect(m_btnSelectTargetLayerFile, &QPushButton::clicked, this, &SpatialJoinDialog::onSelectTargetLayerClicked);
    // connect(m_btnSelectJoinLayerFile, &QPushButton::clicked, this, &SpatialJoinDialog::onSelectJoinLayerClicked);
    connect(m_btnSelectOutputLayer, &QPushButton::clicked, this, &SpatialJoinDialog::onSelectOutputLayerClicked);
    connect(m_btnOk, &QPushButton::clicked, this, &SpatialJoinDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void SpatialJoinDialog::populateTargetLayerComboBox()
{
    m_targetLayerCombo->clear();
    m_targetLayerCombo->addItem("--- 选择目标面图层 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto vlayer = qobject_cast<QgsVectorLayer*>(layer)) {
                Qgis::WkbType wkbType = vlayer->wkbType(); // 获取图层的WKB类型
                Qgis::GeometryType geomType = QgsWkbTypes::geometryType(wkbType); // 解析几何大类

                if (geomType == Qgis::GeometryType::Polygon) { // 只显示面图层
                    m_targetLayerCombo->addItem(vlayer->name(), QVariant::fromValue(vlayer));
                }
            }
    }
    onTargetLayerChanged();
}

void SpatialJoinDialog::populateJoinLayerComboBox()
{
    m_joinLayerCombo->clear();
    m_joinLayerCombo->addItem("--- 选择连接点图层 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto vlayer = qobject_cast<QgsVectorLayer*>(layer)) {
                Qgis::WkbType wkbType = vlayer->wkbType(); // 获取图层的WKB类型
                Qgis::GeometryType geomType = QgsWkbTypes::geometryType(wkbType); // 解析几何大类
                if (geomType == Qgis::GeometryType::Point) { // 只显示点图层
                    m_joinLayerCombo->addItem(vlayer->name(), QVariant::fromValue(vlayer));
                }
            }
    }
    onJoinLayerChanged();
}

void SpatialJoinDialog::populatePoiIdFieldComboBox()
{
    m_poiIdSourceFieldCombo->clear();
    m_poiIdSourceFieldCombo->addItem("--- (自动ID或不选择) ---", QString()); // 空字符串表示不使用特定字段
    if (m_currentJoinLayer) {
        const QgsFields fields = m_currentJoinLayer->fields();
        for (int i = 0; i < fields.count(); ++i) {
            // 通常ID字段是整数或长整型，但也允许文本
            m_poiIdSourceFieldCombo->addItem(fields.field(i).name(), fields.field(i).name());
        }
    }
}


void SpatialJoinDialog::onSelectTargetLayerClicked() { /* ...暂不实现文件选择... */ }
void SpatialJoinDialog::onSelectJoinLayerClicked() { /* ...暂不实现文件选择... */ }

void SpatialJoinDialog::onTargetLayerChanged()
{
    m_currentTargetLayer = nullptr;
    if (m_targetLayerCombo->currentIndex() > 0) {
        QVariant data = m_targetLayerCombo->itemData(m_targetLayerCombo->currentIndex());
        QgsVectorLayer* layer = qvariant_cast<QgsVectorLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentTargetLayer = layer;
            m_lblSelectedTargetLayerInfo->setText(QString("当前目标: %1").arg(layer->name()));
        }
    }
    else {
        m_lblSelectedTargetLayerInfo->setText("当前目标: 无");
    }
    // (可以加更新默认输出名逻辑)
}

void SpatialJoinDialog::onJoinLayerChanged()
{
    m_currentJoinLayer = nullptr;
    if (m_joinLayerCombo->currentIndex() > 0) {
        QVariant data = m_joinLayerCombo->itemData(m_joinLayerCombo->currentIndex());
        QgsVectorLayer* layer = qvariant_cast<QgsVectorLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentJoinLayer = layer;
            m_lblSelectedJoinLayerInfo->setText(QString("当前连接: %1").arg(layer->name()));
        }
    }
    else {
        m_lblSelectedJoinLayerInfo->setText("当前连接: 无");
    }
    populatePoiIdFieldComboBox(); // 连接图层变了，可选的ID字段也要变
    // (可以加更新默认输出名逻辑)
}

void SpatialJoinDialog::onSelectOutputLayerClicked()
{
    QString currentPath = m_editOutputLayerPath->text();
    QString defaultDir;

    if (!currentPath.isEmpty() && QFileInfo(currentPath).exists()) {
        defaultDir = QFileInfo(currentPath).absolutePath();
    }
    else if (m_currentTargetLayer && m_currentTargetLayer->isValid() && !m_currentTargetLayer->source().isEmpty()) {
        // 如果当前路径为空，尝试使用目标图层的目录作为默认目录
        defaultDir = QFileInfo(m_currentTargetLayer->source()).absolutePath();
    }
    else {
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    // 建议的文件名（可以基于目标图层和连接图层生成）
    QString suggestedName;
    if (m_currentTargetLayer && m_currentJoinLayer) {
        suggestedName = QString("%1_joined_with_%2.shp")
            .arg(QFileInfo(m_currentTargetLayer->name()).completeBaseName())
            .arg(QFileInfo(m_currentJoinLayer->name()).completeBaseName());
    }
    else if (m_currentTargetLayer) {
        suggestedName = QString("%1_joined.shp")
            .arg(QFileInfo(m_currentTargetLayer->name()).completeBaseName());
    }
    else {
        suggestedName = "spatial_join_output.shp";
    }
    QString defaultFilePath = QDir(defaultDir).filePath(suggestedName);


    QString filter = "ESRI Shapefiles (*.shp);;GeoPackage (*.gpkg);;All files (*.*)";
    QString selectedFilePath = QFileDialog::getSaveFileName(this,
        "选择输出图层文件路径",
        defaultFilePath, // 使用包含建议文件名的完整路径
        filter);

    if (!selectedFilePath.isEmpty()) {
        // 检查并添加默认后缀 (如果用户选择的过滤器是 "All files" 且没有输入后缀)
        QFileInfo fi(selectedFilePath);
        if (fi.suffix().isEmpty()) {
            // 从选择的过滤器中提取第一个支持的后缀
            QStringList filters = filter.split(";;");
            if (!filters.isEmpty()) {
                QString firstFilter = filters.first(); // e.g., "ESRI Shapefiles (*.shp)"
                if (firstFilter.contains("(*.") && firstFilter.endsWith(")")) {
                    int startIndex = firstFilter.indexOf("(*.") + 3;
                    int endIndex = firstFilter.lastIndexOf(")");
                    QString defaultSuffix = firstFilter.mid(startIndex, endIndex - startIndex);
                    if (defaultSuffix.contains(" ")) { // 如 "shp gpkg"
                        defaultSuffix = defaultSuffix.split(" ").first();
                    }
                    if (!defaultSuffix.isEmpty()) {
                        selectedFilePath += "." + defaultSuffix;
                    }
                    else {
                        selectedFilePath += ".shp"; // 最终回退
                    }
                }
                else {
                    selectedFilePath += ".shp"; // 最终回退
                }
            }
            else {
                selectedFilePath += ".shp"; // 绝对回退
            }
        }
        m_editOutputLayerPath->setText(selectedFilePath);
    }
}


void SpatialJoinDialog::onOkClicked()
{
    // --- 1. 获取参数 ---
    QString targetLayerPath = "";
    if (m_currentTargetLayer) targetLayerPath = m_currentTargetLayer->source();
    // (如果允许从文件选择，这里需要更完善的逻辑来获取路径)

    QString joinLayerPath = "";
    if (m_currentJoinLayer) joinLayerPath = m_currentJoinLayer->source();

    QString outputPath = m_editOutputLayerPath->text();
    QString outputPoiIdFieldName = m_editOutputPoiIdFieldName->text().trimmed();
    QString sourcePoiIdFieldFromJoinLayer = m_poiIdSourceFieldCombo->currentData().toString();

    // --- 参数验证 ---
    if (targetLayerPath.isEmpty()) { QMessageBox::warning(this, "输入错误", "请选择目标图层 (面)。"); return; }
    if (joinLayerPath.isEmpty()) { QMessageBox::warning(this, "输入错误", "请选择连接图层 (点)。"); return; }
    if (outputPath.isEmpty()) { QMessageBox::warning(this, "输入错误", "请输入输出图层路径。"); return; }
    if (outputPoiIdFieldName.isEmpty()) { QMessageBox::warning(this, "输入错误", "请输入输出图层中PoiId字段的名称。"); return; }
    if (sourcePoiIdFieldFromJoinLayer.isEmpty() && m_poiIdSourceFieldCombo->currentIndex() > 0) {
        // 用户选择了一个具体的字段，但我们没获取到名字（理论上不应发生）
        QMessageBox::warning(this, "输入错误", "选择的POI ID来源字段无效。"); return;
    }
    if (m_poiIdSourceFieldCombo->currentIndex() == 0) { // 用户选择了“自动ID或不选择”
        // 对于精简版，我们强制要求用户选择一个来源字段
        QMessageBox::warning(this, "输入错误", "请从POI图层中选择一个字段作为PoiId的来源。");
        return;
    }


    qDebug() << "\n====== 开始GDAL空间连接 (点落入面) ======";
    // ... (其他qDebug输出) ...

    GDALAllRegister();
    OGRRegisterAll();

    CPLSetConfigOption("SHAPE_ENCODING", "CP936"); // 或者 "GBK"

    // --- 2. 打开输入数据集 ---
    GDALDataset* poTargetDS = (GDALDataset*)GDALOpenEx(targetLayerPath.toUtf8().constData(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr);
    if (!poTargetDS) { QMessageBox::critical(this, "GDAL错误", "无法打开目标图层: " + targetLayerPath); return; }
    OGRLayer* poTargetLayer = poTargetDS->GetLayer(0);
    if (!poTargetLayer) { QMessageBox::critical(this, "GDAL错误", "无法从目标文件获取图层。"); GDALClose(poTargetDS); return; }

    GDALDataset* poJoinDS = (GDALDataset*)GDALOpenEx(joinLayerPath.toUtf8().constData(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr);
    if (!poJoinDS) { QMessageBox::critical(this, "GDAL错误", "无法打开连接图层: " + joinLayerPath); GDALClose(poTargetDS); return; }
    OGRLayer* poJoinLayer = poJoinDS->GetLayer(0);
    if (!poJoinLayer) { QMessageBox::critical(this, "GDAL错误", "无法从连接文件获取图层。"); GDALClose(poJoinDS); GDALClose(poTargetDS); return; }

    // --- 3. 创建输出数据集和图层 ---
    const char* pszDriverName = "ESRI Shapefile"; // 默认Shapefile
    if (outputPath.endsWith(".gpkg", Qt::CaseInsensitive)) pszDriverName = "GPKG";
    // (可以添加更多格式判断)

    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (!poDriver) { QMessageBox::critical(this, "GDAL错误", QString("无法获取驱动: %1").arg(pszDriverName)); /*清理*/ return; }

    if (QFile::exists(outputPath)) {
        if (poDriver->Delete(outputPath.toUtf8().constData()) != OGRERR_NONE) {
            qDebug() << "Could not delete existing output file:" << outputPath << ". Attempting to overwrite if driver supports.";
        }
    }

    GDALDataset* poDstDS = poDriver->Create(outputPath.toUtf8().constData(), 0, 0, 0, GDT_Unknown, nullptr /*papszCreateOptions*/);
    if (!poDstDS) { QMessageBox::critical(this, "GDAL错误", "无法创建输出文件: " + outputPath + "\n" + CPLGetLastErrorMsg()); /*清理*/ return; }

    char** papszLayerCreationOptions = nullptr;
    if (strcmp(pszDriverName, "ESRI Shapefile") == 0) {
        papszLayerCreationOptions = CSLAddString(papszLayerCreationOptions, "ENCODING=GBK");
    }
    OGRLayer* poDstLayer = poDstDS->CreateLayer(QFileInfo(outputPath).completeBaseName().toUtf8().constData(),
        poTargetLayer->GetSpatialRef(),
        poTargetLayer->GetGeomType(),
        papszLayerCreationOptions); // 传递编码参数

    if (!poDstLayer) { QMessageBox::critical(this, "GDAL错误", "无法创建输出图层。"); /*清理*/ return; }

    // a. 复制目标图层的原始字段
    OGRFeatureDefn* poTargetFDefn = poTargetLayer->GetLayerDefn();
    for (int iField = 0; iField < poTargetFDefn->GetFieldCount(); iField++) {
        if (poDstLayer->CreateField(poTargetFDefn->GetFieldDefn(iField)) != OGRERR_NONE) {
            qWarning() << "Could not create field (from target):" << poTargetFDefn->GetFieldDefn(iField)->GetNameRef();
        }
    }
    // b. 添加新的 PoiId 字段
    //    我们需要确定 PoiId 字段的类型。最好与源POI ID字段的类型一致。
    OGRFieldDefn oPoiIdField(outputPoiIdFieldName.toUtf8().constData(), OFTString); // 默认为字符串
    int sourcePoiIdFieldIndex = poJoinLayer->GetLayerDefn()->GetFieldIndex(sourcePoiIdFieldFromJoinLayer.toUtf8().constData());
    if (sourcePoiIdFieldIndex != -1) {
        oPoiIdField.SetType(poJoinLayer->GetLayerDefn()->GetFieldDefn(sourcePoiIdFieldIndex)->GetType());
        // 可以根据需要设置宽度和精度
        // oPoiIdField.SetWidth(poJoinLayer->GetLayerDefn()->GetFieldDefn(sourcePoiIdFieldIndex)->GetWidth());
        // oPoiIdField.SetPrecision(poJoinLayer->GetLayerDefn()->GetFieldDefn(sourcePoiIdFieldIndex)->GetPrecision());
    }
    if (poDstLayer->CreateField(&oPoiIdField) != OGRERR_NONE) {
        QMessageBox::critical(this, "GDAL错误", "无法创建新的PoiId字段到输出图层。");
        /*清理*/ return;
    }

    // --- 4. 执行空间连接 ---
    OGRFeature* poTargetFeature;
    OGRFeature* poJoinFeature;
    poTargetLayer->ResetReading();

    OutputManager::instance()->logMessage("开始执行空间连接...");
    int joinCount = 0;

    while ((poTargetFeature = poTargetLayer->GetNextFeature()) != nullptr) {
        OGRGeometry* poTargetGeom = poTargetFeature->GetGeometryRef();
        QString poiIdValueToSet; // 用于存储找到的第一个匹配POI的ID值
        bool foundMatch = false;

        if (poTargetGeom != nullptr && !poTargetGeom->IsEmpty()) {
            poJoinLayer->ResetReading(); // 对每个目标要素，都重新从头遍历连接要素
            // (可选) 设置空间过滤器以提高效率，只遍历与当前目标要素范围相交的连接要素
            // poJoinLayer->SetSpatialFilter(poTargetGeom); 

            while ((poJoinFeature = poJoinLayer->GetNextFeature()) != nullptr) {
                OGRGeometry* poJoinGeom = poJoinFeature->GetGeometryRef();
                if (poJoinGeom != nullptr && !poJoinGeom->IsEmpty()) {
                    // 执行空间判断：点是否落入（或相交）面
                    if (poTargetGeom->Intersects(poJoinGeom)) {
                        // 或者用 Contains: if (poTargetGeom->Contains(poJoinGeom))

                            // 找到了一个匹配的POI
                        if (sourcePoiIdFieldIndex != -1) {
                            poiIdValueToSet = QString::fromUtf8(poJoinFeature->GetFieldAsString(sourcePoiIdFieldIndex));
                        }
                        else {
                            poiIdValueToSet = QString::number(poJoinFeature->GetFID()); // 备用：使用要素的FID
                        }
                        foundMatch = true;
                        joinCount++;
                        OGRFeature::DestroyFeature(poJoinFeature); // 销毁连接要素
                        break; // 找到了第一个匹配就跳出内层循环 (one-to-one)
                    }
                }
                OGRFeature::DestroyFeature(poJoinFeature);
            }
            // poJoinLayer->SetSpatialFilter(nullptr); // 清除空间过滤器
        }

        // 创建输出要素
        OGRFeature* poDstFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());
        poDstFeature->SetFrom(poTargetFeature, TRUE); // 复制几何和所有匹配的原始属性
        if (foundMatch) {
            poDstFeature->SetField(outputPoiIdFieldName.toUtf8().constData(), poiIdValueToSet.toUtf8().constData());
        }
        else {
            // 如果没有匹配的POI，PoiId字段可以留空或设置一个特定值
            // poDstFeature->SetFieldNull(outputPoiIdFieldName.toUtf8().constData());
        }

        if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
            qWarning() << "Failed to create feature in output layer for target FID:" << poTargetFeature->GetFID();
        }
        OGRFeature::DestroyFeature(poDstFeature);
        OGRFeature::DestroyFeature(poTargetFeature); // 销毁目标要素
    }
    OutputManager::instance()->logMessage(QString("空间连接完成，共连接 %1 个目标要素。").arg(joinCount));

    // --- 5. 清理和关闭 ---
    GDALClose(poDstDS);
    GDALClose(poJoinDS);
    GDALClose(poTargetDS);

    // --- 6. 加载结果到QGIS ---

    QString layerProviderOptions;
    if (strcmp(pszDriverName, "ESRI Shapefile") == 0) {
        // 告诉QGIS，这个Shapefile是用UTF-8编码的
        layerProviderOptions = " réelcodage=UTF-8";
    }

    QgsVectorLayer* newLayer = new QgsVectorLayer(outputPath, QFileInfo(outputPath).baseName(), "ogr");
    if (newLayer->isValid()) {
        QgsProject::instance()->addMapLayer(newLayer);
        QMessageBox::information(this, "成功", "空间连接完成！\n输出文件: " + outputPath);
        accept();
    }
    else {
        QMessageBox::critical(this, "加载错误", "空间连接结果已生成，但加载到地图失败: " + outputPath + "\n" + newLayer->error().message());
        if (newLayer) delete newLayer;
    }

    CPLSetConfigOption("SHAPE_ENCODING", nullptr); // 恢复默认，避免影响程序其他部分
}