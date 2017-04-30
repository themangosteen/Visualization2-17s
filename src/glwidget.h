#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QCoreApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QGLShader>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <QTimer>

#include "camera.h"

class MainWindow;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

	friend class MainWindow; // make friend since tightly coupled anyway

public:
	GLWidget(QWidget *parent, MainWindow *mainWindow);
	~GLWidget();

	void initLineRenderMode(std::vector<std::vector<glm::vec3> > *lines, std::vector<std::vector<glm::vec3> > *linesDirections, std::vector<std::vector<glm::vec2> > *linesUV);

	float lineHaloDepth;
	float lineHaloWidth;
	float lineWidthPercentage; // percentage of the whole triangle strip which is the line (rest is halo)

	inline QImage getImage()
	{
		return this->grabFramebuffer();
	}

	enum RenderMode
	{
		NONE,
		LINES,
		POINTS
	} renderMode;

public slots:
	void cleanup();

signals:

protected:

	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

	void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;


protected slots:

	void paintGL() Q_DECL_OVERRIDE;
	void initializeGL() Q_DECL_OVERRIDE;
	void resizeGL(int w, int h) Q_DECL_OVERRIDE;

private:

	void initShaders();

	void allocateGPUBufferLineData();
	void drawLines();

	void calculateFPS();

	Camera camera;

	// CPU line geometry data
	size_t nrLines;
	std::vector<std::vector<glm::vec3> > *lines; // vector of lines (vector of vector of 3D points)
	std::vector<std::vector<glm::vec3> > *linesDirections; // vector of line directions (every vertex has a direction)
	std::vector<std::vector<glm::vec2> > *linesUV; // vector of line UVs

	// GPU line geometry data and shaders
	QOpenGLShaderProgram *simpleLineShader;
	QOpenGLVertexArrayObject vaoLines; // a VAO remembers states of buffer objects, allowing to easily bind/unbind different buffer states for rendering different objects in a scene.
	QOpenGLBuffer vboLines; // actual line data (array of 3D points)
	QOpenGLBuffer vboLinesDirection; // direction of the line segment (at the vertex)
	QOpenGLBuffer vboLinesUV; // direction of the line segment (at the vertex)

	// GUI ELEMENTS

	QPoint lastMousePos; // last mouse position (to determine movement delta)

	// vars to measure fps
	size_t frameCount = 0;
	size_t fps = 0;
	qint64 previousTimeFPS;
	QElapsedTimer fpsTimer;

	// memory usage
	bool GL_NVX_gpu_memory_info_supported = false;
	GLint total_mem_kb = 0;
	GLint cur_avail_mem_kb = 0;

	// triggers the rendering events
	QTimer paintTimer;

	QOpenGLDebugLogger *logger;
	void printDebugMsg(const QOpenGLDebugMessage &msg) { qDebug() << qPrintable(msg.message()); }

	MainWindow *mainWindow;

};

#endif
