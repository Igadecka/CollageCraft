#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QImageReader>
#include <QImage>
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QFileInfo>
#include <opencv2/opencv.hpp>

//Konstruktor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("CollageCraft");

    //Inicjalizacja płótna do układania zdjęć
    plotno = new QGraphicsScene(this);
    plotno->setSceneRect(0, 0, 561, 631);
    ui->graphicsView->setScene(plotno);

    //Połączenie przycisków z funkcjami
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::dodajZdjecie);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::usunZdjecie);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::zapiszKolaz);
    connect(plotno, &QGraphicsScene::selectionChanged, this, &MainWindow::odswiezListeWarstw);

    connect(plotno, &QGraphicsScene::selectionChanged, this, &MainWindow::aktualizujSuwaki);
    connect(ui->sliderSzary, &QSlider::valueChanged, this, &MainWindow::filtrSzary);
    connect(ui->sliderJasnosc, &QSlider::valueChanged, this, &MainWindow::filtrJasnosc);
    connect(ui->sliderKontrast, &QSlider::valueChanged, this, &MainWindow::filtrKontrast);
    connect(ui->sliderNegatyw, &QSlider::valueChanged, this, &MainWindow::filtrNegatyw);
    connect(ui->sliderSepia, &QSlider::valueChanged, this, &MainWindow::filtrSepia);
    connect(ui->przyciskUsunFiltry, &QPushButton::clicked, this, &MainWindow::resetujFiltry);

    connect(ui->comboKsztalt, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::podgladWyciecia);
    connect(ui->sliderRozmiarWyciecia, &QSlider::valueChanged, this, &MainWindow::podgladWyciecia);
    connect(ui->btnWytnij, &QPushButton::clicked, this, &MainWindow::zastosujWyciecie);
    connect(plotno, &QGraphicsScene::selectionChanged, this, &MainWindow::ukryjPodgladWyciecia);

    connect(ui->listaNaklejek, &QListWidget::itemDoubleClicked, this, &MainWindow::dodajNaklejke);

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
        QPixmap obrazPrzeskalowany = piksMapa.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        //Tworzenie obiektu który możemy przesuwac na płótnie
        QGraphicsPixmapItem *obiektGraficzny = plotno->addPixmap(obrazPrzeskalowany);

        //Możliwość zaznaczania i przesuwania
        obiektGraficzny->setFlag(QGraphicsItem::ItemIsMovable);
        obiektGraficzny->setFlag(QGraphicsItem::ItemIsSelectable);

        //Ustawienie środka do obracania i skalowania
        obiektGraficzny->setTransformOriginPoint(obiektGraficzny->boundingRect().center());
        obiektGraficzny->setTransformationMode(Qt::SmoothTransformation);

        //nadanie numeru warstwy
        qreal najwyzszePietro = 0;
        for (QGraphicsItem *element : plotno->items()) {
            if (element->zValue() > najwyzszePietro) {
                najwyzszePietro = element->zValue();
            }
        }
        obiektGraficzny->setZValue(najwyzszePietro + 1.0);

        QFileInfo info(sciezkaPliku);
        obiektGraficzny->setData(0, info.fileName());
        obiektGraficzny->setData(1, QVariant::fromValue(obrazPrzeskalowany.toImage()));

        odswiezListeWarstw();
    }
}

//Usuwanie zaznaczonych zdjęć
void MainWindow::usunZdjecie()
{
    for (QGraphicsItem* wybranyObiekt : plotno->selectedItems()) {
        plotno->removeItem(wybranyObiekt); //usunięcie z płótna
        delete wybranyObiekt; //usunięcie z pamięci
    }
    odswiezListeWarstw();
}

//Skalowanie i obracanie klawiaturą + warstwy
void MainWindow::keyPressEvent(QKeyEvent *klawisz)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsItem *wybranyObiekt = zaznaczone.first();

    //logika klawiszy
    if (klawisz->key() == Qt::Key_Plus || klawisz->key() == Qt::Key_Equal) {
        wybranyObiekt->setScale(wybranyObiekt->scale() + 0.05);
    }
    else if (klawisz->key() == Qt::Key_Minus) {
        if (wybranyObiekt->scale() > 0.1) {
            wybranyObiekt->setScale(wybranyObiekt->scale() - 0.05);
        }
    }
    else if (klawisz->key() == Qt::Key_R) {
        wybranyObiekt->setRotation(wybranyObiekt->rotation() + 5);
    }
    else if (klawisz->key() == Qt::Key_L) {
        wybranyObiekt->setRotation(wybranyObiekt->rotation() - 5);
    }

    //warstwy W, S, E, Q
    else if (klawisz->key() == Qt::Key_W) {
        QGraphicsItem *elementWyzej = nullptr;
        qreal najmniejszaRoznica = 999999;
        for (QGraphicsItem *inny : plotno->items()) {
            if (inny != wybranyObiekt && inny->zValue() > wybranyObiekt->zValue()) {
                qreal roznica = inny->zValue() - wybranyObiekt->zValue();
                if (roznica < najmniejszaRoznica) {
                    najmniejszaRoznica = roznica;
                    elementWyzej = inny;
                }
            }
        }
        if (elementWyzej) {
            qreal mojeZ = wybranyObiekt->zValue();
            wybranyObiekt->setZValue(elementWyzej->zValue());
            elementWyzej->setZValue(mojeZ);
        }
    }
    else if (klawisz->key() == Qt::Key_S) {
        QGraphicsItem *elementNizej = nullptr;
        qreal najmniejszaRoznica = 999999;
        for (QGraphicsItem *inny : plotno->items()) {
            if (inny != wybranyObiekt && inny->zValue() < wybranyObiekt->zValue()) {
                qreal roznica = wybranyObiekt->zValue() - inny->zValue();
                if (roznica < najmniejszaRoznica) {
                    najmniejszaRoznica = roznica;
                    elementNizej = inny;
                }
            }
        }
        if (elementNizej) {
            qreal mojeZ = wybranyObiekt->zValue();
            wybranyObiekt->setZValue(elementNizej->zValue());
            elementNizej->setZValue(mojeZ);
        }
    }
    else if (klawisz->key() == Qt::Key_E || klawisz->key() == Qt::Key_Q) {
        qreal maksymalneZ = wybranyObiekt->zValue();
        qreal minimalneZ = wybranyObiekt->zValue();
        for (QGraphicsItem *element : plotno->items()) {
            if (element->zValue() > maksymalneZ) maksymalneZ = element->zValue();
            if (element->zValue() < minimalneZ) minimalneZ = element->zValue();
        }
        if (klawisz->key() == Qt::Key_E) {
            wybranyObiekt->setZValue(maksymalneZ + 1.0);
        } else if (klawisz->key() == Qt::Key_Q) {
            wybranyObiekt->setZValue(minimalneZ - 1.0);
        }
    }

    odswiezListeWarstw();
}

//Zapisywanie kolażu do pliku
void MainWindow::zapiszKolaz()
{
    QString formatyZapisu = "Obraz PNG (*.png);;Obraz JPG (*.jpg)";
    QString wybranyFormat;
    QString miejsceZapisu = QFileDialog::getSaveFileName(this, "Zapisz kolaż", "", formatyZapisu, &wybranyFormat);

    if (miejsceZapisu.isEmpty()) return;

    if (wybranyFormat.contains("png") && !miejsceZapisu.endsWith(".png", Qt::CaseInsensitive)) {
        miejsceZapisu += ".png";
    } else if (wybranyFormat.contains("jpg") && !miejsceZapisu.endsWith(".jpg", Qt::CaseInsensitive)) {
        miejsceZapisu += ".jpg";
    }

    plotno->clearSelection();

    QImage obrazFinalny(plotno->sceneRect().size().toSize(), QImage::Format_ARGB32);
    obrazFinalny.fill(Qt::white);

    QPainter malarz(&obrazFinalny);
    plotno->render(&malarz);
    malarz.end();

    obrazFinalny.save(miejsceZapisu);
}

void MainWindow::odswiezListeWarstw()
{
    ui->listaWarstw->clear();
    const QList<QGraphicsItem*> elementy = plotno->items(Qt::DescendingOrder);

    for (QGraphicsItem *element : elementy) {
        QString nazwa = element->data(0).toString();
        if (nazwa.isEmpty()) nazwa = "Zdjęcie";

        QListWidgetItem *wpis = new QListWidgetItem(nazwa);
        if (element->isSelected()) {
            wpis->setForeground(QColor(0, 200, 0));
            QFont czcionka = wpis->font();
            czcionka.setBold(true);
            wpis->setFont(czcionka);
        }
        ui->listaWarstw->addItem(wpis);
    }
}

//zmiana obrazu z Qt (QImage) na OpenCV (cv::Mat)
cv::Mat MainWindow::QImageToCvMat(const QImage &inImage)
{
    QImage obrazRGB = inImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat macierz(obrazRGB.height(), obrazRGB.width(),
                    CV_8UC3,
                    (cv::Scalar*)obrazRGB.constBits(),
                    obrazRGB.bytesPerLine());

    cv::Mat macierzBGR;
    cv::cvtColor(macierz, macierzBGR, cv::COLOR_RGB2BGR);
    return macierzBGR;
}

//zmiana obrazu z OpenCV (cv::Mat) na Qt (QImage)
QImage MainWindow::cvMatToQImage(const cv::Mat &inMat)
{
    if (inMat.channels() == 1) {
        QImage obrazQt((const unsigned char*)(inMat.data),
                       inMat.cols, inMat.rows,
                       inMat.step,
                       QImage::Format_Grayscale8);
        return obrazQt.copy();
    }
    else if (inMat.channels() == 3) {
        cv::Mat macierzRGB;
        cv::cvtColor(inMat, macierzRGB, cv::COLOR_BGR2RGB);

        QImage obrazQt((const unsigned char*)(macierzRGB.data),
                       macierzRGB.cols, macierzRGB.rows,
                       macierzRGB.step,
                       QImage::Format_RGB888);
        return obrazQt.copy();
    }
    return QImage();
}


void MainWindow::aktualizujWygladElementu(QGraphicsPixmapItem *wybranyObiekt)
{
    if (!wybranyObiekt) return;

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    cv::Mat obrazRoboczy = QImageToCvMat(oryginalQt);

    int jasnosc = wybranyObiekt->data(2).toInt();
    int kontrast = wybranyObiekt->data(3).toInt();
    int szarosc = wybranyObiekt->data(4).toInt();
    int sepia = wybranyObiekt->data(5).toInt();
    int negatyw = wybranyObiekt->data(6).toInt();

    QVariant daneKształtu = wybranyObiekt->data(7);
    double skalaWyciecia = wybranyObiekt->data(8).toDouble();


    // Jasność i Kontrast
    if (jasnosc != 0 || kontrast != 0) {
        double alfa = (kontrast + 100.0) / 100.0;
        double beta = 128.0 * (1.0 - alfa) + jasnosc;
        obrazRoboczy.convertTo(obrazRoboczy, -1, alfa, beta);
    }

    // Szarość
    if (szarosc > 0) {
        cv::Mat szary1, szary3;
        cv::cvtColor(obrazRoboczy, szary1, cv::COLOR_BGR2GRAY);
        cv::cvtColor(szary1, szary3, cv::COLOR_GRAY2BGR);
        double wagaSzarosci = szarosc / 100.0;
        cv::addWeighted(szary3, wagaSzarosci, obrazRoboczy, 1.0 - wagaSzarosci, 0.0, obrazRoboczy);
    }

    // Sepia
    if (sepia > 0) {
        cv::Mat obrazSepia;
        cv::Mat macierzSepii = (cv::Mat_<float>(3,3) << 0.272, 0.534, 0.131, 0.349, 0.686, 0.168, 0.393, 0.769, 0.189);
        cv::transform(obrazRoboczy, obrazSepia, macierzSepii);
        double wagaSepii = sepia / 100.0;
        cv::addWeighted(obrazSepia, wagaSepii, obrazRoboczy, 1.0 - wagaSepii, 0.0, obrazRoboczy);
    }

    // Negatyw
    if (negatyw > 0) {
        cv::Mat obrazNegatyw;
        cv::bitwise_not(obrazRoboczy, obrazNegatyw);
        double wagaNegatywu = negatyw / 100.0;
        cv::addWeighted(obrazNegatyw, wagaNegatywu, obrazRoboczy, 1.0 - wagaNegatywu, 0.0, obrazRoboczy);
    }

    // Wycinanie
    if (daneKształtu.isValid()) {
        int ksztaltIndeks = daneKształtu.toInt();

        cv::Mat obrazZAlfa;
        cv::cvtColor(obrazRoboczy, obrazZAlfa, cv::COLOR_BGR2BGRA);

        cv::Mat maska = cv::Mat::zeros(obrazRoboczy.size(), CV_8UC1);
        cv::Point srodek(maska.cols / 2, maska.rows / 2);
        int szerokosc = maska.cols * skalaWyciecia;
        int wysokosc = maska.rows * skalaWyciecia;

        if (ksztaltIndeks == 0) { // Owal
            cv::ellipse(maska, srodek, cv::Size(szerokosc/2, wysokosc/2), 0, 0, 360, cv::Scalar(255), cv::FILLED);
        } else if (ksztaltIndeks == 1) { // Prostokąt
            cv::Rect prostokat(srodek.x - szerokosc/2, srodek.y - wysokosc/2, szerokosc, wysokosc);
            cv::rectangle(maska, prostokat, cv::Scalar(255), cv::FILLED);
        } else if (ksztaltIndeks == 2) { // Romb
            std::vector<cv::Point> punktyRombu = {
                cv::Point(srodek.x, srodek.y - wysokosc / 2),
                cv::Point(srodek.x + szerokosc / 2, srodek.y),
                cv::Point(srodek.x, srodek.y + wysokosc / 2),
                cv::Point(srodek.x - szerokosc / 2, srodek.y)
            };
            std::vector<std::vector<cv::Point>> wielokat = { punktyRombu };
            cv::fillPoly(maska, wielokat, cv::Scalar(255));
        }

        std::vector<cv::Mat> kanaly;
        cv::split(obrazZAlfa, kanaly);
        kanaly[3] = maska;
        cv::merge(kanaly, obrazRoboczy);

        cv::cvtColor(obrazRoboczy, obrazRoboczy, cv::COLOR_BGRA2RGBA);
        QImage gotowyQt(obrazRoboczy.data, obrazRoboczy.cols, obrazRoboczy.rows, obrazRoboczy.step, QImage::Format_RGBA8888);
        wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyQt.copy()));

    } else {
        QImage gotowyQt = cvMatToQImage(obrazRoboczy);
        wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyQt));
    }
}



void MainWindow::filtrSzary(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(4, wartosc);
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::filtrJasnosc(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(2, wartosc);
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::filtrKontrast(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(3, wartosc);
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::filtrNegatyw(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(6, wartosc);
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::filtrSepia(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(5, wartosc);
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::zastosujWyciecie()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(7, ui->comboKsztalt->currentIndex());
    wybranyObiekt->setData(8, ui->sliderRozmiarWyciecia->value() / 100.0);
    ukryjPodgladWyciecia();
    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::resetujFiltry()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    // Czyszczenie pamięci zdjęcia
    wybranyObiekt->setData(2, 0);
    wybranyObiekt->setData(3, 0);
    wybranyObiekt->setData(4, 0);
    wybranyObiekt->setData(5, 0);
    wybranyObiekt->setData(6, 0);
    wybranyObiekt->setData(7, QVariant());
    wybranyObiekt->setData(8, QVariant());

    // Wizualny powrót suwaków interfejsu do zera
    ui->sliderJasnosc->blockSignals(true);
    ui->sliderKontrast->blockSignals(true);
    ui->sliderSzary->blockSignals(true);
    ui->sliderSepia->blockSignals(true);
    ui->sliderNegatyw->blockSignals(true);

    ui->sliderJasnosc->setValue(0);
    ui->sliderKontrast->setValue(0);
    ui->sliderSzary->setValue(0);
    ui->sliderSepia->setValue(0);
    ui->sliderNegatyw->setValue(0);

    ui->sliderJasnosc->blockSignals(false);
    ui->sliderKontrast->blockSignals(false);
    ui->sliderSzary->blockSignals(false);
    ui->sliderSepia->blockSignals(false);
    ui->sliderNegatyw->blockSignals(false);

    aktualizujWygladElementu(wybranyObiekt);
}

void MainWindow::aktualizujSuwaki()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    ui->sliderJasnosc->blockSignals(true);
    ui->sliderKontrast->blockSignals(true);
    ui->sliderSzary->blockSignals(true);
    ui->sliderSepia->blockSignals(true);
    ui->sliderNegatyw->blockSignals(true);

    ui->sliderJasnosc->setValue(wybranyObiekt->data(2).toInt());
    ui->sliderKontrast->setValue(wybranyObiekt->data(3).toInt());
    ui->sliderSzary->setValue(wybranyObiekt->data(4).toInt());
    ui->sliderSepia->setValue(wybranyObiekt->data(5).toInt());
    ui->sliderNegatyw->setValue(wybranyObiekt->data(6).toInt());

    ui->sliderJasnosc->blockSignals(false);
    ui->sliderKontrast->blockSignals(false);
    ui->sliderSzary->blockSignals(false);
    ui->sliderSepia->blockSignals(false);
    ui->sliderNegatyw->blockSignals(false);
}

void MainWindow::podgladWyciecia()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;
    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    ukryjPodgladWyciecia();

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    double szerokoscZdjecia = oryginalQt.width();
    double wysokoscZdjecia = oryginalQt.height();
    QPointF srodek(szerokoscZdjecia / 2.0, wysokoscZdjecia / 2.0);

    double skala = ui->sliderRozmiarWyciecia->value() / 100.0;
    QPen czerwonyObrys(Qt::red, 3, Qt::DashLine);
    int wybranyIndeks = ui->comboKsztalt->currentIndex();

    double szerokosc = szerokoscZdjecia * skala;
    double wysokosc = wysokoscZdjecia * skala;

    if (wybranyIndeks == 0) {
        double promienX = szerokosc / 2.0;
        double promienY = wysokosc / 2.0;
        QGraphicsEllipseItem *owal = new QGraphicsEllipseItem(srodek.x() - promienX, srodek.y() - promienY, promienX * 2, promienY * 2);
        owal->setPen(czerwonyObrys);
        owal->setParentItem(wybranyObiekt);
        tymczasowyObrys = owal;
    }
    else if (wybranyIndeks == 1) {
        QGraphicsRectItem *prostokat = new QGraphicsRectItem(srodek.x() - szerokosc / 2.0, srodek.y() - wysokosc / 2.0, szerokosc, wysokosc);
        prostokat->setPen(czerwonyObrys);
        prostokat->setParentItem(wybranyObiekt);
        tymczasowyObrys = prostokat;
    }
    else if (wybranyIndeks == 2) {
        QPolygonF punktyRombu;
        punktyRombu << QPointF(srodek.x(), srodek.y() - wysokosc / 2.0)
                    << QPointF(srodek.x() + szerokosc / 2.0, srodek.y())
                    << QPointF(srodek.x(), srodek.y() + wysokosc / 2.0)
                    << QPointF(srodek.x() - szerokosc / 2.0, srodek.y());
        QGraphicsPolygonItem *romb = new QGraphicsPolygonItem(punktyRombu);
        romb->setPen(czerwonyObrys);
        romb->setParentItem(wybranyObiekt);
        tymczasowyObrys = romb;
    }
}

void MainWindow::ukryjPodgladWyciecia()
{
    if (tymczasowyObrys != nullptr) {
        plotno->removeItem(tymczasowyObrys);
        delete tymczasowyObrys;
        tymczasowyObrys = nullptr;
    }
}

//naklejki
void MainWindow::dodajNaklejke(QListWidgetItem *naklejka)
{
    QIcon ikona = naklejka->icon();
    QPixmap obrazekNaklejki = ikona.pixmap(ikona.actualSize(QSize(200, 200)));

    QGraphicsPixmapItem *nowaNaklejka = new QGraphicsPixmapItem(obrazekNaklejki);

    nowaNaklejka->setFlag(QGraphicsItem::ItemIsMovable, true);
    nowaNaklejka->setFlag(QGraphicsItem::ItemIsSelectable, true);

    nowaNaklejka->setData(0, "Naklejka");
    plotno->addItem(nowaNaklejka);
}