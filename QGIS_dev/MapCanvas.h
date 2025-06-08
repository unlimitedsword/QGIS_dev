// MapCanvas.h

#ifndef MAPCANVAS_H
#define MAPCANVAS_H

#include <QWidget>

// 前向声明，避免在头文件中包含大型QGIS头文件
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapToolPan;

class MapCanvas : public QWidget
{
    Q_OBJECT // << 必须添加，因为我们使用了信号和槽

public:
    MapCanvas(QWidget* parent = nullptr);
    ~MapCanvas();

    QgsMapCanvas* getCanvas() const;
    void zoomToLayer(QgsMapLayer* layer);

public slots:
    // --- 新增的公共槽函数 ---
    void zoomIn();  // 放大
    void zoomOut(); // 缩小

signals:
    // --- 新增的信号 ---
    // 当比例尺变化时，发出此信号，携带格式化好的字符串
    void scaleChanged(const QString& scaleText);

private slots:
    // --- 新增的私有槽函数 ---
    // 用于接收来自 QgsMapCanvas 内部的 scaleChanged 信号
    void onCanvasScaleChanged(double newScale);

private:
    QgsMapCanvas* m_qgsCanvas;
    QgsMapToolPan* m_panTool;
};

#endif // MAPCANVAS_H