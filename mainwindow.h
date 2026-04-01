#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>

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

private:
    Ui::MainWindow *ui;
    QGraphicsScene *plotno;
};
#endif // MAINWINDOW_H