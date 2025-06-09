#include "featureselectiontool.h"
#include <qgsmapcanvas.h>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <qgsproject.h>
#include <qgsmaplayer.h>
#include <qgsapplication.h>

#include <QToolTip> // 使用 Qt 的工具提示

FeatureSelectionTool::FeatureSelectionTool(QgsMapCanvas* canvas)
    : QgsMapToolIdentify(canvas)
{
    this->setCursor(Qt::ArrowCursor);
}

// 当工具被激活时，重置状态
void FeatureSelectionTool::activate()
{
    QgsMapTool::activate();
    clearCurrentSelection();
}

// 当工具被停用时，清理所有状态
void FeatureSelectionTool::deactivate()
{
    QgsMapTool::deactivate();
    clearCurrentSelection();
    QToolTip::hideText();
}

// 新的核心逻辑：鼠标移动事件
void FeatureSelectionTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    // 识别鼠标下的要素
    QList<IdentifyResult> results = this->identify(e->x(), e->y(), IdentifyMode::TopDownAll);

    if (results.isEmpty())
    {
        // 如果鼠标下没有要素，隐藏提示
        QToolTip::hideText();
        return;
    }

    // 只处理最顶层的矢量图层结果
    IdentifyResult topResult = results.first();
    QgsMapLayer* layer = topResult.mLayer;
    QgsFeature feature = topResult.mFeature;

    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
    if (!vlayer || !feature.isValid())
    {
        QToolTip::hideText();
        return;
    }

    // 获取要素的 "name" 属性
    QString displayName;
    int nameFieldIndex = vlayer->fields().indexOf("name");
    if (nameFieldIndex != -1) {
        displayName = feature.attribute("name").toString();
    }
    else {
        displayName = QString("ID: %1").arg(feature.id());
    }

    // 显示提示信息
    if (!displayName.isEmpty()) {
        QString tipText = QString("<b>名称:</b><br>%1").arg(displayName);
        QPoint globalPos = canvas()->mapToGlobal(e->pos());
        QToolTip::showText(globalPos, tipText, canvas());
    }
}


// 我们保留点击事件，用于真正“选定”和高亮要素
void FeatureSelectionTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    QList<IdentifyResult> results = this->identify(e->x(), e->y(), IdentifyMode::TopDownAll);

    // 先清除之前的选择
    clearCurrentSelection();

    if (results.isEmpty()) {
        canvas()->refresh();
        return;
    }

    // 只处理最顶层的矢量图层结果
    IdentifyResult topResult = results.first();
    QgsMapLayer* layer = topResult.mLayer;
    QgsFeature feature = topResult.mFeature;

    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
    if (!vlayer || !feature.isValid()) {
        canvas()->refresh();
        return;
    }

    // 在图层上选择该要素，使其高亮
    vlayer->select(feature.id());

    // 记录下这次选择，方便之后清除
    m_lastSelectedLayer = vlayer;
    m_lastSelectedFeatureId = feature.id();
}

// 辅助函数，用于清除当前的选择
void FeatureSelectionTool::clearCurrentSelection()
{
    if (m_lastSelectedLayer)
    {
        m_lastSelectedLayer->deselect(m_lastSelectedFeatureId);
        m_lastSelectedLayer = nullptr;
    }
    else
    {
        // 如果没有特定记录，则作为备用方案，清除所有图层的选择
        QList<QgsMapLayer*> layers = QgsProject::instance()->mapLayers().values();
        for (QgsMapLayer* layer : layers) {
            // 使用动态类型检查来确保图层是矢量图层
            QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
            if (vectorLayer) {
                vectorLayer->removeSelection();
            }
        }
    }
}