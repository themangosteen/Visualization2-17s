#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>

#include <QFileDialog>
#include <qmessagebox.h>
#include <QPainter>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);

    QLayout *layout = ui->controls->layout();
    layout->setAlignment(Qt::AlignTop);
    ui->controls->setLayout(layout);

    // enable multisampling for anti-aliasing
    auto format = QSurfaceFormat();
    format.setSamples(16);
    format.setVersion( 3, 3 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    QSurfaceFormat::setDefaultFormat(format);

    glWidget = new GLWidget(this, this);
    ui->glLayout->addWidget(glWidget);


    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFileAction()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(closeAction()));

    connect(ui->contourWidthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(contourWidthChanged(double)));

    ui->memSizeLCD->setPalette(Qt::darkBlue);
    ui->usedMemLCD->setPalette(Qt::darkGreen);
    ui->fpsLCD->setPalette(Qt::darkGreen);

}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::generateTestData(int numVertices, glm::vec3 boundingBoxMin, glm::vec3 boundingBoxMax)
{
	datasetLines.clear();

    std::vector<glm::vec3> line1;
	srand(time(NULL)); // change pseudorandom number generator seed

	// generate random line vertices within given bounding box
	for (int i = 0; i < numVertices; ++i) {
		float rx = ((boundingBoxMax.x - boundingBoxMin.x) * ((float)rand()/RAND_MAX)) + boundingBoxMin.x;
		float ry = ((boundingBoxMax.y - boundingBoxMin.y) * ((float)rand()/RAND_MAX)) + boundingBoxMin.y;
		float rz = ((boundingBoxMax.z - boundingBoxMin.z) * ((float)rand()/RAND_MAX)) + boundingBoxMin.z;
		line1.push_back(glm::vec3(rx, ry, rz));
	}

    datasetLines.push_back(line1);
	qDebug() << "Test line data generated:" << numVertices << "vertices";

	glWidget->initLineRenderMode(&datasetLines);
}

void MainWindow::openFileAction()
{
	QString filename = QFileDialog::getOpenFileName(this, "Open dataset file...", 0, tr("Data Files (*.sop)"));

	if (!filename.isEmpty()) {
        // store filename
        fileType.filename = filename;
        std::string fn = filename.toStdString();
        bool success = false;

        // progress bar and top label
        ui->progressBar->setEnabled(true);
        ui->labelTop->setText("Loading data ...");

		std::string filenameExtension = fn.substr(fn.find_last_of(".") + 1);
		if (filenameExtension == "sop") { // LOAD SOP DATA
			success = false;
			ui->labelTop->setText("Yes we would love to load SOP data too.");
        }
		else {
			success = false;
			ui->labelTop->setText("Error loading file " + filename + ": Unknown filename extension.");
		}

        ui->progressBar->setEnabled(false);

        // status message
		if (success) {
            QString type;
			if (fileType.type == SOP) type = "SOP";
            ui->labelTop->setText("File LOADED [" + filename + "] - Type [" + type + "]");

			glWidget->initLineRenderMode(&datasetLines);
        }
		else {
            ui->labelTop->setText("ERROR loading file " + filename + "!");
            ui->progressBar->setValue(0);
        }
	}
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


void MainWindow::closeAction()
{
    close();
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

void MainWindow::on_generateTestDataButton_clicked()
{
	generateTestData(50, glm::vec3(-1.f,-1.f,-1.f), glm::vec3(1.f,1.f,1.f));
}
