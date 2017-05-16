#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QVariant>

#include "glwidget.h"
#include "linevertex.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

    inline GLWidget *getGLWidget()
    {
        return glWidget;
    }

signals:


public slots:

	void displayTotalGPUMemory(float size);
	void displayUsedGPUMemory(float size);
	void displayFPS(int fps);
	void displayGraphicsDeviceInfo(QString string);

protected slots:

    void openFileAction();
    void closeAction();

    void renderModeChanged(int index);
	void on_generateTestDataButton_clicked();
	void on_spinBoxLineTriangleStripWidth_valueChanged(double value);
	void on_spinBoxLineWidthPercentageBlack_valueChanged(double value);
	void on_spinBoxLineWidthDepthCueingFactor_valueChanged(double value);
	void on_spinBoxLineHaloMaxDepth_valueChanged(double value);
	void on_pushButtonRestoreDefaults_clicked();

	glm::vec3 randomPosInBoundingBox(glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax);
	void generateTestData(int numVertices, glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax);

private:

	Ui::MainWindow *ui;

    // DATA

    enum DataType
    {
		SOP
    };

    struct FileType
    {
        QString filename;
        DataType type;
    } fileType;

    GLWidget *glWidget;
	std::vector<std::vector<LineVertex> > datasetLines;

};

#endif // MAINWINDOW_H
