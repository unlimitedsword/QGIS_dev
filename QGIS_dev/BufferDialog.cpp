#include "BufferDialog.h"
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
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <memory> // 用于std::unique_ptr

#include <qgsapplication.h>
#include <qgsgeometry.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsprocessingfeedback.h>
#include <qgsprocessingcontext.h>
#include <qgsprocessingalgorithm.h> // 替换qgsprocessingrunner.h
#include <qgscolorbutton.h>
#include <qgssinglesymbolrenderer.h>
#include <qgsfillsymbol.h>
#include <qgsunittypes.h>
#include <qgsfields.h> // 用于复制属性
#include <qgsdistancearea.h> // 用于单位转换和距离计算
#include <qgsprocessingregistry.h>
#include <qgsvectorfilewriter.h>
#include "Output_Manager.h"
#include <QDebug>

#include "gdal_priv.h"
#include "ogrsf_frmts.h" // OGR Simple Features Library
#include "ogr_geometry.h" // OGR Geometry
#include "ogr_spatialref.h" // OGR Spatial Reference
#include "cpl_conv.h"
#include "cpl_string.h"

BufferDialog::BufferDialog(QWidget* parent)
    : QDialog(parent), m_currentInputLayer(nullptr)
{
    setWindowTitle("缓冲区分析");
    setMinimumWidth(500);
    setupUI();
    populateInputLayerComboBox();
    populateUnitComboBox();

    // 初始更新默认输出路径
    connect(m_inputLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &BufferDialog::onInputLayerChanged);
    connect(m_spinBufferDistance, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &BufferDialog::updateDefaultOutputPath);
}

BufferDialog::~BufferDialog()
{
}

void BufferDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    // 1. 输入图层
    gridLayout->addWidget(new QLabel("输入图层:", this), 0, 0);
    m_inputLayerCombo = new QComboBox(this);
    gridLayout->addWidget(m_inputLayerCombo, 0, 1);
    m_btnSelectInputLayerFile = new QPushButton("从文件...", this);
    // gridLayout->addWidget(m_btnSelectInputLayerFile, 0, 2); // 暂时禁用文件选择，简化
    m_lblSelectedInputLayerInfo = new QLabel("当前选择: 无", this);
    gridLayout->addWidget(m_lblSelectedInputLayerInfo, 1, 0, 1, 3);

    // 2. 缓冲区参数
    gridLayout->addWidget(new QLabel("缓冲区距离:", this), 2, 0);
    m_spinBufferDistance = new QDoubleSpinBox(this);
    m_spinBufferDistance->setDecimals(2);
    m_spinBufferDistance->setMinimum(0.01);
    m_spinBufferDistance->setMaximum(1000000.0);
    m_spinBufferDistance->setValue(100.0);
    gridLayout->addWidget(m_spinBufferDistance, 2, 1);

    m_unitCombo = new QComboBox(this);
    gridLayout->addWidget(m_unitCombo, 2, 2);

    gridLayout->addWidget(new QLabel("端点平滑度 (段数):", this), 3, 0);
    m_spinSegments = new QSpinBox(this);
    m_spinSegments->setMinimum(1); // 至少1段
    m_spinSegments->setMaximum(100);
    m_spinSegments->setValue(8); // QGIS默认值
    gridLayout->addWidget(m_spinSegments, 3, 1, 1, 2);

    m_chkDissolveResult = new QCheckBox("溶解缓冲区结果", this);
    m_chkDissolveResult->setChecked(false);
    gridLayout->addWidget(m_chkDissolveResult, 4, 0, 1, 3);

    // 3. 输出图层路径
    gridLayout->addWidget(new QLabel("输出图层路径:", this), 5, 0);
    m_editOutputLayerPath = new QLineEdit(this);
    m_editOutputLayerPath->setPlaceholderText("选择输出文件路径...");
    gridLayout->addWidget(m_editOutputLayerPath, 5, 1);
    m_btnSelectOutputLayer = new QPushButton("浏览...", this);
    gridLayout->addWidget(m_btnSelectOutputLayer, 5, 2);

    // 4. 可视化参数
    QGroupBox* vizGroup = new QGroupBox("输出图层可视化", this);
    QGridLayout* vizLayout = new QGridLayout(vizGroup);

    m_lblFillColor = new QLabel("填充颜色:", vizGroup);
    m_btnFillColor = new QgsColorButton(vizGroup);
    m_btnFillColor->setDefaultColor(QColor(0, 0, 255, 100)); // 蓝色半透明
    m_btnFillColor->setColor(m_btnFillColor->defaultColor());
    vizLayout->addWidget(m_lblFillColor, 0, 0);
    vizLayout->addWidget(m_btnFillColor, 0, 1);

    m_lblFillOpacity = new QLabel("填充透明度 (0-1):", vizGroup);
    m_spinFillOpacity = new QDoubleSpinBox(vizGroup);
    m_spinFillOpacity->setDecimals(2);
    m_spinFillOpacity->setMinimum(0.0);
    m_spinFillOpacity->setMaximum(1.0);
    m_spinFillOpacity->setValue(m_btnFillColor->color().alphaF()); // 从颜色按钮同步
    connect(m_btnFillColor, &QgsColorButton::colorChanged, this, [this](const QColor& color) {
        m_spinFillOpacity->setValue(color.alphaF());
        });
    connect(m_spinFillOpacity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        QColor c = m_btnFillColor->color();
        c.setAlphaF(value);
        m_btnFillColor->setColor(c);
        });
    vizLayout->addWidget(m_lblFillOpacity, 0, 2);
    vizLayout->addWidget(m_spinFillOpacity, 0, 3);


    m_lblStrokeColor = new QLabel("边界颜色:", vizGroup);
    m_btnStrokeColor = new QgsColorButton(vizGroup);
    m_btnStrokeColor->setDefaultColor(Qt::black);
    m_btnStrokeColor->setColor(m_btnStrokeColor->defaultColor());
    vizLayout->addWidget(m_lblStrokeColor, 1, 0);
    vizLayout->addWidget(m_btnStrokeColor, 1, 1);

    m_lblStrokeWidth = new QLabel("边界宽度:", vizGroup);
    m_spinStrokeWidth = new QDoubleSpinBox(vizGroup);
    m_spinStrokeWidth->setDecimals(2);
    m_spinStrokeWidth->setMinimum(0.0);
    m_spinStrokeWidth->setValue(0.26); // QGIS默认
    vizLayout->addWidget(m_lblStrokeWidth, 1, 2);
    vizLayout->addWidget(m_spinStrokeWidth, 1, 3);

    vizGroup->setLayout(vizLayout);
    gridLayout->addWidget(vizGroup, 6, 0, 1, 3);


    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    // 5. 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnOk = new QPushButton("确定", this);
    m_btnCancel = new QPushButton("取消", this);
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // 连接信号
    // connect(m_btnSelectInputLayerFile, &QPushButton::clicked, this, &BufferDialog::onSelectInputLayerClicked); // 暂时禁用
    connect(m_btnSelectOutputLayer, &QPushButton::clicked, this, &BufferDialog::onSelectOutputLayerClicked);
    connect(m_btnOk, &QPushButton::clicked, this, &BufferDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void BufferDialog::populateInputLayerComboBox()
{
    m_inputLayerCombo->clear();
    m_inputLayerCombo->addItem("--- 从项目中选择矢量图层 ---", QVariant());
    QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers) {
            if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
                m_inputLayerCombo->addItem(vectorLayer->name(), QVariant::fromValue(vectorLayer));
            }
    }
    onInputLayerChanged(); // 更新初始状态
}

void BufferDialog::populateUnitComboBox()
{
    m_unitCombo->clear();
    // QgsUnitTypes::DistanceUnit 是 QGIS 3.x 的方式
    m_unitCombo->addItem(QgsUnitTypes::toString(Qgis::DistanceUnit::Meters), static_cast<int>(Qgis::DistanceUnit::Meters));
    m_unitCombo->addItem(QgsUnitTypes::toString(Qgis::DistanceUnit::Kilometers), static_cast<int>(Qgis::DistanceUnit::Kilometers));
    m_unitCombo->addItem(QgsUnitTypes::toString(Qgis::DistanceUnit::Feet), static_cast<int>(Qgis::DistanceUnit::Feet));
    m_unitCombo->addItem(QgsUnitTypes::toString(Qgis::DistanceUnit::Miles), static_cast<int>(Qgis::DistanceUnit::Miles));
    m_unitCombo->addItem(QgsUnitTypes::toString(Qgis::DistanceUnit::Degrees), static_cast<int>(Qgis::DistanceUnit::Degrees)); // 度 (用于地理坐标系)

    // 默认选中米
    int meterIndex = m_unitCombo->findData(static_cast<int>(Qgis::DistanceUnit::Meters));
    if (meterIndex != -1) {
        m_unitCombo->setCurrentIndex(meterIndex);
    }
}

void BufferDialog::onSelectInputLayerClicked() // 暂时未使用
{
    // ... (如果需要从文件选择，这里的逻辑类似于其他对话框) ...
}

void BufferDialog::onInputLayerChanged()
{
    m_currentInputLayer = nullptr;
    if (m_inputLayerCombo->currentIndex() > 0) {
        QVariant data = m_inputLayerCombo->currentData();
        QgsVectorLayer* layer = qvariant_cast<QgsVectorLayer*>(data);
        if (layer && QgsProject::instance()->mapLayer(layer->id())) {
            m_currentInputLayer = layer;
            m_lblSelectedInputLayerInfo->setText(QString("当前选择: %1").arg(layer->name()));
            // 尝试根据图层CRS的单位更新单位下拉框的默认值
            if (layer->crs().isValid()) {
                Qgis::DistanceUnit layerUnits = Qgis::DistanceUnit::Meters; // Default to meters
                int unitIndex = m_unitCombo->findData(static_cast<int>(layerUnits));
                if (unitIndex != -1) {
                    m_unitCombo->setCurrentIndex(unitIndex);
                }
                else if (layer->crs().isGeographic()) {
                    int degIndex = m_unitCombo->findData(static_cast<int>(Qgis::DistanceUnit::Degrees));
                    if (degIndex != -1) m_unitCombo->setCurrentIndex(degIndex);
                }
            }
        }
    }
    else {
        m_lblSelectedInputLayerInfo->setText("当前选择: 无");
    }
    updateDefaultOutputPath();
}


void BufferDialog::updateDefaultOutputPath()
{
    if (!m_currentInputLayer) {
        m_editOutputLayerPath->clear();
        return;
    }
    QFileInfo fi(m_currentInputLayer->source());
    QString baseName = fi.completeBaseName();
    if (baseName.isEmpty()) baseName = m_currentInputLayer->name().remove(QRegExp("[^a-zA-Z0-9_]")); // 如果source是内存图层

    QString dirPath = QgsProject::instance()->homePath(); // 默认保存到项目路径
    if (fi.exists()) dirPath = fi.absolutePath();

    double dist = m_spinBufferDistance->value();

    QString defaultOutputName = QString("%1_buffer_%2.%3").arg(baseName).arg(dist).arg("shp"); // 默认shp
    m_editOutputLayerPath->setText(QDir(dirPath).filePath(defaultOutputName));
}


void BufferDialog::onSelectOutputLayerClicked()
{
    QString filter = "ESRI Shapefiles (*.shp);;GeoPackage (*.gpkg);;GeoTIFF (*.tif);;All files (*.*)";
    QString defaultDir = QgsProject::instance()->homePath();

    if (defaultDir.isEmpty()) {
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString fileName = QFileDialog::getSaveFileName(this, "保存输出图层",
        m_editOutputLayerPath->text(),
        filter);
    if (!fileName.isEmpty()) {
        m_editOutputLayerPath->setText(fileName);
    }
}
// 公共接口实现
QgsVectorLayer* BufferDialog::selectedInputLayer() const { return m_currentInputLayer; }
double BufferDialog::bufferDistance() const { return m_spinBufferDistance->value(); }
Qgis::DistanceUnit BufferDialog::distanceUnits() const { return static_cast<Qgis::DistanceUnit>(m_unitCombo->currentData().toInt()); }
int BufferDialog::segments() const { return m_spinSegments->value(); }
bool BufferDialog::dissolveResult() const { return m_chkDissolveResult->isChecked(); }
QString BufferDialog::outputLayerPath() const { return m_editOutputLayerPath->text(); }
QColor BufferDialog::fillColor() const { return m_btnFillColor->color(); }
double BufferDialog::fillOpacity() const { return m_spinFillOpacity->value(); }
QColor BufferDialog::strokeColor() const { return m_btnStrokeColor->color(); }
double BufferDialog::strokeWidth() const { return m_spinStrokeWidth->value(); }


void BufferDialog::onOkClicked()
{
    QString inputPath = m_currentInputLayer ? m_currentInputLayer->source() : QString();

    double distanceValueFromUI = bufferDistance();
    Qgis::DistanceUnit unitSelectedByUser = distanceUnits(); // 我们仍用QGIS枚举，但需转为GDAL能理解的方式
    int segments = this->segments();
    bool dissolve = dissolveResult();
    QString outputPath = outputLayerPath();
    // 可视化参数在GDAL版本中暂时不直接应用到输出文件，而是加载后设置QGIS图层样式

    if (inputPath.isEmpty() || outputPath.isEmpty() || distanceValueFromUI <= 0) {
        QMessageBox::warning(this, "输入错误", "请检查所有输入参数。");
        return;
    }

    qDebug() << "\n====== 开始GDAL缓冲区分析 ======";
    // ... 其他qDebug输出 ...

    GDALAllRegister(); // 注册所有GDAL驱动
    OGRRegisterAll();  // 注册所有OGR驱动

    // 1. 打开输入矢量数据集
    GDALDataset* poSrcDS = (GDALDataset*)GDALOpenEx(inputPath.toUtf8().constData(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr);
    if (!poSrcDS) {
        QMessageBox::critical(this, "GDAL错误", "无法打开输入矢量文件: " + inputPath + "\n" + CPLGetLastErrorMsg());
        return;
    }
    OGRLayer* poSrcLayer = poSrcDS->GetLayer(0); // 假设只有一个图层
    if (!poSrcLayer) {
        QMessageBox::critical(this, "GDAL错误", "无法获取输入图层。");
        GDALClose(poSrcDS);
        return;
    }
    OGRSpatialReference* poSrcSRS = poSrcLayer->GetSpatialRef(); // 获取源SRS

    // 2. 单位和距离转换 (这是最复杂的部分)
    double distanceInSrcUnits;
    bool srcIsGeographic = false;
    if (poSrcSRS != nullptr && poSrcSRS->IsGeographic()) {
        srcIsGeographic = true;
    }

    else {
        // 需要将用户选择的单位（米、公里等）转换为源图层CRS的单位
        // 如果源是地理坐标系（单位是度），这是一个近似转换
        // 如果源是投影坐标系（单位是米/英尺），这是一个标准转换
        OGRSpatialReference oTargetSRS_ForConversion; // 临时目标SRS用于单位转换
        double conversionFactor = 1.0;

        switch (unitSelectedByUser) {
        case Qgis::DistanceUnit::Meters:     conversionFactor = 1.0; break;
        case Qgis::DistanceUnit::Kilometers: conversionFactor = 1000.0; break;
        case Qgis::DistanceUnit::Feet:       conversionFactor = 0.3048; break; // 1英尺 = 0.3048米
        case Qgis::DistanceUnit::Miles:      conversionFactor = 1609.34; break; // 1英里 = 1609.34米
        default:
            QMessageBox::warning(this, "单位警告", "不支持的缓冲区单位，将按地图单位处理。");
            distanceInSrcUnits = distanceValueFromUI;
            goto proceed_with_buffer; // 跳过复杂转换
        }

        double distanceInMeters = distanceValueFromUI * conversionFactor;

        if (srcIsGeographic) {
            // 将米近似转换为度。这是一个粗略的全局估计。
            // 1度约等于111公里 (111000米) 在赤道附近。
            // 更精确的方法需要考虑纬度。
            // OGRGeometry::Buffer 在地理坐标系下，通常期望距离也是以度为单位。
            distanceInSrcUnits = distanceInMeters / 111000.0;
            qDebug() << "Input is geographic. Buffer distance approx." << distanceInSrcUnits << "degrees.";
        }
        else if (poSrcSRS != nullptr) {
            // 投影坐标系，获取其线性单位转换为米后的比例因子
            double linearUnitsToMeters = poSrcSRS->GetLinearUnits(); // 返回1个输入单位等于多少米
            if (linearUnitsToMeters > 0) {
                distanceInSrcUnits = distanceInMeters / linearUnitsToMeters;
                qDebug() << "Input is projected. Linear unit factor:" << linearUnitsToMeters
                    << "Buffer distance in layer units:" << distanceInSrcUnits;
            }
            else {
                qDebug() << "Could not determine linear unit factor for projected CRS. Using meters directly.";
                distanceInSrcUnits = distanceInMeters; // 假设投影单位是米
            }
        }
        else {
            qDebug() << "Source CRS unknown. Assuming distance is in target units for buffer.";
            distanceInSrcUnits = distanceValueFromUI; // 无法确定单位，按用户输入
        }
    }
proceed_with_buffer:;


    // 3. 创建输出Shapefile驱动
    const char* pszDriverName = "ESRI Shapefile"; // 或根据outputPath后缀选择
    if (outputPath.endsWith(".gpkg", Qt::CaseInsensitive)) pszDriverName = "GPKG";

    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (poDriver == nullptr) {
        QMessageBox::critical(this, "GDAL错误", QString("无法获取驱动: %1").arg(pszDriverName));
        GDALClose(poSrcDS);
        return;
    }

    // 4. 创建输出数据集 (如果文件已存在则先删除)
    if (QFile::exists(outputPath)) {
        if (GDALDeleteDataset(poDriver, outputPath.toUtf8().constData()) != CE_None) {
            qDebug() << "Could not delete existing output file:" << outputPath;
        }
    }
    GDALDataset* poDstDS = poDriver->Create(outputPath.toUtf8().constData(), 0, 0, 0, GDT_Unknown, nullptr);
    if (poDstDS == nullptr) {
        QMessageBox::critical(this, "GDAL错误", "无法创建输出文件: " + outputPath + "\n" + CPLGetLastErrorMsg());
        GDALClose(poSrcDS);
        return;
    }

    // 5. 创建输出图层，定义为面类型，并复制源图层的字段和SRS
    OGRLayer* poDstLayer = poDstDS->CreateLayer(QFileInfo(outputPath).completeBaseName().toUtf8().constData(),
        poSrcSRS, // 使用源图层的SRS
        wkbPolygon, nullptr); // 几何类型为面
    if (poDstLayer == nullptr) {
        QMessageBox::critical(this, "GDAL错误", "无法创建输出图层。");
        GDALClose(poDstDS); GDALClose(poSrcDS);
        return;
    }
    OGRFeatureDefn* poSrcFDefn = poSrcLayer->GetLayerDefn();
    for (int iField = 0; iField < poSrcFDefn->GetFieldCount(); iField++) {
        OGRFieldDefn* poFieldDefn = poSrcFDefn->GetFieldDefn(iField);
        if (poDstLayer->CreateField(poFieldDefn) != OGRERR_NONE) {
            qWarning() << "Could not create field:" << poFieldDefn->GetNameRef();
        }
    }

    // 6. 遍历要素并生成缓冲区
    OGRFeature* poSrcFeature;
    OGRFeature* poDstFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());
    QList<OGRGeometry*> bufferedGeometries; // 用于溶解

    poSrcLayer->ResetReading();
    while ((poSrcFeature = poSrcLayer->GetNextFeature()) != nullptr) {
        OGRGeometry* poSrcGeom = poSrcFeature->GetGeometryRef();
        if (poSrcGeom != nullptr) {
            // 核心：调用Buffer方法
            // 第二个参数是 segments (quadsecs in OGR)
            OGRGeometry* poBufferedGeom = poSrcGeom->Buffer(distanceInSrcUnits, segments);

            if (poBufferedGeom != nullptr && !poBufferedGeom->IsEmpty()) {
                if (dissolve) {
                    bufferedGeometries.append(poBufferedGeom); // 先收集，不直接写入
                }
                else {
                    poDstFeature->SetGeometry(poBufferedGeom);
                    for (int iField = 0; iField < poSrcFDefn->GetFieldCount(); iField++) {
                        poDstFeature->SetField(iField, poSrcFeature->GetRawFieldRef(iField));
                    }
                    if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
                        qWarning() << "Failed to create feature in output layer.";
                    }
                    OGRGeometryFactory::destroyGeometry(poBufferedGeom);
                }
            }
            else if (poBufferedGeom) {
                OGRGeometryFactory::destroyGeometry(poBufferedGeom);
            }
        }
        OGRFeature::DestroyFeature(poSrcFeature);
    }

    // 7. 如果需要溶解
    if (dissolve && !bufferedGeometries.isEmpty()) {
        qDebug() << "Dissolving" << bufferedGeometries.size() << "geometries...";
        OGRGeometryCollection oGeomColl;
        for (OGRGeometry* geom : bufferedGeometries) {
            oGeomColl.addGeometry(geom);
            OGRGeometryFactory::destroyGeometry(geom); // addGeometry会克隆，所以可以销毁原件
        }
        bufferedGeometries.clear();

        OGRGeometry* poDissolvedGeom = oGeomColl.UnionCascaded(); // 或者 oGeomColl.UnaryUnion();
        if (poDissolvedGeom && !poDissolvedGeom->IsEmpty()) {
            poDstFeature->SetGeometry(poDissolvedGeom);
            // 溶解后属性如何处理？这里不设置
            if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
                qWarning() << "Failed to create dissolved feature in output layer.";
            }
            OGRGeometryFactory::destroyGeometry(poDissolvedGeom);
        }
        else if (poDissolvedGeom) {
            OGRGeometryFactory::destroyGeometry(poDissolvedGeom);
        }
    }
    OGRFeature::DestroyFeature(poDstFeature);

    // 8. 清理和关闭
    GDALClose(poDstDS); // 关闭输出数据集以写入磁盘
    GDALClose(poSrcDS);

    // 9. 加载结果并设置样式 (这部分与QGIS Processing的方案类似)
    QgsVectorLayer* newLayer = new QgsVectorLayer(outputPath, QFileInfo(outputPath).baseName(), "ogr");
    if (newLayer->isValid()) {
        std::unique_ptr<QgsFillSymbol> fillSymbolPtr = QgsFillSymbol::createSimple({
            {"color", fillColor().name(QColor::HexArgb)},
            {"style", "solid"},
            {"outline_color", strokeColor().name(QColor::HexArgb)},
            {"outline_width", QString::number(strokeWidth())}
            });
        fillSymbolPtr->setOpacity(fillOpacity());
        QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(fillSymbolPtr.get());
        newLayer->setRenderer(renderer);
        fillSymbolPtr.release(); // 交给renderer管理，防止析构

        QgsProject::instance()->addMapLayer(newLayer);
        QMessageBox::information(this, "成功", "GDAL缓冲区分析完成！\n输出文件: " + outputPath);
        accept();
    }
    else {
        QMessageBox::critical(this, "加载错误", "缓冲区已生成，但无法加载到地图: " + outputPath + "\n" + newLayer->error().message());
        delete newLayer;
    }
}
