#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QImageReader>
#include <QImage>
#include <QPainter>
#include <QGraphicsPixmapItem>


//Konstruktor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Inicjalizacja płótna do układania zdjęć
    plotno = new QGraphicsScene(this);
    plotno->setSceneRect(0, 0, 561, 631);
    ui->graphicsView->setScene(plotno);

    //Połączenie przycisków z funkcjami
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::dodajZdjecie);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::usunZdjecie);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::zapiszKolaz);

    //Usunięcie scrollbarów
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//Funkcja dodawania zdjęć
void MainWindow::dodajZdjecie()
{
    QString dostepneFormaty = "Obrazy (*.png *.jpg *.jpeg);;Wszystkie pliki (*)";
    QString sciezkaPliku = QFileDialog::getOpenFileName(this, "Wybierz zdjęcie", "", dostepneFormaty);

    if (sciezkaPliku.isEmpty()) return;

    QImageReader czytnik(sciezkaPliku); //Automatyczne ustawianie orientacji zdjęcia
    czytnik.setAutoTransform(true);

    QImage obrazZPliku = czytnik.read();
    if (!obrazZPliku.isNull()) {
        QPixmap piksMapa = QPixmap::fromImage(obrazZPliku);
        QPixmap obrazPrzeskalowany = piksMapa.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation); //Wstępne skalowanie zdjęcia

        //Tworzenie obiektu który możemy przesuwac na płótnie
        QGraphicsPixmapItem *obiektGraficzny = plotno->addPixmap(obrazPrzeskalowany);

        //Możliwość zaznaczania i przesuwania
        obiektGraficzny->setFlag(QGraphicsItem::ItemIsMovable);
        obiektGraficzny->setFlag(QGraphicsItem::ItemIsSelectable);

        //Ustawienie środka do obracania i skalowania
        obiektGraficzny->setTransformOriginPoint(obiektGraficzny->boundingRect().center());
        obiektGraficzny->setTransformationMode(Qt::SmoothTransformation);
    }
}

//Usuwanie zaznaczonych zdjęć
void MainWindow::usunZdjecie()
{
    for (QGraphicsItem* wybranyObiekt : plotno->selectedItems()) {
        plotno->removeItem(wybranyObiekt); //usunięcie z płótna
        delete wybranyObiekt; //usuniięcie z pamięci
    }
}

//Skalowanie i obracanie klawiaturą
void MainWindow::keyPressEvent(QKeyEvent *klawisz)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsItem *wybranyObiekt = zaznaczone.first();

    //logika klawiszy
    if (klawisz->key() == Qt::Key_Plus || klawisz->key() == Qt::Key_Equal) {
        wybranyObiekt->setScale(wybranyObiekt->scale() + 0.05);  //powiekszenie 5%
    }
    else if (klawisz->key() == Qt::Key_Minus) {
        if (wybranyObiekt->scale() > 0.1) { //zablokowanie żeby zdjęcie nie zniknęło
            wybranyObiekt->setScale(wybranyObiekt->scale() - 0.05); //pomniejszenie 5%
        }
    }
    else if (klawisz->key() == Qt::Key_R) {
        wybranyObiekt->setRotation(wybranyObiekt->rotation() + 5); //obrót 5 stopni w prawo
    }
    else if (klawisz->key() == Qt::Key_L) {
        wybranyObiekt->setRotation(wybranyObiekt->rotation() - 5); //5 stopni w lewo
    }
}


//Zapisywanie kolażu do pliku
void MainWindow::zapiszKolaz()
{
    QString formatyZapisu = "Obraz PNG (*.png);;Obraz JPG (*.jpg)";
    QString wybranyFormat;

    QString miejsceZapisu = QFileDialog::getSaveFileName(this, "Zapisz kolaż", "", formatyZapisu, &wybranyFormat); //okno zapisu

    if (miejsceZapisu.isEmpty()) return;

    if (wybranyFormat.contains("png") && !miejsceZapisu.endsWith(".png", Qt::CaseInsensitive)) {
        miejsceZapisu += ".png";
    } else if (wybranyFormat.contains("jpg") && !miejsceZapisu.endsWith(".jpg", Qt::CaseInsensitive)) {
        miejsceZapisu += ".jpg";
    }

    plotno->clearSelection();

    //tworzenie pustego obrazu o rozmiarze plótna
    QImage obrazFinalny(plotno->sceneRect().size().toSize(), QImage::Format_ARGB32);
    obrazFinalny.fill(Qt::white);  //białe tło

    //Przerysowywanie tego co jest na płótnie do pliku graficznego
    QPainter malarz(&obrazFinalny);

    plotno->render(&malarz);
    malarz.end();

    obrazFinalny.save(miejsceZapisu); //Zapisanie na dysku
}