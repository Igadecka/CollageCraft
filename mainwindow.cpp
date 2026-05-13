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
        QPixmap obrazPrzeskalowany = piksMapa.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation); //Wstępne skalowanie zdjęcia

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
        //ustawiamy to zdjęcie o 1 poziom wyżej niż reszta
        obiektGraficzny->setZValue(najwyzszePietro + 1.0);

        QFileInfo info(sciezkaPliku);
        obiektGraficzny->setData(0, info.fileName());
        obiektGraficzny->setData(1, QVariant::fromValue(obrazPrzeskalowany));


        //aktualizacja listy warstw w oknie aplikacji
        odswiezListeWarstw();

    }
}

//Usuwanie zaznaczonych zdjęć
void MainWindow::usunZdjecie()
{
    for (QGraphicsItem* wybranyObiekt : plotno->selectedItems()) {
        plotno->removeItem(wybranyObiekt); //usunięcie z płótna
        delete wybranyObiekt; //usuniięcie z pamięci
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


    //warstwy

    //przesuwanie o 1 pozycję wyżej (W)
    else if (klawisz->key() == Qt::Key_W) {
        QGraphicsItem *elementWyzej = nullptr;
        qreal najmniejszaRoznica = 999999;

        //szukanie na płótnie najbliższego zdjęcia wyżej
        for (QGraphicsItem *inny : plotno->items()) {
            if (inny != wybranyObiekt && inny->zValue() > wybranyObiekt->zValue()) {
                qreal roznica = inny->zValue() - wybranyObiekt->zValue();
                if (roznica < najmniejszaRoznica) {
                    najmniejszaRoznica = roznica;
                    elementWyzej = inny;
                }
            }
        }

        //jeśli inne zdjęcie jest nad tym wybranym to zamiana wartości z
        if (elementWyzej) {
            qreal mojeZ = wybranyObiekt->zValue();
            wybranyObiekt->setZValue(elementWyzej->zValue());
            elementWyzej->setZValue(mojeZ);
        }
    }

    //przesuwanie o 1 pozycję niżej (S)
    else if (klawisz->key() == Qt::Key_S) {
        QGraphicsItem *elementNizej = nullptr;
        qreal najmniejszaRoznica = 999999;

        //szukanie na płótnie najbliższego zdjęcia wyżej
        for (QGraphicsItem *inny : plotno->items()) {
            if (inny != wybranyObiekt && inny->zValue() < wybranyObiekt->zValue()) {
                qreal roznica = wybranyObiekt->zValue() - inny->zValue();
                if (roznica < najmniejszaRoznica) {
                    najmniejszaRoznica = roznica;
                    elementNizej = inny;
                }
            }
        }

        //jeśli inne zdjęcie jest pod tym wybranym to zamiana wartości z
        if (elementNizej) {
            qreal mojeZ = wybranyObiekt->zValue();
            wybranyObiekt->setZValue(elementNizej->zValue());
            elementNizej->setZValue(mojeZ);
        }
    }

    //na sam wierzch (E) albo na sam spód (Q)
    else if (klawisz->key() == Qt::Key_E || klawisz->key() == Qt::Key_Q) {
        qreal maksymalneZ = wybranyObiekt->zValue();
        qreal minimalneZ = wybranyObiekt->zValue();

        //szukanie samej góry i samego dołu na płótnie
        for (QGraphicsItem *element : plotno->items()) {
            if (element->zValue() > maksymalneZ) maksymalneZ = element->zValue();
            if (element->zValue() < minimalneZ) minimalneZ = element->zValue();
        }

        if (klawisz->key() == Qt::Key_E) {
            wybranyObiekt->setZValue(maksymalneZ + 1.0); //kładziemy na samym szczycie
        } else if (klawisz->key() == Qt::Key_Q) {
            wybranyObiekt->setZValue(minimalneZ - 1.0); //spychamy na samo dno
        }
    }

    odswiezListeWarstw();

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

void MainWindow::odswiezListeWarstw()
{
    ui->listaWarstw->clear(); //czyścimy stare napisy

    //pobieramy wszystkie elementy posortowane według wart. Z
    //DescendingOrder sprawi że element na samym wierzchu będzie pierwszy na liście
    const QList<QGraphicsItem*> elementy = plotno->items(Qt::DescendingOrder);

    for (QGraphicsItem *element : elementy) {
        QString nazwa = element->data(0).toString();

        if (nazwa.isEmpty()) nazwa = "Zdjęcie";

        //dodajemy nazwę do List Widgetu
        QListWidgetItem *wpis = new QListWidgetItem(nazwa);

        //jeśli element jest zaznaczony na płótnie, pogrubiamy go na liście
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

    //tworzenie macierzy OpenCV
    cv::Mat macierz(obrazRGB.height(), obrazRGB.width(),
                    CV_8UC3, //8bitowy, bez znaku, 3 kolory
                    (cv::Scalar*)obrazRGB.constBits(),
                    obrazRGB.bytesPerLine());

    //zamaiana kolejności kolorów z RGB (Qt) na BGR (OpenCV)
    cv::Mat macierzBGR;
    cv::cvtColor(macierz, macierzBGR, cv::COLOR_RGB2BGR);

    return macierzBGR;
}


//zmiana obrazu z OpenCV (cv::Mat) powrotem na Qt (QImage)
QImage MainWindow::cvMatToQImage(const cv::Mat &inMat)
{
    //obraz jest czarno-biały (1 kanał jasności)
    if (inMat.channels() == 1) {
        QImage obrazQt((const unsigned char*)(inMat.data),
                       inMat.cols, inMat.rows,
                       inMat.step,
                       QImage::Format_Grayscale8);
        return obrazQt.copy();
    }

    //obraz jest kolorowy (3 kanały BGR)
    else if (inMat.channels() == 3) {
        cv::Mat macierzRGB;
        //BGR na RGB
        cv::cvtColor(inMat, macierzRGB, cv::COLOR_BGR2RGB);

        QImage obrazQt((const unsigned char*)(macierzRGB.data),
                       macierzRGB.cols, macierzRGB.rows,
                       macierzRGB.step,
                       QImage::Format_RGB888);
        return obrazQt.copy();
    }

    return QImage();
}


void MainWindow::filtrSzary(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(4, wartosc);

    //wyciąganie orginalnego obrazu
    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    //zamiana do openCV
    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);

    //tworzenie kopii ale w szarości
    cv::Mat szaryJednokanalowy, szaryTrzykanalowy;
    cv::cvtColor(obrazOryginalny, szaryJednokanalowy, cv::COLOR_BGR2GRAY);
    cv::cvtColor(szaryJednokanalowy, szaryTrzykanalowy, cv::COLOR_GRAY2BGR); //tyle samo kanałow przed miksowaniem

    //obliczenie intensywności
    double szaroscWartosc = wartosc/100.0;
    double kolorWartosc = 1.0-szaroscWartosc;

    cv::Mat obrazWynikowy;
    //mikowanie dwóch obrazów zgodnie z proporcjami
    cv::addWeighted(szaryTrzykanalowy, szaroscWartosc, obrazOryginalny, kolorWartosc, 0.0, obrazWynikowy);
    QImage gotowyObrazQt = cvMatToQImage(obrazWynikowy);
    wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyObrazQt));

}


void MainWindow::filtrJasnosc(int wartoscJasnosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(2, wartoscJasnosc); // Zapisuje pozycję suwaka do pamięci podręcznej zdjęcia

    //pobiera czysty oryginał zdjecia
    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);
    cv::Mat obrazPoZmianie;

    obrazOryginalny.convertTo(obrazPoZmianie, -1, 1.0, wartoscJasnosc); //dodaje albo odejmje wartość

    QImage gotowyObrazQt = cvMatToQImage(obrazPoZmianie);
    wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyObrazQt));

}


void MainWindow::filtrKontrast(int wartoscKontrast)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(3, wartoscKontrast);

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);
    cv::Mat obrazPoZmianie;

    //Obliczamy współczynnik mnożenia (Alfa).
    double alfa = (wartoscKontrast + 100.0) / 100.0;
    //trzymamy szarość na poziomie wartości 128, aby obraz nie robił się po prostu czarny, gdy zmniejszamy kontrast
    double beta = 128.0 * (1.0 - alfa);

    obrazOryginalny.convertTo(obrazPoZmianie, -1, alfa, beta);

    QImage gotowyObrazQt = cvMatToQImage(obrazPoZmianie);
    wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyObrazQt));

}

void MainWindow::filtrNegatyw(int wartosc)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(6, wartosc);

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);
    cv::Mat obrazNegatyw;

    //odwracamy każdą wartość piksela
    cv::bitwise_not(obrazOryginalny, obrazNegatyw);

    //aalpha blending
    double alfa = wartosc / 100.0;
    double beta = 1.0 - alfa;

    cv::Mat obrazPoZmianie;

    //Zmiksowanie obu obrazów
    cv::addWeighted(obrazNegatyw, alfa, obrazOryginalny, beta, 0.0, obrazPoZmianie);

    QImage gotowyObrazQt = cvMatToQImage(obrazPoZmianie);
    wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyObrazQt));
}

void MainWindow::filtrSepia(int wartoscSepia)
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    wybranyObiekt->setData(5, wartoscSepia);

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);
    cv::Mat obrazSepia;

    //tabela z wzorami na stworzenie sepii (wzmacnia czerwony/zielony, osłabia niebieski)
    cv::Mat macierzSepii = (cv::Mat_<float>(3,3) <<
                                0.272, 0.534, 0.131,
                            0.349, 0.686, 0.168,
                            0.393, 0.769, 0.189);

    cv::transform(obrazOryginalny, obrazSepia, macierzSepii); //przepuszczenie oryginału przez tabelę

    //alpha blending
    double alfa = wartoscSepia / 100.0;
    double beta = 1.0 - alfa;

    cv::Mat obrazPoZmianie;

    cv::addWeighted(obrazSepia, alfa, obrazOryginalny, beta, 0.0, obrazPoZmianie);

    QImage gotowyObrazQt = cvMatToQImage(obrazPoZmianie);
    wybranyObiekt->setPixmap(QPixmap::fromImage(gotowyObrazQt));
}

void MainWindow::aktualizujSuwaki()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    //zamrożenie suwaków
    ui->sliderJasnosc->blockSignals(true);
    ui->sliderKontrast->blockSignals(true);
    ui->sliderSzary->blockSignals(true);
    ui->sliderSepia->blockSignals(true);
    ui->sliderNegatyw->blockSignals(true);

    //odczytanie i ustawienie wartości
    ui->sliderJasnosc->setValue(wybranyObiekt->data(2).toInt());
    ui->sliderKontrast->setValue(wybranyObiekt->data(3).toInt());
    ui->sliderSzary->setValue(wybranyObiekt->data(4).toInt());
    ui->sliderSepia->setValue(wybranyObiekt->data(5).toInt());
    ui->sliderNegatyw->setValue(wybranyObiekt->data(6).toInt());

    //odblokowanie suwaków
    ui->sliderJasnosc->blockSignals(false);
    ui->sliderKontrast->blockSignals(false);
    ui->sliderSzary->blockSignals(false);
    ui->sliderSepia->blockSignals(false);
    ui->sliderNegatyw->blockSignals(false);
}

void MainWindow::resetujFiltry()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;

    //wyciągnięcie czystego zdjecia i nałożenie go z powrotem na ekran
    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    wybranyObiekt->setPixmap(QPixmap::fromImage(oryginalQt));

    wybranyObiekt->setData(2, 0);
    wybranyObiekt->setData(3, 0);
    wybranyObiekt->setData(4, 0);
    wybranyObiekt->setData(5, 0);
    wybranyObiekt->setData(6, 0);

    //zablokowanie suwaków
    ui->sliderJasnosc->blockSignals(true);
    ui->sliderKontrast->blockSignals(true);
    ui->sliderSzary->blockSignals(true);
    ui->sliderSepia->blockSignals(true);
    ui->sliderNegatyw->blockSignals(true);

    //fizyczne przesunięcie suwaków z powrotem na miejsce w skali
    ui->sliderJasnosc->setValue(0);
    ui->sliderKontrast->setValue(0);
    ui->sliderSzary->setValue(0);
    ui->sliderSepia->setValue(0);
    ui->sliderNegatyw->setValue(0);

    //odblokowanie suwaków
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

    //Jeśli na ekranie jest stara czerwona ramka to usuwamy
    //Zapobiega nakładaniu się wielu ramek na siebie
    if (tymczasowyObrys != nullptr) {
        plotno->removeItem(tymczasowyObrys);
        delete tymczasowyObrys;
        tymczasowyObrys = nullptr;
    }

    //Pobieramy oryginalne zdjęcie żeby miec jego wymiary
    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    //obliczamy środek zdjęcia
    double szerokoscZdjecia = oryginalQt.width();
    double wysokoscZdjecia = oryginalQt.height();
    QPointF srodek(szerokoscZdjecia / 2.0, wysokoscZdjecia / 2.0);


    //odczytujemy suwak i zamieniamy na ułamek dziesiętny
    double skala = ui->sliderRozmiarWyciecia->value() / 100.0;

    //wygląd ramki podglądu
    QPen czerwonyObrys(Qt::red, 3, Qt::DashLine);

    int wybranyIndeks = ui->comboKsztalt->currentIndex();

    //obliczamy uniwersalne wymiary dla wszystkich kształtów
    double szerokosc = szerokoscZdjecia * skala;
    double wysokosc = wysokoscZdjecia * skala;

    if (wybranyIndeks == 0) { //Owal
        double promienX = szerokosc / 2.0;
        double promienY = wysokosc / 2.0;
        //rysujemy elipsę wychodzącą ze środka
        QGraphicsEllipseItem *owal = new QGraphicsEllipseItem(srodek.x() - promienX, srodek.y() - promienY, promienX * 2, promienY * 2);
        owal->setPen(czerwonyObrys);
        owal->setParentItem(wybranyObiekt); //przyklejamy ramkę do zdjęcia
        tymczasowyObrys = owal;
    }
    else if (wybranyIndeks == 1) { //Prostokąt
        QGraphicsRectItem *prostokat = new QGraphicsRectItem(srodek.x() - szerokosc / 2.0, srodek.y() - wysokosc / 2.0, szerokosc, wysokosc);
        prostokat->setPen(czerwonyObrys);
        prostokat->setParentItem(wybranyObiekt);
        tymczasowyObrys = prostokat;
    }

    else if (wybranyIndeks == 2) { //Romb
        QPolygonF punktyRombu;
        //Wierzchołki: góra, prawo, dół, lewo
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

void MainWindow::zastosujWyciecie()
{
    auto zaznaczone = plotno->selectedItems();
    if (zaznaczone.isEmpty()) return;

    QGraphicsPixmapItem *wybranyObiekt = dynamic_cast<QGraphicsPixmapItem*>(zaznaczone.first());
    if (!wybranyObiekt) return;


    //potwierdzono wycięcie więc usuwamy ramkę podglądowa
    if (tymczasowyObrys != nullptr) {
        plotno->removeItem(tymczasowyObrys);
        delete tymczasowyObrys;
        tymczasowyObrys = nullptr;
    }

    QImage oryginalQt = wybranyObiekt->data(1).value<QImage>();
    if (oryginalQt.isNull()) return;

    //BGR2BGRA dodaje czwarty kanał (Alpha) który odp. za przezroczystość
    cv::Mat obrazOryginalny = QImageToCvMat(oryginalQt);
    cv::Mat obrazZAlfa;
    cv::cvtColor(obrazOryginalny, obrazZAlfa, cv::COLOR_BGR2BGRA);

    //robimy czarną matrycę (maska)
    cv::Mat maska = cv::Mat::zeros(obrazOryginalny.size(), CV_8UC1);
    cv::Point srodek(maska.cols / 2, maska.rows / 2);

    double skala = ui->sliderRozmiarWyciecia->value() / 100.0;
    int wybranyIndeks = ui->comboKsztalt->currentIndex();

    int szerokosc = maska.cols * skala;
    int wysokosc = maska.rows * skala;

    //Rysujemy na masce na biało. Te piksele które będą widoczne
    if (wybranyIndeks == 0) { //Owal
        cv::ellipse(maska, srodek, cv::Size(szerokosc/2, wysokosc/2), 0, 0, 360, cv::Scalar(255), cv::FILLED);
    }
    else if (wybranyIndeks == 1) { //Prostokąt
        cv::Rect prostokat(srodek.x - szerokosc/2, srodek.y - wysokosc/2, szerokosc, wysokosc);
        cv::rectangle(maska, prostokat, cv::Scalar(255), cv::FILLED);
    }
    else if (wybranyIndeks == 2) { //Romb
        //Góra, Prawo, Dół, Lewo
        std::vector<cv::Point> punktyRombu = {
            cv::Point(srodek.x, srodek.y - wysokosc / 2),
            cv::Point(srodek.x + szerokosc / 2, srodek.y),
            cv::Point(srodek.x, srodek.y + wysokosc / 2),
            cv::Point(srodek.x - szerokosc / 2, srodek.y)
        };
        std::vector<std::vector<cv::Point>> wielokat = { punktyRombu };
        cv::fillPoly(maska, wielokat, cv::Scalar(255));
    }

    //rozszczepiamy obraz na 4 osobne kanaly (B, G, R, Alpha)
    std::vector<cv::Mat> kanaly;
    cv::split(obrazZAlfa, kanaly);
    kanaly[3] = maska; //Podmieniamy kanał Alpha na wyrysowaną maskę
    cv::Mat obrazWyciety;
    cv::merge(kanaly, obrazWyciety); //scalamy z powrotem w jeden obraz

    //BGRA na RGBA
    cv::cvtColor(obrazWyciety, obrazWyciety, cv::COLOR_BGRA2RGBA);
    //tłumaczymy dane na obiekt graficzny Qt
    QImage gotowyObrazQt(obrazWyciety.data, obrazWyciety.cols, obrazWyciety.rows, obrazWyciety.step, QImage::Format_RGBA8888);
    QImage bezpiecznaKopia = gotowyObrazQt.copy();
    //copy() odrywa zdjęcie od zmiennych OpenCV
    //bez tego zdjęcie zniknęłoby po zakończeniu funkcji

    wybranyObiekt->setPixmap(QPixmap::fromImage(bezpiecznaKopia));
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
void MainWindow::dodajNaklejke(QListWidgetItem *item)
{
    QIcon ikona = item->icon(); //pobranie ikony z panelu bocznego
    QPixmap obrazekNaklejki = ikona.pixmap(ikona.actualSize(QSize(200, 200))); //konwersja na Qpixmape

    QGraphicsPixmapItem *nowaNaklejka = new QGraphicsPixmapItem(obrazekNaklejki); //tworzy nowy obiekt na płótnie

    nowaNaklejka->setFlag(QGraphicsItem::ItemIsMovable, true); //poruszanie i zaznaczane
    nowaNaklejka->setFlag(QGraphicsItem::ItemIsSelectable, true);

    nowaNaklejka->setData(0, "Naklejka");
    plotno->addItem(nowaNaklejka); //dodanie na płótno
}