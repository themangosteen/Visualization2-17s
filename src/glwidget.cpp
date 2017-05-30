#include "glwidget.h"

#include <QMouseEvent>
#include <QDir>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mainwindow.h"

GLWidget::GLWidget(QWidget *parent, MainWindow *mainWindow)
		: QOpenGLWidget(parent)
{
	mainWindow = mainWindow;

	// since Qt puts MainWindow and its UI in another thread than GLWidget
	// we must use signal/slots to communicate instead of accessing MainWindow functions directly
	connect(this, &GLWidget::totalGPUMemoryChanged, mainWindow, &MainWindow::displayTotalGPUMemory);
	connect(this, &GLWidget::usedGPUMemoryChanged, mainWindow, &MainWindow::displayUsedGPUMemory);
	connect(this, &GLWidget::fpsChanged, mainWindow, &MainWindow::displayFPS);
	connect(this, &GLWidget::graphicsDeviceInfoChanged, mainWindow, &MainWindow::displayGraphicsDeviceInfo);

	renderMode = RenderMode::NONE;
	lineTriangleStripWidth = 0.03f;
	lineWidthPercentageBlack = 0.3f;
	lineWidthDepthCueingFactor = 1.0f;
	lineHaloMaxDepth = 0.02f;
	enableClipping = false;
	updateClipPlaneNormal();
	clipPlaneDistance = 0;

	nrLines = 0;

}

GLWidget::~GLWidget()
{
	delete logger; logger = nullptr;
}

void GLWidget::initShaders()
{
	QString buildDir = QCoreApplication::applicationDirPath();
	shaderLinesWithHalos = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	shaderLinesWithHalos->addShaderFromSourceFile(QOpenGLShader::Vertex, buildDir + "/shaders/shader_lines_with_halos.vert");
	shaderLinesWithHalos->addShaderFromSourceFile(QOpenGLShader::Fragment, buildDir + "/shaders/shader_lines_with_halos.frag");
	shaderLinesWithHalos->link();

}

void GLWidget::cleanup()
{
	// makes the widget's rendering context the current OpenGL rendering context
	makeCurrent();

	for(int i=0; i<lines->size(); i++) {
		(*vaoLines[i]).destroy();
	}
	shaderLinesWithHalos = nullptr;

	doneCurrent();
}

void GLWidget::initializeGL()
{
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

	QWidget::setFocusPolicy(Qt::FocusPolicy::ClickFocus);

	initializeOpenGLFunctions();

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	// print glError messages
	logger = new QOpenGLDebugLogger(this);
	logger->initialize();
	connect(logger, &QOpenGLDebugLogger::messageLogged, this, &GLWidget::printDebugMsg);
	logger->startLogging();

//	if (!vaoLines.create()) {
//		qDebug() << "error creating vao";
//	}

	initShaders();

	// get graphics device and opengl info
	QString extensions = QString((const char*)glGetString(GL_EXTENSIONS));
	QString glversion = QString((const char*)glGetString(GL_VERSION));
	QString vendor = QString((const char*)glGetString(GL_VENDOR));
	QString renderer = QString((const char*)glGetString(GL_RENDERER));

	// GL_NVX_gpu_memory_info is an extension by NVIDIA
	// that provides applications visibility into GPU
	// hardware memory utilization
	if (vendor.contains("nvidia", Qt::CaseInsensitive) && extensions.contains("GL_NVX_gpu_memory_info", Qt::CaseInsensitive))
		GL_NVX_gpu_memory_info_supported = true;
	else {
		qDebug() << "OpenGL extension GL_NVX_gpu_memory_info not supported (requires NVIDIA gpu)";
	}

	QString deviceInfoString = "Graphics Device:\n" + renderer + "\nby " + vendor + "\n\nOpenGL version: " + glversion + (GL_NVX_gpu_memory_info_supported ? "" : "\n\n[!] GL_NVX_gpu_memory_info not supported (needs NVIDIA)");
	qDebug().noquote() << deviceInfoString;
	emit graphicsDeviceInfoChanged(deviceInfoString);

	GLint total_mem_kb = 0;
	GLint cur_avail_mem_kb = 0;
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
	}

	float cur_avail_mem_mb = float(cur_avail_mem_kb) / 1024.0f;
	float total_mem_mb = float(total_mem_kb) / 1024.0f;
	emit totalGPUMemoryChanged(total_mem_mb);
	emit usedGPUMemoryChanged(0);

	// start scene update and paint timer
	connect(&paintTimer, SIGNAL(timeout()), this, SLOT(update()));
	paintTimer.start(16); // draw one frame each interval of 0.016 seconds, 1/0.016 yields about 60 fps
	previousTimeFPS = 0;
	fpsTimer.start();

	qDebug() << ""; // newline

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
	nrLines = lines->size();
// BIND VERTEX BUFFER TO SHADER ATTRIBUTES
	for(int i = 0; i<nrLines; i++) {


		std::vector<LineVertex> line1 = (*lines)[i];
//        std::vector<LineVertex> line1 = lines[i];

		//qDebug() << "line number of vertices (duplicated to draw as triangle strips):" << line1.size();
//		QOpenGLVertexArrayObject vaoLine;
//
//		if (!vaoLine.create()) {
//			qDebug() << "error creating vao";
//		}
		vaoLines.push_back(new QOpenGLVertexArrayObject());
		QOpenGLVertexArrayObject* vaoLine = vaoLines[vaoLines.size()-1];
		QOpenGLVertexArrayObject::Binder vaoBinder(vaoLine); // destructor unbinds (i.e. when out of scope)
		QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

		// ALLOCATE VERTEX DATA ON BUFFER
		// allocate line vertex data to vertex buffer object
		// each vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
		// we store all three attributes interleaved on single vbo [<posdirectionuv><posdirectionuv>...]
		// NOTE: we store two sequential copies of each vertex (one with v = 0, one with v = 1) to draw lines as triangle strips
		vboLines.push_back(new QOpenGLBuffer());
		QOpenGLBuffer* vboLine = vboLines[vboLines.size()-1];
		vboLine->create();
		vboLine->bind();
		vboLine->setUsagePattern(QOpenGLBuffer::StaticDraw);
		vboLine->allocate(&line1[0], line1.size() * 8 * sizeof(GLfloat));


		shaderLinesWithHalos->bind();
// important: offset is meant between shader attributes! not between data of individual vertices!
		// for sequential vbo attribute storage [xyzxyzxyzxyzxyz...uvuvuvuvuvuvuv...] we just set offset to first position of uv
		// however here for interleaved attribute storage [xyzxyzuv...xyzxyzuv...], i.e. sequential vertex data storage
		// we must use the stride to indicate the size of the vertex data (here 8 floats) and attribute offset inside the stride
		// attributeStartPos(vertexindex) = vertexindex*stride + offset
		shaderLinesWithHalos->enableAttributeArray(0); // assume shader attribute "position" at index 0
		shaderLinesWithHalos->setAttributeBuffer(0, GL_FLOAT, 0 * sizeof(GLfloat), 3, 8 *
																					  sizeof(GL_FLOAT)); // attribute offset 0 byte, 3 floats xyz, vertex stride 8*4 byte
		shaderLinesWithHalos->enableAttributeArray(1); // assume shader attribute "direction" at index 1
		shaderLinesWithHalos->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(GLfloat), 3, 8 *
																					  sizeof(GL_FLOAT)); // attribute offset 3*4 byte, 3 floats xyz, vertex stride 8*4 byte
		shaderLinesWithHalos->enableAttributeArray(2); // assume shader attribute "uv" at index 2
		shaderLinesWithHalos->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(GLfloat), 2, 8 *
																					  sizeof(GL_FLOAT)); // attribute offset 6*4 byte, 2 floats uv, vertex stride 8*4 byte

		// unbind buffer and shader program
		vboLine->release();

		shaderLinesWithHalos->release();
	}





	// display memory usage
	if (GL_NVX_gpu_memory_info_supported) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &cur_avail_mem_kb);
	}
	// check OpenGL error
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		qDebug() << "OpenGL error:" << err;
	}

	emit usedGPUMemoryChanged(float(total_mem_kb - cur_avail_mem_kb) / 1024.0f);
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

	for(int i = 0; i<vaoLines.size(); i++){


		// BIND BUFFERS AND INIT SHADER UNIFORMS

		// bind vertex array object to bind all vbos associated with it
		QOpenGLVertexArrayObject::Binder vaoBinder(vaoLines[i]); // destructor unbinds (i.e. when out of scope)


// bind shader program and set shader uniforms
		// note that glm uses column vectors, qt uses row vectors, thus transpose
		shaderLinesWithHalos->bind();
		QMatrix4x4 viewMat = QMatrix4x4(glm::value_ptr(camera.getViewMatrix())).transposed();
		QMatrix4x4 projMat = QMatrix4x4(glm::value_ptr(camera.getProjectionMatrix())).transposed();
		glm::vec3 camPosGLM = camera.getPosition();
		QVector3D camPos = QVector3D(camPosGLM.x,camPosGLM.y,camPosGLM.z);
		QVector3D lightPos = QVector3D(0, 0, 100);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("viewMat"), viewMat);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("projMat"), projMat);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("inverseProjMat"), projMat.inverted());
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("lightPos"), lightPos); // in view space
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("cameraPos"), camPos);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("colorLine"), 0.0f, 0.0f, 0.0f);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("colorHalo"), 1.0f, 1.0f, 1.0f);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("lineTriangleStripWidth"), lineTriangleStripWidth);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("lineWidthPercentageBlack"), lineWidthPercentageBlack);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("lineWidthDepthCueingFactor"), lineWidthDepthCueingFactor);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("lineHaloMaxDepth"), lineHaloMaxDepth);
		QVector3D clipPlaneN = QVector3D(clipPlaneNormal.x, clipPlaneNormal.y, clipPlaneNormal.z);
		if (!enableClipping)
			clipPlaneN = QVector3D(0,0,0);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("clipPlaneNormal"), clipPlaneN);
		shaderLinesWithHalos->setUniformValue(shaderLinesWithHalos->uniformLocation("clipPlaneDistance"), clipPlaneDistance);

		// DRAW

		glf->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glf->glDrawArrays(GL_TRIANGLE_STRIP, 0, (*lines)[i].size());

	}
	shaderLinesWithHalos->release();

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

	emit fpsChanged(fps);

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

