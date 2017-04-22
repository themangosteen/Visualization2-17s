#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QVariant>

#include "ui_MainWindow.h"
#include "GLWidget.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:

	MainWindow(QWidget *parent = 0);
	~MainWindow();

	void setAnimationFrameGUI(int frame);
	void displayTotalGPUMemory(float size);
	void displayUsedGPUMemory(float size);
	void displayFPS(int fps);

	inline GLWidget *getGLWidget()
	{
		return glWidget;
	}

	inline int getMaxFrames() { return ui->frame_slider->maximum(); }
	inline int getCurrentFrame() { return ui->frame_slider->value(); }

signals:


public slots:

	void frameChanged(int value);
	void playAnimation();
	void pauseAnimation();

protected slots:

	void openFileAction();
	void closeAction();

	void ambientChanged(double value);
	void diffuseChanged(double value);
	void specularChanged(double value);
	
	void renderModeChanged(int index);
	void alwaysLitCheckBoxChanged(int state);
	void contourWidthChanged(double value);


private:

	// USER INTERFACE ELEMENTS

	Ui_MainWindow *ui;


	// DATA 

	enum DataType
	{
		NETCDF
	};

	struct FileType
	{
		QString filename;
		DataType type;
	} fileType;

	GLWidget *glWidget;
	std::vector<std::vector<glm::vec3> > datasetLines;

    void loadStandardData();
};

#endif
