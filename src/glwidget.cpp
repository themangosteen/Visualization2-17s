#include "glwidget.h"

#include <QMouseEvent>
#include <QDir>
#ifdef __linux__
#include <GL/glut.h>
#elif _WIN32
#include <gl/GLU.h>
#else
#error platform not supported
#endif
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mainwindow.h"

GLWidget::GLWidget(QWidget *parent, MainWindow *mainWindow)
		: QOpenGLWidget(parent)
{
	mainWindow = mainWindow;

	renderMode = RenderMode::NONE;
	lineTriangleStripWidth = 0.03f;
	lineWidthPercentageBlack = 0.3f;
	lineHaloMaxDepth = 0.01f;
	nrLines = 0;

}

GLWidget::~GLWidget()
{
	delete logger; logger = nullptr;
}

void GLWidget::initShaders()
{
	QString buildDir = QCoreApplication::applicationDirPath();
	simpleLineShader = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	simpleLineShader->addShaderFromSourceFile(QOpenGLShader::Vertex, buildDir + "/shaders/simple_line_shader.vert");
	simpleLineShader->addShaderFromSourceFile(QOpenGLShader::Fragment, buildDir + "/shaders/simple_line_shader.frag");
	simpleLineShader->link();

}

void GLWidget::cleanup()
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	vaoLines.destroy();
	simpleLineShader = nullptr;

	doneCurrent();
}

void GLWidget::initializeGL()
{
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

	QWidget::setFocusPolicy(Qt::FocusPolicy::ClickFocus);

	initializeOpenGLFunctions();
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	// print glError messages
	logger = new QOpenGLDebugLogger(this);
	logger->initialize();
	connect(logger, &QOpenGLDebugLogger::messageLogged, this, &GLWidget::printDebugMsg);
	logger->startLogging();

	if (!vaoLines.create()) {
		qDebug() << "error creating vao";
	}

	initShaders();

	// GL_NVX_gpu_memory_info is an extension by NVIDIA
	// that provides applications visibility into GPU
	// hardware memory utilization
	QString extensions = QString((const char*)glGetString(GL_EXTENSIONS));
	QString vendor = QString((const char*)glGetString(GL_VENDOR));
	QString renderer = QString((const char*)glGetString(GL_RENDERER));
	qDebug() << "Graphics Device:" << renderer << "by" << vendor;
	if (vendor.contains("nvidia", Qt::CaseInsensitive) && extensions.contains("GL_NVX_gpu_memory_info", Qt::CaseInsensitive))
		GL_NVX_gpu_memory_info_supported = true;
	else {
		qDebug() << "OpenGL extension GL_NVX_gpu_memory_info not supported (requires NVIDIA gpu)";
	}
	GLint total_mem_kb = 0;
	GLint cur_avail_mem_kb = 0;
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
	}

	float cur_avail_mem_mb = float(cur_avail_mem_kb) / 1024.0f;
	float total_mem_mb = float(total_mem_kb) / 1024.0f;
	//mainWindow->displayTotalGPUMemory(total_mem_mb); // TODO FIX SEGFAULT
	//mainWindow->displayUsedGPUMemory(0); // TODO FIX SEGFAULT

	// start scene update and paint timer
	connect(&paintTimer, SIGNAL(timeout()), this, SLOT(update()));
	paintTimer.start(16); // draw one frame each interval of 0.016 seconds, 1/0.016 yields about 60 fps
	previousTimeFPS = 0;
	fpsTimer.start();

}


void GLWidget::initLineRenderMode(std::vector<std::vector<LineVertex> > *lines)
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	this->lines = lines;
	renderMode = RenderMode::LINES;

	// allocate data
	allocateGPUBufferLineData();
}

void GLWidget::allocateGPUBufferLineData()
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	// load lines
	// TODO: currently we just use one single line (one single array of vertices)
	nrLines = lines->size();

	std::vector<LineVertex> line1 = (*lines)[0];

	//qDebug() << "line number of vertices (duplicated to draw as triangle strips):" << line1.size();

	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines); // destructor unbinds (i.e. when out of scope)
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

	// ALLOCATE VERTEX DATA ON BUFFER
	// allocate line vertex data to vertex buffer object
	// each vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
	// we store all three attributes interleaved on single vbo [<posdirectionuv><posdirectionuv>...]
	// NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
	vboLines.create();
	vboLines.bind();
	vboLines.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vboLines.allocate(&line1[0], line1.size() * 8 * sizeof(GLfloat));

	// BIND VERTEX BUFFER TO SHADER ATTRIBUTES
	simpleLineShader->bind();

	// important: offset is meant between shader attributes! not between data of individual vertices!
	// for sequential vbo attribute storage [xyzxyzxyzxyzxyz...uvuvuvuvuvuvuv...] we just set offset to first position of uv
	// however here for interleaved attribute storage [xyzxyzuv...xyzxyzuv...], i.e. sequential vertex data storage
	// we must use the stride to indicate the size of the vertex data (here 8 floats) and attribute offset inside the stride
	// attributeStartPos(vertexindex) = vertexindex*stride + offset
	simpleLineShader->enableAttributeArray(0); // assume shader attribute "position" at index 0
	simpleLineShader->setAttributeBuffer(0, GL_FLOAT, 0*sizeof(GLfloat), 3, 8 * sizeof(GL_FLOAT)); // attribute offset 0 byte, 3 floats xyz, vertex stride 8*4 byte
	simpleLineShader->enableAttributeArray(1); // assume shader attribute "direction" at index 1
	simpleLineShader->setAttributeBuffer(1, GL_FLOAT, 3*sizeof(GLfloat), 3, 8 * sizeof(GL_FLOAT)); // attribute offset 3*4 byte, 3 floats xyz, vertex stride 8*4 byte
	simpleLineShader->enableAttributeArray(2); // assume shader attribute "uv" at index 2
	simpleLineShader->setAttributeBuffer(2, GL_FLOAT, 6*sizeof(GLfloat), 2, 8 * sizeof(GL_FLOAT)); // attribute offset 6*4 byte, 2 floats uv, vertex stride 8*4 byte

	// unbind buffer and shader program
	vboLines.release();
	simpleLineShader->release();

	// display memory usage
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
	}
	// check OpenGL error
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		qDebug() << "OpenGL error: " << err;
	}
	//mainWindow->displayUsedGPUMemory(float(total_mem_kb - cur_avail_mem_kb) / 1024.0f); // TODO FIX SEGFAULT!!
}

void GLWidget::paintGL()
{
	calculateFPS();

	switch (renderMode) {
		case(RenderMode::NONE):
			break; // do nothing
		case(RenderMode::LINES):
			drawLines();
			break;
		case(RenderMode::POINTS):
			// TODO
			break;
		default:
			break;
	}
}


void GLWidget::drawLines()
{
	QOpenGLFunctions *glf = QOpenGLContext::currentContext()->functions();

	// BIND BUFFERS AND INIT SHADER UNIFORMS

	// bind vertex array object to bind all vbos associated with it
	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines); // destructor unbinds (i.e. when out of scope)

	// bind shader program and set shader uniforms
	// note that glm uses column vectors, qt uses row vectors, thus transpose
	simpleLineShader->bind();
	QMatrix4x4 viewMat = QMatrix4x4(glm::value_ptr(camera.getViewMatrix())).transposed();
	QMatrix4x4 projMat = QMatrix4x4(glm::value_ptr(camera.getProjectionMatrix())).transposed();
	glm::vec3 camPosGLM = camera.getPosition();
	QVector3D camPos = QVector3D(camPosGLM.x,camPosGLM.y,camPosGLM.z);
	QVector3D lightPos = QVector3D(0, 0, 100);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("viewMat"), viewMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("projMat"), projMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("inverseProjMat"), projMat.inverted());
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lightPos"), lightPos); // in view space
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("cameraPos"), camPos);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("color"), 0.9f, 0.3f, 0.8f);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lineTriangleStripWidth"), lineTriangleStripWidth);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lineWidthPercentageBlack"), lineWidthPercentageBlack);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lineHaloMaxDepth"), lineHaloMaxDepth);


	// DRAW

	glf->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glf->glEnable(GL_BLEND); glf->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glf->glDrawArrays(GL_TRIANGLE_STRIP, 0, (*lines)[0].size()); // TODO change to triangle strip

	simpleLineShader->release();
}

void GLWidget::calculateFPS()
{
	++frameCount;

	// calculate time passed since last frame
	qint64 currentTime = fpsTimer.elapsed(); // in milliseconds
	qint64 timeInterval = currentTime - previousTimeFPS;

	// when timeInterval reaches one second
	if (timeInterval > ((qint64)1000)) {

		// calculate the number of frames per second
		fps = frameCount / (timeInterval / 1000.0f);

		previousTimeFPS = currentTime;
		frameCount = 0;
	}

	//mainWindow->displayFPS(fps);

}

void GLWidget::resizeGL(int w, int h)
{
	camera.setAspect(float(w) / h);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	lastMousePos = event->pos();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	camera.zoom(event->delta() / 30);
	update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastMousePos.x();
	int dy = event->y() - lastMousePos.y();

	// rotate camera
	if (event->buttons() & Qt::LeftButton || event->buttons() & Qt::RightButton) {
		camera.rotateAzimuth(dx / 100.0f);
		camera.rotatePolar(dy / 100.0f);
	}

	lastMousePos = event->pos();
	update();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_Space:
			break;
		default:
			event->ignore();
			break;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{

}

