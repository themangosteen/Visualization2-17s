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
#include "linevertex.h"

class MainWindow;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

	friend class MainWindow; // make friend since tightly coupled anyway

public:
	GLWidget(QWidget *parent, MainWindow *mainWindow);
	~GLWidget();

	void initLineRenderMode(std::vector<std::vector<LineVertex> > *lines);

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

	// CPU line vertex data
	// each line vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	// NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	size_t nrLines;
	std::vector<std::vector<LineVertex> > *lines;

	// GPU line vertex data and shaders
	// each line vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	// NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	QOpenGLShaderProgram *simpleLineShader;
	QOpenGLVertexArrayObject vaoLines; // a VAO remembers states of buffer objects, allowing to easily bind/unbind different buffer states for rendering different objects in a scene.
	QOpenGLBuffer vboLines;

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
