#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QVariant>

#include "libtrkfileio/trkfileio.h"

#include "glwidget.h"
#include "linevertex.h"

namespace Ui {
class MainWindow;
}

//! \brief The MainWindow class.
//!
//! Class related to the MainWindow Qt User Interface.
//! Contains a GLWidget for OpenGL Drawing.
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

	//! \brief File dialog to open data files.
    void openFileAction();
    void closeAction();

    void renderModeChanged(int index);
	void on_generateTestDataButton_clicked();
	void on_spinBoxLineTriangleStripWidth_valueChanged(double value);
	void on_spinBoxLineWidthPercentageBlack_valueChanged(double value);
	void on_spinBoxLineWidthDepthCueingFactor_valueChanged(double value);
	void on_spinBoxLineHaloMaxDepth_valueChanged(double value);
	void on_pushButtonRestoreDefaults_clicked();

	void on_checkBoxEnableClipping_clicked(bool checked);
	void on_pushButtonSetClipPlaneNormal_clicked();
	void on_horizontalSliderClipPlaneDistance_valueChanged(int value);

	//! \brief generate a random position within an axis-aligned bounding box
	//! \param boundingBoxMin position defining start of axis aligned bounding box
	//! \param boundingBoxMax position defining end of axis aligned bounding box
	glm::vec3 randomPosInBoundingBox(glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax);

	//! \brief generate vertices along smooth lines within given bounding box
	//! \param numVertices this is only half the number of vertices generated: two copies of each vertex are stored in sequential manner to be able to render triangle strips later
	//! \param boundingBoxMin position defining start of axis aligned bounding box
	//! \param boundingBoxMax position defining end of axis aligned bounding box
	//!
	//! each vertex consists of 8 floats: 3 position, 3 direction to next vertex, 2 uv
	//! NOTE: two copies of all vertices are stored in sequential manner,
	//! with uv v-coordinate 0 and 1 to use for drawing as triangle strips (two strip vertices for each line vertex)
	void generateTestData(int numVertices, glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax);

	//! \brief Load TrackVis Tractography Track Line Data.
	//! \param filename path to file
	//! \return true if file was successfully loaded, else false
	//!
	//! This uses libtrkfileio by lheric from https://github.com/lheric/libtrkfileio.
	bool loadTRKData(QString &filename);

	//! \brief generate additional line vertex data (directions and uv) from line positions
	//! \param linePositions x,y,z coords of line points
	//!
	//! take x,y,z line points as input and store line vertices,
	//! each vertex consists of 8 floats: 3 position, 3 direction to next vertex, 2 uv.
	//! NOTE: two copies of all vertices are stored in sequential manner,
	//! with uv v-coordinate 0 and 1 to use for drawing as triangle strips (two strip vertices for each line vertex).
	//! u-coordinate is same for both and is interpolated along the length of the whole line,
	//! v-coordinate is set to 0 for vertex on "right" side of the strip and to 1 for vertex on "left" side.
	//! direction to next vertex: take average of direction to current and direction to next for smoother directions.
	void generateAdditionalLineVertexData(std::vector<glm::vec3> linePositions);

private:

	Ui::MainWindow *ui;

    // DATA

    enum DataType
    {
		TRK
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
