#pragma once
#include <QtWidgets/QMainWindow>
#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>

class MapCanvas : public QWidget 
{
	Q_OBJECT

public:
	MapCanvas(QWidget* parent = nullptr);
	~MapCanvas();

	void addVectorLayer(QString vectorLayerPath);

private:
	QgsMapCanvas* m_qgsCanvas; //声明画布
	QgsMapToolPan* m_panTool; // 声明平移工具
};