#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QGraphicsPixmapItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene=new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Wybierz zdjęcie", "", "Obrazy (*.png *.jpg *.jpeg)", nullptr, QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty()){
        QPixmap image(fileName);
        QGraphicsPixmapItem *item =scene->addPixmap(image);
        ui->graphicsView->fitInView(item, Qt::KeepAspectRatio);
    }
}

