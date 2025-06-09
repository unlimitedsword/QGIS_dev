#include <qgsmaptoolidentify.h> // 使用这个基类，它为识别要素提供了便利
#include <qgsmapcanvas.h> // 确保包含QgsMapCanvas相关定义
#include <qgsmapmouseevent.h> // 确保包含QgsMapCanvasMouseEvent相关定义

class QgsMapCanvas;
class QgsVectorLayer;
class QgsFeature;

class FeatureSelectionTool : public QgsMapToolIdentify
{
    Q_OBJECT

public:
    explicit FeatureSelectionTool(QgsMapCanvas* canvas);

    // 重写父类的鼠标事件处理函数
    void canvasMoveEvent(QgsMapMouseEvent* e) override; // 主要逻辑在这里
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    // 当工具被激活或停用时调用
    void activate() override;
    void deactivate() override;

private:
    void clearCurrentSelection();

    // 记录当前高亮的要素，避免重复操作
    QgsVectorLayer* m_lastSelectedLayer = nullptr;
    QgsFeatureId m_lastSelectedFeatureId;
};
