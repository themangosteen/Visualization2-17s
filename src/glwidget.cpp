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
	lineHaloWidth = 0.03f;
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


void GLWidget::initLineRenderMode(std::vector<std::vector<glm::vec3> > *lines, std::vector<std::vector<glm::vec3> > *linesDirections, std::vector<std::vector<glm::vec2> > *linesUV)
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	this->lines = lines;
	this->linesDirections = linesDirections;
	this->linesUV = linesUV;
	renderMode = RenderMode::LINES;

	// allocate data
	allocateGPUBufferLineData();

}

void GLWidget::allocateGPUBufferLineData()
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	// load lines
	nrLines = lines->size();

	// TODO create struct of line vertices
	// each vertex should contain: glm::vec3 position, glm::vec2 texturecoords, glm::vec3 line direction

	for (size_t i = 0; i < nrLines; ++i) {
		std::vector<glm::vec3> line = (*lines)[i];
	}

	std::vector<glm::vec3> line1 = (*lines)[0];
	std::vector<glm::vec3> line1Directions = (*linesDirections)[0];
	std::vector<glm::vec2> line1UV = (*linesUV)[0];

	qDebug() << "line number of vertices:" << line1.size();

	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines); // destructor unbinds (i.e. when out of scope)
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

	// POSITION
	// allocate line vertex data to vertex buffer object
	vboLines.create();
	vboLines.bind();
	vboLines.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vboLines.allocate(&line1[0], line1.size() * sizeof(glm::vec3)); // just first line for now

	// bind vbo to shader attribute
	simpleLineShader->bind();
	int positionAttribShaderIndex = simpleLineShader->attributeLocation("position");
	simpleLineShader->enableAttributeArray(positionAttribShaderIndex); // enable bound vertex buffer at this index
	simpleLineShader->setAttributeBuffer(positionAttribShaderIndex, GL_FLOAT, 0, 3); // 3 components x,y,z
	vboLines.release();

	// DIRECTION
	// allocate line direction data to vertex buffer object
	vboLinesDirection.create();
	vboLinesDirection.bind();
	vboLinesDirection.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vboLinesDirection.allocate(&line1Directions[0], line1Directions.size() * sizeof(glm::vec3)); // just first line for now

	// bind vbo to shader attribute
	int directionAttribShaderIndex = simpleLineShader->attributeLocation("direction");
	simpleLineShader->enableAttributeArray(directionAttribShaderIndex); // enable bound vertex buffer at this index
	simpleLineShader->setAttributeBuffer(directionAttribShaderIndex, GL_FLOAT, 0, 3); // 3 components x,y,z
	vboLinesDirection.release();

	// UV
	// allocate line direction data to vertex buffer object
	vboLinesUV.create();
	vboLinesUV.bind();
	vboLinesUV.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vboLinesUV.allocate(&line1UV[0], line1UV.size() * sizeof(glm::vec2)); // just first line for now

	// bind vbo to shader attribute
	int uvAttribShaderIndex = simpleLineShader->attributeLocation("uv");
	simpleLineShader->enableAttributeArray(uvAttribShaderIndex); // enable bound vertex buffer at this index
	simpleLineShader->setAttributeBuffer(uvAttribShaderIndex, GL_FLOAT, 0, 2); // 2 components u,v
	vboLinesUV.release();

	// unbind shader program
	simpleLineShader->release();

	// display memory usage
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
	}
	// check OpenGL error
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		qDebug() << "!!!OpenGL error: " << err;
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
	//QVector3D cameraPos = QVector3D(viewMat * QVector4D(m_camera.getPosition().x, m_camera.getPosition().y, m_camera.getPosition().z, 1));
	glm::vec3 camPosGLM = camera.getPosition();
	QVector3D camPos = QVector3D(camPosGLM.x,camPosGLM.y,camPosGLM.z);
//	qDebug() << "CameraPos: "<<camPos;
	QVector3D lightPos = QVector3D(0, 0, 100);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("viewMat"), viewMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("projMat"), projMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lightPos"), lightPos); // in view space
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lineHaloWidth"), lineHaloWidth);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("color"), 0.9f, 0.3f, 0.8f);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("cameraPos"), camPos);

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

