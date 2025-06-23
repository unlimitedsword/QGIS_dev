// ResampleDialog.cpp
#include "ResampleDialog.h"
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

#include <qgsproject.h>
#include <qgsrasterlayer.h>
#include <qgsapplication.h> // For QgsApplication::messageBar() if needed
#include "Output_Manager.h" // 用于日志输出
#include <QDebug>
#include <qgsunittypes.h>

// GDAL for resampling (gdalwarp functionality)
#include "gdal_priv.h"
#include "gdalwarper.h"
#include "cpl_conv.h"
#include "gdal_alg.h"
#include "cpl_string.h"
#include "ogr_spatialref.h" // 如果涉及投影字符串

ResampleDialog::ResampleDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("栅格重采样");
    setMinimumWidth(550);
    setupUI();
    populateInputRasterComboBox();
    connect(m_inputRasterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ResampleDialog::onInputLayerOrMethodChanged);
    connect(m_resampleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ResampleDialog::onInputLayerOrMethodChanged);

}

ResampleDialog::~ResampleDialog()
{
}

void ResampleDialog::setupUI()
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

    // 2. 重采样方法
    gridLayout->addWidget(new QLabel("重采样方法:", this), 2, 0);
    m_resampleMethodCombo = new QComboBox(this);
    m_resampleMethodCombo->addItem("最近邻 (Nearest Neighbour)", static_cast<int>(GRA_NearestNeighbour));
    m_resampleMethodCombo->addItem("双线性插值 (Bilinear)", static_cast<int>(GRA_Bilinear));
    m_resampleMethodCombo->addItem("立方卷积 (Cubic Convolution)", static_cast<int>(GRA_Cubic));
    // m_resampleMethodCombo->addItem("三次样条 (Cubic Spline)", static_cast<int>(GRA_CubicSpline));
    // m_resampleMethodCombo->addItem("Lanczos", static_cast<int>(GRA_Lanczos));
    gridLayout->addWidget(m_resampleMethodCombo, 2, 1, 1, 2);

    // 3. 输出像元大小 (可选)
    gridLayout->addWidget(new QLabel("输出像元大小 X:", this), 3, 0);
    m_spinOutputPixelSizeX = new QDoubleSpinBox(this);
    m_spinOutputPixelSizeX->setDecimals(6); // 允许更高精度
    m_spinOutputPixelSizeX->setMinimum(0);  // 0 表示使用源分辨率
    m_spinOutputPixelSizeX->setSuffix(" (地图单位)");
    m_spinOutputPixelSizeX->setSpecialValueText("使用源分辨率"); // 当值为0时显示
    gridLayout->addWidget(m_spinOutputPixelSizeX, 3, 1);

    gridLayout->addWidget(new QLabel("输出像元大小 Y:", this), 4, 0);
    m_spinOutputPixelSizeY = new QDoubleSpinBox(this);
    m_spinOutputPixelSizeY->setDecimals(6);
    m_spinOutputPixelSizeY->setMinimum(0);
    m_spinOutputPixelSizeY->setSuffix(" (地图单位)");
    m_spinOutputPixelSizeY->setSpecialValueText("使用源分辨率");
    gridLayout->addWidget(m_spinOutputPixelSizeY, 4, 1);

    m_lblPixelSizeHint = new QLabel("提示: X和Y都设为0或留空，则保持原始分辨率。", this);
    m_lblPixelSizeHint->setStyleSheet("font-style: italic; color: gray;");
    gridLayout->addWidget(m_lblPixelSizeHint, 5, 0, 1, 3);


    // 4. 输出栅格路径
    gridLayout->addWidget(new QLabel("输出栅格路径:", this), 6, 0);
    m_editOutputRasterPath = new QLineEdit(this);
    m_editOutputRasterPath->setPlaceholderText("选择输出文件路径...");
    gridLayout->addWidget(m_editOutputRasterPath, 6, 1);
    m_btnSelectOutputRaster = new QPushButton("浏览...", this);
    gridLayout->addWidget(m_btnSelectOutputRaster, 6, 2);

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
    connect(m_inputRasterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString unitSuffix = " (未知单位)";
        if (index > 0) {
            QVariant data = m_inputRasterCombo->itemData(index);
            QgsRasterLayer* layer = qvariant_cast<QgsRasterLayer*>(data);
            if (layer && QgsProject::instance()->mapLayer(layer->id())) {
                m_currentInputRasterPath = layer->source();
                m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (项目)").arg(layer->name()));
                if (layer->crs().isGeographic()) {
                    unitSuffix = " (度)";
                }
                else {
                    unitSuffix = QString(" (%1)").arg(QgsUnitTypes::toString(layer->crs().mapUnits()));
                }
            }
        }
        else {
            if (m_currentInputRasterPath.isEmpty() || !m_lblSelectedRasterInfo->text().contains("(文件)")) {
                // ... (之前的逻辑)
            }
            else { // 如果是从文件选择的路径
                QgsRasterLayer tempLayer(m_currentInputRasterPath);
                if (tempLayer.isValid() && tempLayer.crs().isValid()) {
                    if (tempLayer.crs().isGeographic()) {
                        unitSuffix = " (度)";
                    }
                    else {
                        unitSuffix = QString(" (%1)").arg(QgsUnitTypes::toString(tempLayer.crs().mapUnits()));
                    }
                }
            }
        }
        m_spinOutputPixelSizeX->setSuffix(unitSuffix);
        m_spinOutputPixelSizeY->setSuffix(unitSuffix);
        updateDefaultOutputPath();
        });
    connect(m_btnSelectInputRasterFile, &QPushButton::clicked, this, &ResampleDialog::onSelectInputRasterClicked);
    connect(m_resampleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ResampleDialog::onInputLayerOrMethodChanged);
    connect(m_btnSelectOutputRaster, &QPushButton::clicked, this, &ResampleDialog::onSelectOutputRasterClicked);
    connect(m_btnOk, &QPushButton::clicked, this, &ResampleDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ResampleDialog::populateInputRasterComboBox()
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

void ResampleDialog::onSelectInputRasterClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择输入栅格文件",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "栅格文件 (*.tif *.tiff *.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        m_currentInputRasterPath = filePath;
        m_lblSelectedRasterInfo->setText(QString("当前选择: %1 (文件)").arg(QFileInfo(filePath).fileName()));
        m_inputRasterCombo->setCurrentIndex(0); // 清除下拉框选择
        updateDefaultOutputPath();
    }
}

void ResampleDialog::onInputLayerOrMethodChanged()
{
    updateDefaultOutputPath();
}

void ResampleDialog::updateDefaultOutputPath()
{
    if (m_currentInputRasterPath.isEmpty()) {
        m_editOutputRasterPath->clear();
        return;
    }

    QFileInfo fi(m_currentInputRasterPath);
    QString baseName = fi.completeBaseName();
    QString dirPath = fi.absolutePath();
    QString suffix = fi.suffix(); // 保留原始后缀

    QString methodSuffix;
    GDALResampleAlg alg = static_cast<GDALResampleAlg>(m_resampleMethodCombo->currentData().toInt());
    switch (alg) {
    case GRA_NearestNeighbour: methodSuffix = "nn"; break;
    case GRA_Bilinear:         methodSuffix = "bilinear"; break;
    case GRA_Cubic:            methodSuffix = "cubic"; break;
        // case GRA_CubicSpline:      methodSuffix = "cubicspline"; break;
        // case GRA_Lanczos:          methodSuffix = "lanczos"; break;
    default:                   methodSuffix = "res"; break;
    }

    QString defaultOutputName = QString("%1_resampled_%2.%3").arg(baseName).arg(methodSuffix).arg(suffix);
    m_editOutputRasterPath->setText(QDir(dirPath).filePath(defaultOutputName));
}


void ResampleDialog::onSelectOutputRasterClicked()
{
    QString defaultPath = m_editOutputRasterPath->text();
    if (defaultPath.isEmpty() && !m_currentInputRasterPath.isEmpty()) {
        // 尝试基于输入路径生成一个默认的保存路径
        QFileInfo fi(m_currentInputRasterPath);
        defaultPath = fi.absolutePath();
    }
    else if (defaultPath.isEmpty()) {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QString filePath = QFileDialog::getSaveFileName(this, "选择输出栅格文件路径",
        defaultPath,
        "GeoTIFF (*.tif *.tiff);;Erdas Imagine (*.img);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        // 确保有后缀
        if (QFileInfo(filePath).suffix().isEmpty()) {
            if (!filePath.endsWith(".")) filePath += ".";
            filePath += "tif"; // 默认tif
        }
        m_editOutputRasterPath->setText(filePath);
    }
}

// 公共接口实现
QString ResampleDialog::selectedInputRasterPath() const {
    return m_currentInputRasterPath;
}

Qgis::RasterResamplingMethod ResampleDialog::selectedResamplingMethod() const {
    // GDALResampleAlg 与 Qgis::RasterResamplingMethod 的值是对应的
    return static_cast<Qgis::RasterResamplingMethod>(m_resampleMethodCombo->currentData().toInt());
}

QString ResampleDialog::outputRasterPath() const {
    return m_editOutputRasterPath->text();
}

double ResampleDialog::outputPixelSizeX() const {
    double val = m_spinOutputPixelSizeX->value();
    return (val <= 0) ? -1.0 : val; // 0或负数表示使用源分辨率
}
double ResampleDialog::outputPixelSizeY() const {
    double val = m_spinOutputPixelSizeY->value();
    return (val <= 0) ? -1.0 : val;
}


// ResampleDialog.cpp

void ResampleDialog::onOkClicked()
{
    // --- 步骤 0: 获取所有输入参数 ---
    QString inputPath = selectedInputRasterPath();
    GDALResampleAlg resampleAlg = static_cast<GDALResampleAlg>(m_resampleMethodCombo->currentData().toInt());
    QString outputPath = outputRasterPath();
    double targetPixelSizeX = outputPixelSizeX(); // -1.0 或 0.0 表示使用源分辨率
    double targetPixelSizeY = outputPixelSizeY(); // -1.0 或 0.0 表示使用源分辨率

    // --- 参数检查 ---
    if (inputPath.isEmpty()) { QMessageBox::warning(this, "输入错误", "请选择输入栅格图层。"); return; }
    if (outputPath.isEmpty()) { QMessageBox::warning(this, "输入错误", "请输入输出栅格路径。"); return; }

    qDebug() << "\n====== 开始重采样操作 (直接WarpOperation) ======";
    qDebug() << "GDAL版本:" << GDALVersionInfo("RELEASE_NAME");
    qDebug() << "输入文件:" << inputPath;
    qDebug() << "输出文件:" << outputPath;
    qDebug() << "重采样算法 (int):" << static_cast<int>(resampleAlg);
    qDebug() << "目标像元大小 X:" << targetPixelSizeX << " Y:" << targetPixelSizeY;

    GDALAllRegister();

    // --- 步骤 1: 打开输入数据集 ---
    GDALDatasetH hSrcDS = GDALOpen(inputPath.toUtf8().constData(), GA_ReadOnly);
    if (!hSrcDS) {
        QMessageBox::critical(this, "GDAL错误", "无法打开输入栅格: " + inputPath + "\n" + CPLGetLastErrorMsg());
        return;
    }

    // --- 步骤 2: 获取输入参数，计算输出几何参数 ---
    const int srcWidth = GDALGetRasterXSize(hSrcDS);
    const int srcHeight = GDALGetRasterYSize(hSrcDS);
    const int bandCount = GDALGetRasterCount(hSrcDS);
    if (bandCount == 0) {
        QMessageBox::critical(this, "GDAL错误", "输入栅格不包含任何波段。");
        GDALClose(hSrcDS);
        return;
    }
    const GDALDataType dataType = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS, 1));
    double adfSrcGeoTransform[6];
    if (GDALGetGeoTransform(hSrcDS, adfSrcGeoTransform) != CE_None) {
        QMessageBox::critical(this, "GDAL错误", "无法获取输入栅格的地理变换参数。");
        GDALClose(hSrcDS);
        return;
    }
    const char* pszSrcProjection = GDALGetProjectionRef(hSrcDS);

    int dstWidth = srcWidth;
    int dstHeight = srcHeight;
    double adfDstGeoTransform[6];
    memcpy(adfDstGeoTransform, adfSrcGeoTransform, sizeof(double) * 6); // 初始化目标地理变换

    // 用户指定了目标分辨率
    if (targetPixelSizeX > 0 && targetPixelSizeY > 0) {
        double dfSrcXMin = adfSrcGeoTransform[0];
        double dfSrcYMax = adfSrcGeoTransform[3];
        double dfSrcXMax = dfSrcXMin + adfSrcGeoTransform[1] * srcWidth;
        double dfSrcYMin = dfSrcYMax + adfSrcGeoTransform[5] * srcHeight;

        qDebug() << "  源X范围:" << dfSrcXMin << "to" << dfSrcXMax << "(宽度:" << (dfSrcXMax - dfSrcXMin) << ")";
        qDebug() << "  源Y范围:" << dfSrcYMin << "to" << dfSrcYMax << "(高度:" << (dfSrcYMax - dfSrcYMin) << ")";
        qDebug() << "  目标像元X:" << targetPixelSizeX << "目标像元Y:" << targetPixelSizeY;

        // 确保除数不为0，并且是有意义的正值
        if (targetPixelSizeX <= 0 || targetPixelSizeY <= 0) {
            QMessageBox::critical(this, "参数错误", "目标像元大小必须为正数。");
            GDALClose(hSrcDS);
            return;
        }

        double extentWidth = std::abs(dfSrcXMax - dfSrcXMin);
        double extentHeight = std::abs(dfSrcYMax - dfSrcYMin);

        // 如果源数据的地理范围本身非常小（例如，接近0），则不应进行基于分辨率的尺寸计算
        if (extentWidth < 1e-9 || extentHeight < 1e-9) { // 1e-9 是一个很小的阈值
            qWarning() << "源数据地理范围过小，无法根据目标分辨率计算尺寸。将使用源尺寸。";
            // 保持 dstWidth = srcWidth, dstHeight = srcHeight
        }
        else {
            dstWidth = static_cast<int>(extentWidth / targetPixelSizeX + 0.5);
            dstHeight = static_cast<int>(extentHeight / std::abs(targetPixelSizeY) + 0.5); // Y像元大小通常用绝对值
        }

        // 保持原始栅格左上角坐标和像元大小符号
        adfDstGeoTransform[0] = adfSrcGeoTransform[0];
        adfDstGeoTransform[1] = targetPixelSizeX;
        adfDstGeoTransform[2] = adfSrcGeoTransform[2]; // 保留旋转参数
        adfDstGeoTransform[3] = adfSrcGeoTransform[3];
        adfDstGeoTransform[4] = adfSrcGeoTransform[4]; // 保留旋转参数
        adfDstGeoTransform[5] = (adfSrcGeoTransform[5] < 0) ? -std::abs(targetPixelSizeY) : std::abs(targetPixelSizeY);
    }

    qDebug() << "计算出的目标尺寸:" << dstWidth << "x" << dstHeight;
    if (dstWidth <= 0 || dstHeight <= 0) {
        QMessageBox::critical(this, "参数错误",
            QString("计算出的目标栅格尺寸为 %1x%2，这是一个无效尺寸。\n\n"
                "这通常意味着您设定的“目标像元大小”相对于源栅格的地理范围过大。\n\n"
                "源栅格宽度（地理单位）: %3\n"
                "源栅格高度（地理单位）: %4\n"
                "您设定的目标像元大小 X: %5, Y: %6 (单位与源栅格一致)")
            .arg(dstWidth).arg(dstHeight)
            .arg(std::abs(adfSrcGeoTransform[1] * srcWidth)) // 源地理宽度
            .arg(std::abs(adfSrcGeoTransform[5] * srcHeight)) // 源地理高度
            .arg(targetPixelSizeX).arg(targetPixelSizeY)
        );
        GDALClose(hSrcDS);
        return;
    }

    // --- 步骤 3: 创建输出数据集 ---
    GDALDriverH hDriver = GDALGetDriverByName("GTiff");
    if (!hDriver) {
        QMessageBox::critical(this, "GDAL错误", "无法获取GTiff驱动。");
        GDALClose(hSrcDS);
        return;
    }
    char** papszCreateOptions = nullptr;
    papszCreateOptions = CSLSetNameValue(papszCreateOptions, "TILED", "YES");
    papszCreateOptions = CSLSetNameValue(papszCreateOptions, "COMPRESS", "LZW");

    GDALDatasetH hDstDS = GDALCreate(hDriver, outputPath.toUtf8().constData(),
        dstWidth, dstHeight, bandCount, dataType, papszCreateOptions);
    CSLDestroy(papszCreateOptions);

    if (!hDstDS) {
        QMessageBox::critical(this, "GDAL错误", "无法创建输出文件: " + outputPath + "\n" + CPLGetLastErrorMsg());
        GDALClose(hSrcDS);
        return;
    }

    // 设置输出数据集的地理转换和投影
    GDALSetGeoTransform(hDstDS, adfDstGeoTransform);
    if (pszSrcProjection != nullptr && strlen(pszSrcProjection) > 0) {
        GDALSetProjection(hDstDS, pszSrcProjection);
    }
    qDebug() << "输出数据集已创建。";

    // --- 步骤 4: 配置并执行 GDALWarpOperation ---
    GDALWarpOptions* psWarpOptions = GDALCreateWarpOptions();
    psWarpOptions->eResampleAlg = resampleAlg;

    // 关键：将源和已创建的目标数据集都提供给WarpOptions
    psWarpOptions->hSrcDS = hSrcDS;
    psWarpOptions->hDstDS = hDstDS;

    psWarpOptions->nBandCount = bandCount;
    psWarpOptions->panSrcBands = (int*)CPLMalloc(sizeof(int) * bandCount);
    psWarpOptions->panDstBands = (int*)CPLMalloc(sizeof(int) * bandCount);
    for (int i = 0; i < bandCount; i++) {
        psWarpOptions->panSrcBands[i] = i + 1;
        psWarpOptions->panDstBands[i] = i + 1;
    }

    // 创建转换器，此时源和目标数据集的投影和地理变换都已确定
    // 如果源和目标投影不同，GDALCreateGenImgProjTransformer2会处理重投影
    // 如果相同，它主要处理地理变换之间的映射
    psWarpOptions->pTransformerArg = GDALCreateGenImgProjTransformer2(hSrcDS, hDstDS, nullptr); // 第三个参数可传NULL或创建选项
    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

    if (!psWarpOptions->pTransformerArg) {
        QMessageBox::critical(this, "GDAL错误", "创建坐标转换器失败:\n" + QString(CPLGetLastErrorMsg()));
        CPLFree(psWarpOptions->panSrcBands); CPLFree(psWarpOptions->panDstBands);
        GDALDestroyWarpOptions(psWarpOptions);
        GDALClose(hDstDS); GDALClose(hSrcDS);
        return;
    }
    qDebug() << "WarpOptions配置完毕，转换器已创建。";

    GDALWarpOperation oWarper;
    CPLErr eErr = oWarper.Initialize(psWarpOptions);

    if (eErr == CE_None) {
        qDebug() << "WarpOperation初始化成功，开始ChunkAndWarpImage...";
        // 目标尺寸从hDstDS获取
        eErr = oWarper.ChunkAndWarpImage(0, 0, GDALGetRasterXSize(hDstDS), GDALGetRasterYSize(hDstDS));
        if (eErr != CE_None) {
            QMessageBox::critical(this, "重采样失败", "GDAL ChunkAndWarpImage 操作失败: " + QString(CPLGetLastErrorMsg()));
        }
    }
    else {
        QMessageBox::critical(this, "GDAL错误", "初始化Warp操作失败: " + QString(CPLGetLastErrorMsg()));
    }

    // --- 步骤 5: 清理资源 ---
    if (psWarpOptions != nullptr) {
        if (psWarpOptions->pTransformerArg != nullptr) {
            GDALDestroyTransformer(psWarpOptions->pTransformerArg);
            psWarpOptions->pTransformerArg = nullptr;
        }
        if (psWarpOptions->panSrcBands) { // 检查是否为nullptr，以防之前分配失败
            CPLFree(psWarpOptions->panSrcBands); psWarpOptions->panSrcBands = nullptr;
        }
        if (psWarpOptions->panDstBands) {
            CPLFree(psWarpOptions->panDstBands); psWarpOptions->panDstBands = nullptr;
        }
        GDALDestroyWarpOptions(psWarpOptions); psWarpOptions = nullptr;
    }

    // **确保目标数据集在所有操作完成后，并且在尝试用QGIS加载它之前被关闭**
    if (hDstDS != nullptr) {
        qDebug() << "Closing destination dataset (hDstDS) before attempting to load with QGIS.";
        GDALClose(hDstDS);
        hDstDS = nullptr; // 设为nullptr，避免后续意外关闭
    }
    if (hSrcDS != nullptr) {
        qDebug() << "Closing source dataset (hSrcDS).";
        GDALClose(hSrcDS);
        hSrcDS = nullptr;
    }

    // --- 步骤 6: 结果反馈和加载到QGIS ---
    if (eErr == CE_None) { // 只有当 GDAL Warp 操作没有报告错误时才继续
        QMessageBox::information(this, "成功", "栅格重采样完成！\n输出文件: " + outputPath);

        // ====================== 加载结果图层并进行详细错误检查 ======================
        OutputManager::instance()->logMessage("Attempting to load resampled raster: " + outputPath);
        QgsRasterLayer* newLayer = new QgsRasterLayer(outputPath, QFileInfo(outputPath).baseName(), "gdal");

        if (newLayer->isValid()) {
            OutputManager::instance()->logMessage("Resampled raster layer loaded successfully: " + newLayer->name());
            QgsProject::instance()->addMapLayer(newLayer);
            // (可选) 主窗口缩放到新图层
            // 例如，可以发射一个信号，让主窗口去处理
            // emit resampleSucceededAndLayerAdded(newLayer); 
        }
        else {
            QString errorMsg = "加载重采样后的栅格图层失败: " + outputPath;
            if (newLayer) { // 确保newLayer不是nullptr才调用error()
                errorMsg += "\nQGIS Layer Error: " + newLayer->error().message();
                delete newLayer; // 清理无效图层对象
                newLayer = nullptr;
            }
            else {
                errorMsg += "\nQGIS Layer Error: Layer object creation failed (returned nullptr).";
            }

            const char* gdalLastError = CPLGetLastErrorMsg();
            if (gdalLastError && strlen(gdalLastError) > 0 && strcmp(gdalLastError, "None") != 0) { // 检查是否真的有GDAL错误
                errorMsg += "\nLast GDAL Error (after attempting QGIS load): " + QString::fromUtf8(gdalLastError);
            }

            OutputManager::instance()->logError(errorMsg);
            QMessageBox::warning(this, "加载失败", errorMsg);
        }
        // =======================================================================
        accept(); // 关闭对话框
    }
    else {
        // 如果 eErr != CE_None，说明GDAL Warp操作本身就失败了
        // QMessageBox::critical 已经在之前的错误判断中显示过了，这里可以不再重复
        // 或者，如果之前的QMessageBox只针对特定步骤，这里可以加一个总的失败提示
        OutputManager::instance()->logError("GDAL Warp operation failed with error code: " + QString::number(eErr));
    }
}