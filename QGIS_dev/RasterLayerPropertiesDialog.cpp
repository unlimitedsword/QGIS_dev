// RasterLayerPropertiesDialog.cpp
#include "RasterLayerPropertiesDialog.h"
#include <qgsrasterlayer.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsrectangle.h>
#include <qgsrasterbandstats.h>
#include "gdal_priv.h"
#include "cpl_conv.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QGroupBox>
#include <QDebug>
#include <QPushButton>
#include <QFile>

RasterLayerPropertiesDialog::RasterLayerPropertiesDialog(QgsRasterLayer* layer, QWidget* parent)
    : QDialog(parent), m_rasterLayer(layer)
{
    if (!m_rasterLayer || !m_rasterLayer->isValid()) {
        qWarning() << "RasterLayerPropertiesDialog created with invalid layer.";
        // 如果在构造时就出错，最好不要继续执行setupUI，可以简单地reject
        // QTimer::singleShot(0, this, &QDialog::reject); // 异步关闭，避免在构造中直接reject可能的问题
        return; // 或者直接返回，让调用者检查并处理
    }

    setWindowTitle(QString("栅格图层属性 - %1").arg(m_rasterLayer->name()));
    resize(700, 650); // 尝试一个更大的尺寸
    setupUI();
    populateGeneralInfo();
    populateAllBandStatistics(); // 初始化时就显示所有波段统计
}

RasterLayerPropertiesDialog::~RasterLayerPropertiesDialog()
{
}

void RasterLayerPropertiesDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- 1. 基本信息组 ---
    QGroupBox* infoGroup = new QGroupBox("基本信息", this);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    m_infoTableWidget = new QTableWidget(this);
    m_infoTableWidget->setColumnCount(2);
    m_infoTableWidget->setHorizontalHeaderLabels({ "属性", "值" });

    m_infoTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // “属性”列根据内容自动调整
    m_infoTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);       // “值”列自动拉伸填充

    m_infoTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_infoTableWidget->verticalHeader()->setVisible(false);
    m_infoTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_infoTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    infoLayout->addWidget(m_infoTableWidget);
    infoGroup->setLayout(infoLayout);
    mainLayout->addWidget(infoGroup);

    // --- 2. 所有波段统计信息组 ---
    QGroupBox* allStatsGroup = new QGroupBox("所有波段统计", this);
    QVBoxLayout* allStatsLayout = new QVBoxLayout(allStatsGroup);
    m_allBandStatsTextEdit = new QTextEdit(this);
    m_allBandStatsTextEdit->setReadOnly(true);

    // 调整字体
    QFont statsFont("Courier", 9); // 使用Courier字体（等宽），字号设为9
    m_allBandStatsTextEdit->setFont(statsFont);
    // 或者只改变字号:
    // QFont currentFont = m_allBandStatsTextEdit->font();
    // currentFont.setPointSize(9); // 设置一个较小的字号
    // m_allBandStatsTextEdit->setFont(currentFont);

    allStatsLayout->addWidget(m_allBandStatsTextEdit);
    allStatsGroup->setLayout(allStatsLayout);
    mainLayout->addWidget(allStatsGroup);

    mainLayout->addStretch(); // 确保内容靠上

    // (可选) 添加一个关闭按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeButton = new QPushButton("关闭", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void RasterLayerPropertiesDialog::populateGeneralInfo()
{
    if (!m_rasterLayer) return;

    m_infoTableWidget->setRowCount(0); // 清空旧内容

    auto addInfoRow = [&](const QString& label, const QString& value) {
        int row = m_infoTableWidget->rowCount();
        m_infoTableWidget->insertRow(row);
        m_infoTableWidget->setItem(row, 0, new QTableWidgetItem(label));
        m_infoTableWidget->setItem(row, 1, new QTableWidgetItem(value));
        };

    addInfoRow("名称:", m_rasterLayer->name());
    addInfoRow("路径:", m_rasterLayer->source());
    addInfoRow("宽度 (列):", QString::number(m_rasterLayer->width()));
    addInfoRow("高度 (行):", QString::number(m_rasterLayer->height()));
    addInfoRow("波段数量:", QString::number(m_rasterLayer->bandCount()));

    QgsCoordinateReferenceSystem crs = m_rasterLayer->crs();
    if (crs.isValid()) {
        addInfoRow("坐标参考系:", QString("%1 (%2)").arg(crs.description()).arg(crs.authid()));
    }
    else {
        addInfoRow("坐标参考系:", "未知");
    }

    QgsRectangle extent = m_rasterLayer->extent();
    if (!extent.isEmpty()) {
        addInfoRow("范围 (XMin):", QString::number(extent.xMinimum()));
        addInfoRow("范围 (YMin):", QString::number(extent.yMinimum()));
        addInfoRow("范围 (XMax):", QString::number(extent.xMaximum()));
        addInfoRow("范围 (YMax):", QString::number(extent.yMaximum()));
    }
    else {
        addInfoRow("范围:", "未知");
    }
    m_infoTableWidget->resizeColumnsToContents();
}

void RasterLayerPropertiesDialog::populateAllBandStatistics()
{
    if (!m_rasterLayer) return;

    m_allBandStatsTextEdit->clear();
    QString statsTextAccumulator; // 用于累积所有波段的文本

    // 获取栅格文件的完整路径
    QString rasterFilePath = m_rasterLayer->source();
    if (rasterFilePath.isEmpty() || !QFile::exists(rasterFilePath)) {
        statsTextAccumulator = "错误：无法获取有效的栅格文件路径。";
        m_allBandStatsTextEdit->setText(statsTextAccumulator);
        return;
    }

    qDebug() << "Populating all band statistics using GDAL for:" << rasterFilePath;

    // 注册所有GDAL驱动 (通常在QgsApplication::initQgis()中已完成，但再次调用无害)
    GDALAllRegister();

    // 打开栅格数据集
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(rasterFilePath.toUtf8().constData(), GA_ReadOnly);
    if (poDataset == nullptr) {
        statsTextAccumulator = QString("GDAL错误：无法打开栅格文件。\n%1").arg(CPLGetLastErrorMsg());
        m_allBandStatsTextEdit->setText(statsTextAccumulator);
        return;
    }

    int numBands = poDataset->GetRasterCount();
    if (numBands == 0) {
        statsTextAccumulator = "错误：栅格文件不包含任何波段。";
        m_allBandStatsTextEdit->setText(statsTextAccumulator);
        GDALClose(poDataset);
        return;
    }

    // 遍历所有波段
    for (int i = 1; i <= numBands; ++i) {
        statsTextAccumulator += QString("--- 波段 %1 ---\n").arg(i);

        GDALRasterBand* poBand = poDataset->GetRasterBand(i);
        if (poBand == nullptr) {
            statsTextAccumulator += QString("  错误：无法获取波段 %1 的数据。\n\n").arg(i);
            continue; // 继续下一个波段
        }

        // 尝试获取元数据中的统计值，或进行近似计算
        double dfMin = 0.0, dfMax = 0.0, dfMean = 0.0, dfStdDev = 0.0;
        int bSuccessMin, bSuccessMax;

        dfMin = poBand->GetMinimum(&bSuccessMin);
        dfMax = poBand->GetMaximum(&bSuccessMax);

        // 如果元数据中没有Min/Max，或者获取失败，则计算近似值
        if (!bSuccessMin || !bSuccessMax) {
            qDebug() << "Band" << i << ": No Min/Max in metadata, computing (approximate)...";
            // ComputeRasterMinMax的第二个参数是输出数组，第一个是Min，第二个是Max
            double adfMinMax[2];
            if (poBand->ComputeRasterMinMax(TRUE, adfMinMax) == CE_None) { // TRUE for ApproxOK
                dfMin = adfMinMax[0];
                dfMax = adfMinMax[1];
            }
            else {
                qDebug() << "GDAL ComputeRasterMinMax for band" << i << "FAILED:" << CPLGetLastErrorMsg();
            }
        }

        // 尝试获取均值和标准差 (GetStatistics会尝试从元数据读取，如果不存在则计算)
        // 第一个参数 bApproxOK 设为 TRUE
        if (poBand->GetStatistics(TRUE, TRUE, &dfMin, &dfMax, &dfMean, &dfStdDev) == CE_Failure) {
            qDebug() << "GDAL GetStatistics for band" << i << "FAILED: " << CPLGetLastErrorMsg();
            // 作为后备，再尝试一次 ComputeStatistics (近似)
            if (poBand->ComputeStatistics(TRUE, &dfMean, &dfStdDev, nullptr, nullptr, nullptr, nullptr) == CE_Failure) {
                qDebug() << "GDAL ComputeStatistics (fallback) for band" << i << "FAILED: " << CPLGetLastErrorMsg();
            }
        }

        // 获取NoData值
        int bGotNoData;
        double nodataValue = poBand->GetNoDataValue(&bGotNoData);

        statsTextAccumulator += QString("  最小值: %1\n").arg(dfMin);
        statsTextAccumulator += QString("  最大值: %1\n").arg(dfMax);
        statsTextAccumulator += QString("  平均值: %1\n").arg(dfMean);
        statsTextAccumulator += QString("  标准差: %1\n").arg(dfStdDev);
        if (bGotNoData) {
            statsTextAccumulator += QString("  NoData值: %1\n").arg(nodataValue);
        }
        statsTextAccumulator += "\n";
    }

    GDALClose(poDataset); // 关闭数据集
    m_allBandStatsTextEdit->setText(statsTextAccumulator);
}