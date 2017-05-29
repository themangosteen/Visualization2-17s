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

//! \brief The GLWidget class
//! This is the main class containing the OpenGL context
//! All interaction with OpenGL happens here
class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

	friend class MainWindow; //!< make friend since tightly coupled anyway

public:
	GLWidget(QWidget *parent, MainWindow *mainWindow);
	~GLWidget();

	//! \brief set up OpenGL buffers and shaders to render lines
	//! \param lines vector of lines (vector of vector of line vertices)
	//! each vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	//! we store all three attributes interleaved on single vbo [<posdirectionuv><posdirectionuv>...]
	//! NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	void initLineRenderMode(std::vector<std::vector<LineVertex> > *lines);

	float lineTriangleStripWidth; //!< total width of triangle strip (black line + white halo)
	float lineWidthPercentageBlack; //!< percentage of triangle strip drawn black to represent line (rest is white halo)
	float lineWidthDepthCueingFactor; //!< how much the black line is drawn thinner with increasing depth
	float lineHaloMaxDepth; //!< max depth displacement of halo

	bool enableClipping; //!< if enabled, clip beyond a specified clipping plane. note: no real clipping we only discard fragments
	glm::vec3 clipPlaneNormal; //!< direction along which to clip. note: no real clipping we only discard fragments
	float clipPlaneDistance; //!< distance from origin in direction of clipPlaneNormal beyond which to clip

	//! \brief getImage
	//! \return an image snapshot of the current OpenGL framebuffer
	inline QImage getImage()
	{
		return this->grabFramebuffer();
	}

	//! \brief updateClipPlaneNormal
	//! set the clip plane normal to the current right camera vector
	//! this is much easier for the user than setting the clip plane explicitly
	inline void updateClipPlaneNormal()
	{
		clipPlaneNormal = camera.getRight();
	}

	//! \brief RenderMode enum
	//! currently only LINES is supported
	enum RenderMode
	{
		NONE,
		LINES,
		POINTS
	} renderMode;

public slots:
	void cleanup();

signals:
	void usedGPUMemoryChanged(float size);
	void totalGPUMemoryChanged(float size);
	void fpsChanged(int fps);
	void graphicsDeviceInfoChanged(QString string);

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

	//! CPU line vertex data
	//! each line vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	//! NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	std::vector<std::vector<LineVertex> > *lines;
	size_t nrLines;

	// GPU line vertex data and shaders
	// each line vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	// NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	QOpenGLShaderProgram *shaderLinesWithHalos;
	QOpenGLVertexArrayObject vaoLines; // VAO remembers states of buffer objects, allowing to easily bind/unbind different buffer states for rendering different objects in a scene.
	QOpenGLBuffer vboLines;

	// GUI ELEMENTS

	QPoint lastMousePos; // last mouse position (to determine mouse movement delta)

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
