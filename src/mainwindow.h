#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QVariant>

#include "glwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

    void displayTotalGPUMemory(float size);
    void displayUsedGPUMemory(float size);
    void displayFPS(int fps);

    inline GLWidget *getGLWidget()
    {
        return glWidget;
    }

signals:


public slots:


protected slots:

    void openFileAction();
    void closeAction();

    void renderModeChanged(int index);
    void contourWidthChanged(double value);

	void generateTestData(int numVertices, glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax);

private slots:
	void on_generateTestDataButton_clicked();

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
    std::vector<std::vector<glm::vec3> > datasetLines;
};

#endif // MAINWINDOW_H
