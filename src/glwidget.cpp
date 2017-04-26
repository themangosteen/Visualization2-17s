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
	lineHaloWidth = 0.2f;
	nrLines = 0;

}

GLWidget::~GLWidget()
{
	delete logger; logger = nullptr;
}

void GLWidget::initShaders()
{
	simpleLineShader = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	simpleLineShader->addShaderFromSourceFile(QOpenGLShader::Vertex, "/shaders/simple_line_shader.vert");
	simpleLineShader->addShaderFromSourceFile(QOpenGLShader::Fragment, "/shaders/simple_line_shader.frag");
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
	glClearColor(0.862f, 0.929f, 0.949f, 1.0f);
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


void GLWidget::initLineRenderMode(std::vector<std::vector<glm::vec3> > *lines)
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	this->lines = lines;
	renderMode = RenderMode::LINES;

	simpleLineShader->bind();

	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines);
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

	// TODO: bind positions etc.

	simpleLineShader->release();
	allocateGPUBuffer();
}

void GLWidget::allocateGPUBuffer()
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	// load lines
	nrLines = lines->size();

	for (size_t i = 0; i < nrLines; ++i) {
		std::vector<glm::vec3> line = (*lines)[i];
	}

	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines); // destructor unbinds (i.e. when out of scope)
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

	/* TODO
	// allocate data to buffers (positions, radii, colors) and bind buffers to shaders

	m_vbo_pos.create();
	m_vbo_pos.bind();
	m_vbo_pos.setUsagePattern(QOpenGLBuffer::StaticDraw); // maybe just create once and use StreamDraw
	m_vbo_pos.allocate(&m_pos[0], m_pos.size() * sizeof(glm::vec3));
	int atomPosAttribIndex = shaderprogram_lines->attributeLocation("atomPos");
	shaderprogram_lines->enableAttributeArray(atomPosAttribIndex); // enable bound vertex buffer at this index
	shaderprogram_lines->setAttributeBuffer(atomPosAttribIndex, GL_FLOAT, 0, 3); // 3 components x,y,z

	m_vbo_radii.create();
	m_vbo_radii.bind();
	m_vbo_radii.setUsagePattern(QOpenGLBuffer::StaticDraw); // maybe just create once and use StreamDraw
	m_vbo_radii.allocate(&m_radii[0], m_radii.size() * sizeof(float));
	int atomRadiusAttribIndex = shaderprogram_lines->attributeLocation("atomRadius");
	shaderprogram_lines->enableAttributeArray(atomRadiusAttribIndex); // enable bound vertex buffer at this index
	shaderprogram_lines->setAttributeBuffer(atomRadiusAttribIndex, GL_FLOAT, 0, 1); // 1 component

	m_vbo_colors.create();
	m_vbo_colors.bind();
	m_vbo_colors.setUsagePattern(QOpenGLBuffer::StaticDraw); // maybe just create once and use StreamDraw
	m_vbo_colors.allocate(&m_colors[0], m_colors.size() * sizeof(glm::vec3));
	int atomColorAttribIndex = shaderprogram_lines->attributeLocation("atomColor");
	shaderprogram_lines->enableAttributeArray(atomColorAttribIndex); // enable bound vertex buffer at this index
	shaderprogram_lines->setAttributeBuffer(atomColorAttribIndex, GL_FLOAT, 0, 3); // 3 components x,y,z

	// unbind buffers and shader program
	m_vbo_pos.release();
	m_vbo_radii.release();
	m_vbo_colors.release();
	shaderprogram_lines->release();
	*/

	// display memory usage
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
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

	/* BIND BUFFERS AND INIT SHADER UNIFORMS */

	// bind vertex array object to bind all vbos associated with it
	QOpenGLVertexArrayObject::Binder vaoBinder(&vaoLines); // destructor unbinds (i.e. when out of scope)
	// bind shader program
	simpleLineShader->bind();

	// set shader uniforms
	// note that glm uses column vectors, qt uses row vectors, thus transpose
	QMatrix4x4 viewMat = QMatrix4x4(glm::value_ptr(camera.getViewMatrix())).transposed();
	QMatrix4x4 projMat = QMatrix4x4(glm::value_ptr(camera.getProjectionMatrix())).transposed();
	//QVector3D cameraPos = QVector3D(viewMat * QVector4D(m_camera.getPosition().x, m_camera.getPosition().y, m_camera.getPosition().z, 1));
	QVector3D lightPos = QVector3D(0, 0, 100);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("viewMat"), viewMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("projMat"), projMat);
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lightPos"), lightPos); // in view space
	simpleLineShader->setUniformValue(simpleLineShader->uniformLocation("lineHaloWidth"), lineHaloWidth);

	/* DRAW */

	glf->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glf->glEnable(GL_BLEND); glf->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	size_t nrLines = 0;
	glf->glDrawArrays(GL_POINTS, 0, nrLines);

	// unbind shader program
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
	if (event->buttons() & Qt::LeftButton) {
		camera.rotateAzimuth(dx / 100.0f);
		camera.rotatePolar(dy / 100.0f);
	}
	if (event->buttons() & Qt::RightButton) {
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

