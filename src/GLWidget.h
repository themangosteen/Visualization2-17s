#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QGLShader>
#include <QOpenGLShaderProgram>
#include <QFileSystemWatcher>
#include <QElapsedTimer>
#include <QTimer>

#include "Camera.h"

class MainWindow;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

	friend class MainWindow; // make friend since tightly coupled anyway

public:
	GLWidget(QWidget *parent, MainWindow *mainWindow);
	~GLWidget();

	void initLineRenderMode(std::vector<std::vector<glm::vec3> > *lines);

	void playAnimation();
	void pauseAnimation();
	void setAnimationFrame(int frameNr);

	float ambientFactor;
	float diffuseFactor;
	float specularFactor;
	float shininess;
	bool alwaysLit;
	float lineHaloDepth;
	float lineHaloWidth;

	inline QImage getImage()
	{
		return this->grabFramebuffer();
	}

	enum RenderMode
	{
		NONE,
		PDB,   // RCSB Protein Data Bank files
		NETCDF // Network Common Data Form files
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
	void fileChanged(const QString &path);

private:

	void drawLines();

	bool loadLineShader();

	void initglsw();

	void allocateGPUBuffer(int frameNr);

	void calculateFPS();

	Camera camera;

	size_t nrLines;

	// CPU line geometry data
	std::vector<std::vector<glm::vec3> > *lines; // vector of lines (vector of vector of points)

	// GPU line geometry data and shaders
	QOpenGLShaderProgram *shaderprogramLines;
	QOpenGLShader *vertexShaderLines;
	QOpenGLShader *geomShaderLines;
	QOpenGLShader *fragmentShaderLines;
	QOpenGLVertexArrayObject vao_lines; // a VAO remembers states of buffer objects, allowing to easily bind/unbind different buffer states for rendering different objects in a scene.

	// ------------------------------

	QPoint lastMousePos; // last mouse position (to determine movement delta)

	int projMatShaderLocation; // projection matrix location in shader
	int modelViewMatShaderLocation; // model view matrix location in shader

	QFileSystemWatcher *fileSystemWatcher;

	int currentFrame;
	int nrFrames = 600;
	bool isPlaying;
	qint64 lastTime;

	QElapsedTimer animationTimer;

	// vars to measure fps
	size_t frameCount;
	size_t fps;
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
