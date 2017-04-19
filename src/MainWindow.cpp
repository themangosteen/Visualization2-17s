#include <QFileDialog>
#include <qmessagebox.h>
#include <QPainter>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui = new Ui_MainWindow();
	ui->setupUi(this);
	QLayout *layout = ui->controls->layout();
	layout->setAlignment(Qt::AlignTop);
	ui->controls->setLayout(layout);

	// enable multisampling for anti-aliasing
	auto format = QSurfaceFormat();
	format.setSamples(16);
	QSurfaceFormat::setDefaultFormat(format);

	glWidget = new GLWidget(this, this);
	ui->glLayout->addWidget(glWidget);
	

	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFileAction()));
	connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(closeAction()));

	connect(ui->frame_slider, SIGNAL(valueChanged(int)), this, SLOT(frameChanged(int)));

	// shading
	connect(ui->ambientSpinBox, SIGNAL(valueChanged(double)), this, SLOT(ambientChanged(double)));
	connect(ui->diffuseSpinBox, SIGNAL(valueChanged(double)), this, SLOT(diffuseChanged(double)));
	connect(ui->specularpinBox, SIGNAL(valueChanged(double)), this, SLOT(specularChanged(double)));

	// render mode
	connect(ui->imposter_switch, SIGNAL(currentIndexChanged(int)), this, SLOT(renderModeChanged(int)));
	connect(ui->alwaysLitCheckBox, SIGNAL(stateChanged(int)), this, SLOT(alwaysLitCheckBoxChanged(int)));
	connect(ui->contourWidthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(contourWidthChanged(double)));

	// animation
	connect(ui->playButton, SIGNAL(clicked()), this, SLOT(playAnimation()));
	connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pauseAnimation()));


	ui->memSizeLCD->setPalette(Qt::darkBlue);
	ui->usedMemLCD->setPalette(Qt::darkGreen);
	ui->fpsLCD->setPalette(Qt::darkGreen);

}

MainWindow::~MainWindow()
{
}


//-------------------------------------------------------------------------------------------------
// Slots
//-------------------------------------------------------------------------------------------------

void MainWindow::openFileAction()
{
	QString filename = QFileDialog::getOpenFileName(this, "Data File", 0, tr("Data Files (*.nc)"));

	if (!filename.isEmpty())
	{
		// store filename
		fileType.filename = filename;
		std::string fn = filename.toStdString();
		bool success = false;

		// progress bar and top label
		ui->progressBar->setEnabled(true);
		ui->labelTop->setText("Loading data ...");

		if (fn.substr(fn.find_last_of(".") + 1) == "nc") // LOAD NetCDF DATA
		{
			int nrFrames;
			success = false; //NetCDFLoader::readData(filename, m_animation, &nrFrames, m_Ui->progressBar);

			ui->frame_slider->setMaximum(nrFrames);
			ui->frame_slider->setValue(0);
			glWidget->initLineRenderMode(&datasetLines);
		}

		ui->progressBar->setEnabled(false);

		// status message
		if (success)
		{
			QString type;
			if (fileType.type == NETCDF) type = "NETCDF";
			ui->labelTop->setText("File LOADED [" + filename + "] - Type [" + type + "]");
		}
		else
		{
			ui->labelTop->setText("ERROR loading file " + filename + "!");
			ui->progressBar->setValue(0);
		}
	}
}

void MainWindow::frameChanged(int value)
{
	if (value < datasetLines.size()) {
		glWidget->setAnimationFrame(value);
	}
	glWidget->update();
	glWidget->repaint();
}

void MainWindow::ambientChanged(double value)
{
	glWidget->ambientFactor = value;
	glWidget->update();
}
void MainWindow::diffuseChanged(double value)
{
	glWidget->diffuseFactor = value;
	glWidget->update();
}
void MainWindow::specularChanged(double value)
{
	glWidget->specularFactor = value;
	glWidget->update();
}

void MainWindow::renderModeChanged(int index)
{
	if (index == 0) {
		ui->contourWidthSpinBox->setEnabled(true);
	}
	else {
		ui->contourWidthSpinBox->setEnabled(false);
	}
	glWidget->update();
}
void MainWindow::contourWidthChanged(double value)
{
	glWidget->lineHaloWidth = value;
	glWidget->update();
}
void MainWindow::alwaysLitCheckBoxChanged(int state)
{
	if (state == 0) {
		glWidget->alwaysLit = 0;
	}
	else {
		glWidget->alwaysLit = 1;
	}
	glWidget->update();
}


void MainWindow::closeAction()
{
	close();
}

void MainWindow::playAnimation()
{
	glWidget->playAnimation();
}

void MainWindow::pauseAnimation()
{
	glWidget->pauseAnimation();
}

void MainWindow::setAnimationFrameGUI(int frame)
{
	ui->frame_slider->setValue(frame);
}

void MainWindow::displayTotalGPUMemory(float size)
{
	// size is in mb

	ui->memSizeLCD->display(size);
}
void MainWindow::displayUsedGPUMemory(float size)
{
	// size is in mb

	float total_mem_mb = (float)glWidget->total_mem_kb / 1024;
	if (size > total_mem_mb*0.9)
		ui->usedMemLCD->setPalette(Qt::red);
	else if (size > total_mem_mb*0.75)
		ui->usedMemLCD->setPalette(Qt::yellow);
	else
		ui->usedMemLCD->setPalette(Qt::darkGreen);

	ui->usedMemLCD->display(size);
}

void MainWindow::displayFPS(int fps)
{
	if (fps < 10)
		ui->fpsLCD->setPalette(Qt::red);
	else if (fps < 25)
		ui->fpsLCD->setPalette(Qt::yellow);
	else
		ui->fpsLCD->setPalette(Qt::darkGreen);

	ui->fpsLCD->display(fps);
}
