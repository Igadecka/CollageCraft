#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <opencv2/opencv.hpp>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *zdarzenie) override;

private slots:
    void dodajZdjecie();
    void usunZdjecie();
    void zapiszKolaz();
    void odswiezListeWarstw();
    void filtrSzary(int wartoscSzarosci);
    void filtrJasnosc(int wartoscJasnosci);
    void filtrKontrast(int wartoscKontrastu);
    void filtrNegatyw(int wartoscNegatyw);
    void filtrSepia(int wartoscSepia);
    void aktualizujSuwaki();
    void resetujFiltry();
    void podgladWyciecia(); // Reaguje na suwak i rysuje ramkę
    void zastosujWyciecie(); // Reaguje na przycisk i ucina piksele
    void ukryjPodgladWyciecia();
    void dodajNaklejke(QListWidgetItem *item);



private:
    Ui::MainWindow *ui;
    QGraphicsScene *plotno;

    cv::Mat QImageToCvMat(const QImage &inImage);
    QImage cvMatToQImage(const cv::Mat &inMat);

    QGraphicsItem *tymczasowyObrys = nullptr;

};
#endif // MAINWINDOW_H