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
    gridLayout->addWidget(m_btnSelectInputLayerFile, 0, 2); // 暂时禁用文件选择，简化
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
    connect(m_btnSelectInputLayerFile, &QPushButton::clicked, this, &BufferDialog::onSelectInputLayerClicked);
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

void BufferDialog::onSelectInputLayerClicked()
{
    QString filter = "ESRI Shapefiles (*.shp);;GeoPackage (*.gpkg);;所有文件 (*.*)";
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择输入矢量文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        filter
    );
    if (filePath.isEmpty())
        return;

    // 尝试加载为矢量图层
    std::unique_ptr<QgsVectorLayer> tempLayer(new QgsVectorLayer(filePath, QFileInfo(filePath).baseName(), "ogr"));
    if (!tempLayer->isValid()) {
        QMessageBox::critical(this, "加载错误", "无法加载所选文件为有效矢量图层:\n" + filePath);
        return;
    }

    // 更新UI显示
    m_lblSelectedInputLayerInfo->setText(QString("当前选择: %1 (文件)").arg(QFileInfo(filePath).fileName()));
    m_inputLayerCombo->setCurrentIndex(0); // 选中“--- 从项目中选择矢量图层 ---”

    // 设置当前输入图层为临时加载的图层
    m_currentInputLayer = tempLayer.release(); // 交给成员变量管理

    // 更新单位下拉框（根据图层CRS）
    if (m_currentInputLayer->crs().isValid()) {
        Qgis::DistanceUnit layerUnits = Qgis::DistanceUnit::Meters; // 默认米
        int unitIndex = m_unitCombo->findData(static_cast<int>(layerUnits));
        if (unitIndex != -1) {
            m_unitCombo->setCurrentIndex(unitIndex);
        }
        else if (m_currentInputLayer->crs().isGeographic()) {
            int degIndex = m_unitCombo->findData(static_cast<int>(Qgis::DistanceUnit::Degrees));
            if (degIndex != -1) m_unitCombo->setCurrentIndex(degIndex);
        }
    }

    updateDefaultOutputPath();
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
    Qgis::DistanceUnit unitSelectedByUser = distanceUnits();
    int segments = this->segments();
    bool dissolve = dissolveResult();
    QString outputPath = outputLayerPath();

    if (inputPath.isEmpty() || outputPath.isEmpty() || distanceValueFromUI <= 0) {
        QMessageBox::warning(this, "输入错误", "请检查所有输入参数。");
        return;
    }

    qDebug() << "\n====== 开始GDAL缓冲区分析 ======";
    qDebug() << "输入图层:" << inputPath;
    qDebug() << "UI距离值:" << distanceValueFromUI;
    qDebug() << "UI单位:" << QgsUnitTypes::toString(unitSelectedByUser) << "(" << static_cast<int>(unitSelectedByUser) << ")";
    qDebug() << "段数:" << segments;
    qDebug() << "溶解:" << dissolve;
    qDebug() << "输出路径:" << outputPath;


    GDALAllRegister();
    OGRRegisterAll();

    GDALDataset* poSrcDS = (GDALDataset*)GDALOpenEx(inputPath.toUtf8().constData(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr);
    if (!poSrcDS) {
        QMessageBox::critical(this, "GDAL错误", "无法打开输入矢量文件: " + inputPath + "\n" + CPLGetLastErrorMsg());
        return;
    }
    OGRLayer* poSrcLayer = poSrcDS->GetLayer(0);
    if (!poSrcLayer) {
        QMessageBox::critical(this, "GDAL错误", "无法获取输入图层。");
        GDALClose(poSrcDS);
        return;
    }
    OGRSpatialReference* poSrcSRS = poSrcLayer->GetSpatialRef();

    // --- 重点修改区域开始 ---
    double distanceInSrcUnits = distanceValueFromUI; // 默认值，如果无法进行转换则使用

    if (poSrcSRS != nullptr) {
        double distanceInMeters_fromUserSelection; // 用户选择的单位转换成米

        // 步骤1: 将用户界面上的距离和单位统一转换为米 (除非用户选择的就是度)
        bool userSelectedDegrees = (unitSelectedByUser == Qgis::DistanceUnit::Degrees);

        if (!userSelectedDegrees) {
            switch (unitSelectedByUser) {
            case Qgis::DistanceUnit::Meters:
                distanceInMeters_fromUserSelection = distanceValueFromUI;
                break;
            case Qgis::DistanceUnit::Kilometers:
                distanceInMeters_fromUserSelection = distanceValueFromUI * 1000.0;
                break;
            case Qgis::DistanceUnit::Feet:
                distanceInMeters_fromUserSelection = distanceValueFromUI * 0.3048;
                break;
            case Qgis::DistanceUnit::Miles:
                distanceInMeters_fromUserSelection = distanceValueFromUI * 1609.34;
                break;
            default:
                QMessageBox::warning(this, "单位警告", "未知的缓冲区单位，将按输入值直接处理。");
                // distanceInSrcUnits 保持为 distanceValueFromUI
                goto end_unit_conversion_label; // 直接跳到转换结束
            }
            qDebug() << "用户输入已转换为米:" << distanceInMeters_fromUserSelection << "m";
        }

        // 步骤2: 根据图层坐标系，将米（或度）转换为图层实际单位
        if (poSrcSRS->IsGeographic()) {
            if (userSelectedDegrees) {
                distanceInSrcUnits = distanceValueFromUI; // 用户选了度，图层是地理坐标系，直接用
                qDebug() << "图层为地理坐标系，用户单位为度。缓冲区距离 (度):" << distanceInSrcUnits;
            }
            else {
                // 图层是地理坐标系，用户单位是米/公里等线性单位，需转为度
                // 1度约等于111195米 (这是一个平均值，更精确的计算会考虑纬度)
                // QGIS内部进行此类操作时可能会临时投影到合适的等距方位投影
                // GDAL OGR_G_Buffer 在地理坐标系上期望距离参数也是度
                distanceInSrcUnits = distanceInMeters_fromUserSelection / 111195.0;
                qDebug() << "图层为地理坐标系，用户选择线性单位。缓冲区距离 (近似度):" << distanceInSrcUnits;
            }
        }
        else { // 图层是投影坐标系
            if (userSelectedDegrees) {
                // 用户选了度，但图层是投影坐标系。这种情况通常不建议。
                // QGIS桌面通常不允许为投影图层选择“度”作为缓冲区单位。
                // 我们这里采取一个策略：警告并按原始值处理，期望用户知道其含义。
                QMessageBox::warning(this, "单位不匹配警告", "为投影坐标系图层选择“度”作为缓冲区单位可能产生非预期结果。将直接使用输入值作为图层单位的距离。");
                distanceInSrcUnits = distanceValueFromUI;
                qDebug() << "图层为投影坐标系，用户单位为度。缓冲区距离 (按图层单位):" << distanceInSrcUnits;
            }
            else {
                // 图层是投影坐标系，用户单位是米/公里等线性单位
                // 获取投影坐标系的线性单位对应的米数 (例如，如果单位是英尺，返回0.3048)
                double metersPerLayerUnit = poSrcSRS->GetLinearUnits(nullptr); // 获取1个图层单位等于多少米
                if (metersPerLayerUnit > 1e-9) { // 避免除以0或非常小的值
                    distanceInSrcUnits = distanceInMeters_fromUserSelection / metersPerLayerUnit;
                    qDebug() << "图层为投影坐标系。1图层单位=" << metersPerLayerUnit << "米。缓冲区距离 (图层单位):" << distanceInSrcUnits;
                }
                else {
                    qDebug() << "无法确定投影坐标系的有效线性单位因子 (因子=" << metersPerLayerUnit
                        << ")。假设图层单位是米。";
                    distanceInSrcUnits = distanceInMeters_fromUserSelection; // 回退：假设图层单位是米
                }
            }
        }
    }
    else { // poSrcSRS is NULL (图层没有坐标系信息)
        qDebug() << "输入图层无坐标系信息。将直接使用用户输入值 (" << distanceValueFromUI
            << ") 和单位 (" << QgsUnitTypes::toString(unitSelectedByUser)
            << ") 作为缓冲区的距离。";
        // 在这种情况下，如果用户选了公里但图层单位是米，结果会不正确。
        // 但没有CRS信息，无法做更智能的转换。
        // 为了与之前的逻辑（和QGIS某些工具的行为）保持一定一致性，
        // 如果是常见线性单位，我们还是先转成米，再假设图层单位是米
        // 如果是度，就直接用。这是一个折衷。
        switch (unitSelectedByUser) {
        case Qgis::DistanceUnit::Meters:     distanceInSrcUnits = distanceValueFromUI; break;
        case Qgis::DistanceUnit::Kilometers: distanceInSrcUnits = distanceValueFromUI * 1000.0; break; // 这里产生100000
        case Qgis::DistanceUnit::Feet:       distanceInSrcUnits = distanceValueFromUI * 0.3048; break;
        case Qgis::DistanceUnit::Miles:      distanceInSrcUnits = distanceValueFromUI * 1609.34; break;
        case Qgis::DistanceUnit::Degrees:    distanceInSrcUnits = distanceValueFromUI; break; // 直接用度
        default: distanceInSrcUnits = distanceValueFromUI; break; // 其他未知单位直接用
        }
        qDebug() << "无CRS，最终采用的缓冲距离(假设目标单位兼容米或度):" << distanceInSrcUnits;
        // 如果图层是度为单位但无CRS，而用户选了公里，上面的转换会导致距离值很大（如100km -> 100000）
        // 这时GDAL Buffer会将其解释为100000度，这是不正确的。
        // 最安全的做法是，如果无CRS，就直接用 distanceValueFromUI，并警告用户。
        // distanceInSrcUnits = distanceValueFromUI; // Revert to this simpler logic for no-CRS
        // QMessageBox::warning(this, "坐标系缺失", "输入图层缺少坐标系信息。缓冲区距离将直接使用您输入的值，请确保其单位与图层内部单位一致。");

    }

end_unit_conversion_label:;
    qDebug() << "最终用于 OGR_G_Buffer 的距离值:" << distanceInSrcUnits;
    // --- 重点修改区域结束 ---


    const char* pszDriverName = "ESRI Shapefile";
    if (outputPath.endsWith(".gpkg", Qt::CaseInsensitive)) pszDriverName = "GPKG";

    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (poDriver == nullptr) {
        QMessageBox::critical(this, "GDAL错误", QString("无法获取驱动: %1").arg(pszDriverName));
        GDALClose(poSrcDS);
        return;
    }

    if (QFile::exists(outputPath)) {
        if (GDALDeleteDataset(poDriver, outputPath.toUtf8().constData()) != CE_None) {
            qDebug() << "无法删除已存在的输出文件:" << outputPath << CPLGetLastErrorMsg();
            // 可以选择报错返回，或者继续尝试覆盖 (Create可能失败)
        }
    }
    GDALDataset* poDstDS = poDriver->Create(outputPath.toUtf8().constData(), 0, 0, 0, GDT_Unknown, nullptr);
    if (poDstDS == nullptr) {
        QMessageBox::critical(this, "GDAL错误", "无法创建输出文件: " + outputPath + "\n" + CPLGetLastErrorMsg());
        GDALClose(poSrcDS);
        return;
    }

    OGRLayer* poDstLayer = poDstDS->CreateLayer(QFileInfo(outputPath).completeBaseName().toUtf8().constData(),
        poSrcSRS,
        wkbPolygon, nullptr);
    if (poDstLayer == nullptr) {
        QMessageBox::critical(this, "GDAL错误", "无法创建输出图层。");
        GDALClose(poDstDS); GDALClose(poSrcDS);
        return;
    }
    OGRFeatureDefn* poSrcFDefn = poSrcLayer->GetLayerDefn();
    for (int iField = 0; iField < poSrcFDefn->GetFieldCount(); iField++) {
        OGRFieldDefn* poFieldDefn = poSrcFDefn->GetFieldDefn(iField);
        if (poDstLayer->CreateField(poFieldDefn) != OGRERR_NONE) {
            qWarning() << "无法创建字段:" << poFieldDefn->GetNameRef();
        }
    }

    OGRFeature* poSrcFeature;
    OGRFeature* poDstFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());
    QList<OGRGeometry*> bufferedGeometries;

    poSrcLayer->ResetReading();
    int featureCount = 0;
    while ((poSrcFeature = poSrcLayer->GetNextFeature()) != nullptr) {
        featureCount++;
        OGRGeometry* poSrcGeom = poSrcFeature->GetGeometryRef();
        if (poSrcGeom != nullptr && !poSrcGeom->IsEmpty()) {
            OGRGeometry* poBufferedGeom = poSrcGeom->Buffer(distanceInSrcUnits, segments);

            if (poBufferedGeom != nullptr && !poBufferedGeom->IsEmpty()) {
                if (dissolve) {
                    bufferedGeometries.append(poBufferedGeom->clone()); // 克隆以备溶解
                    OGRGeometryFactory::destroyGeometry(poBufferedGeom);
                }
                else {
                    poDstFeature->SetFrom(poSrcFeature, TRUE); // 复制属性
                    poDstFeature->SetGeometryDirectly(poBufferedGeom); // poBufferedGeom所有权转移
                    if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
                        qWarning() << "无法在输出图层创建要素。";
                    }
                    // poDstFeature->SetGeometry(nullptr) might be needed if reusing poDstFeature
                    // but SetGeometryDirectly transfers ownership, so it's fine.
                }
            }
            else if (poBufferedGeom) { // Buffer可能返回空几何
                OGRGeometryFactory::destroyGeometry(poBufferedGeom);
            }
        }
        OGRFeature::DestroyFeature(poSrcFeature);
    }
    qDebug() << "处理了" << featureCount << "个要素。";


    if (dissolve && !bufferedGeometries.isEmpty()) {
        qDebug() << "开始溶解" << bufferedGeometries.size() << "个缓冲区几何...";
        // OGRGeometryCollection oGeomColl; // 旧方法
        // for (OGRGeometry* geom : bufferedGeometries) {
        //    oGeomColl.addGeometry(geom); // addGeometry clones
        //    OGRGeometryFactory::destroyGeometry(geom); // 所以可以销毁原件
        // }
        // bufferedGeometries.clear();
        // OGRGeometry* poDissolvedGeom = oGeomColl.UnionCascaded();

        // 更高效的溶解方法，特别是对于大量几何图形
        OGRGeometry* poDissolvedGeom = nullptr;
        if (!bufferedGeometries.isEmpty()) {
            poDissolvedGeom = bufferedGeometries.takeFirst(); // 从第一个开始
            for (OGRGeometry* geom : bufferedGeometries) {
                OGRGeometry* tempUnion = poDissolvedGeom->Union(geom);
                OGRGeometryFactory::destroyGeometry(poDissolvedGeom);
                OGRGeometryFactory::destroyGeometry(geom);
                poDissolvedGeom = tempUnion;
                if (!poDissolvedGeom) { // Union失败
                    qWarning() << "溶解过程中Union操作失败。";
                    break;
                }
            }
        }
        bufferedGeometries.clear();


        if (poDissolvedGeom && !poDissolvedGeom->IsEmpty()) {
            // 对于溶解后的单个要素，属性如何处理？通常是清空或赋一个代表性值。
            // 这里我们不设置属性，只设置几何。
            poDstFeature->SetGeometryDirectly(poDissolvedGeom); // 所有权转移
            if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
                qWarning() << "无法在输出图层创建溶解后的要素。";
            }
        }
        else if (poDissolvedGeom) { //可能是空的union结果
            OGRGeometryFactory::destroyGeometry(poDissolvedGeom);
            qDebug() << "溶解结果为空几何。";
        }
        qDebug() << "溶解完成。";
    }
    else if (dissolve && bufferedGeometries.isEmpty()) {
        qDebug() << "请求溶解但没有有效的缓冲区几何体可供溶解。";
    }

    OGRFeature::DestroyFeature(poDstFeature);

    GDALClose(poDstDS);
    GDALClose(poSrcDS);

    QgsVectorLayer* newLayer = new QgsVectorLayer(outputPath, QFileInfo(outputPath).baseName(), "ogr");
    if (newLayer->isValid()) {
        std::unique_ptr<QgsFillSymbol> fillSymbol(QgsFillSymbol::createSimple({
            {"color", fillColor().name(QColor::HexArgb)},
            {"style", "solid"},
            {"outline_color", strokeColor().name(QColor::HexArgb)},
            {"outline_width", QString::number(strokeWidth())},
            {"outline_style", "solid"} // 添加边界样式以确保可见
            }));
        // fillSymbol->setOpacity(fillOpacity()); // QgsColorButton已经包含alpha，所以这里不需要单独设置了
                                                // 如果颜色按钮的alphaF()与m_spinFillOpacity不同步则需要

        QColor currentFillColor = fillColor(); // 从按钮获取
        fillSymbol->setColor(currentFillColor); // 设置包含透明度的颜色
        // fillSymbol->setOpacity(currentFillColor.alphaF()); // 或者这样明确设置，如果上面setColor不处理alpha的话


        QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(fillSymbol.release()); // release所有权
        newLayer->setRenderer(renderer);
        newLayer->triggerRepaint(); // 确保刷新

        QgsProject::instance()->addMapLayer(newLayer);
        QMessageBox::information(this, "成功", "GDAL缓冲区分析完成！\n输出文件: " + outputPath);
        accept();
    }
    else {
        QMessageBox::critical(this, "加载错误", "缓冲区已生成，但无法加载到地图: " + outputPath + "\n错误: " + newLayer->error().message());
        delete newLayer; // 清理无效图层
    }
}
