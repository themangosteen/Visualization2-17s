#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <qmessagebox.h>
#include <QPainter>
#include <iostream>

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

void MainWindow::generateTestData()
{

    std::vector<glm::vec3> line1;
    line1.push_back(glm::vec3(0  , 0  , 0));
    line1.push_back(glm::vec3(0.5, 0  , 0));
    line1.push_back(glm::vec3(0.5, 0.2, 0));
    line1.push_back(glm::vec3(1  , 1  , 0));
    datasetLines.push_back(line1);

    qDebug() << "Test line data generated.";

	glWidget->initLineRenderMode(&datasetLines);

}

void MainWindow::openFileAction()
{
	QString filename = QFileDialog::getOpenFileName(this, "Select your cool Spaghetti File!", 0, tr("Data Files (*.nc)"));

    glWidget->initLineRenderMode(&datasetLines);

	if (!filename.isEmpty()) {
        // store filename
        fileType.filename = filename;
        std::string fn = filename.toStdString();
        bool success = false;

        // progress bar and top label
        ui->progressBar->setEnabled(true);
        ui->labelTop->setText("Loading data ...");

		if (fn.substr(fn.find_last_of(".") + 1) == "sop") { // LOAD SOP DATA
            success = false; //NetCDFLoader::readData(filename, m_animation, &nrFrames, m_Ui->progressBar);
            ui->labelTop->setText("Loading SOP data is unfortunately not supported as we were not able to find any data about it :/ ");
			glWidget->initLineRenderMode(&datasetLines);
        }

        ui->progressBar->setEnabled(false);

        // status message
		if (success) {
            QString type;
			if (fileType.type == SOP) type = "SOP";
            ui->labelTop->setText("File LOADED [" + filename + "] - Type [" + type + "]");
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
	generateTestData();
}
