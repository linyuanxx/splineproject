#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    glf = new GLForm;
    this->setCentralWidget(glf);

    //读入文件；
    connect(this->ui->readaction, SIGNAL(triggered()), this, SLOT(fileRead()));
    //清空显示数据；
    connect(this->ui->actionClear, SIGNAL(triggered()), this->glf, SLOT(clearData()));
    //显示原始点群数据；
    connect(this->ui->actionSource, SIGNAL(triggered()), this, SLOT(showSource()));
    //显示等高线；
    connect(this->ui->actioncontour, SIGNAL(triggered()), this, SLOT(showContour()));

    //bspline line
    connect(this->ui->actionOrdinary, SIGNAL(triggered()), this, SLOT(ordinaryLine()));
    //bspline bubble line
    connect(this->ui->actionBubble, SIGNAL(triggered()), this, SLOT(bubbleLine()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fileRead()
{
    QString filename = QFileDialog::getOpenFileName(this, "Read Model", "/home/linyuan/modelfiles");
    this->loadfile.setFileName(filename);
    this->loadfile.getPoints(pt);
    this->loadfile.getContours(contour, LoadFile::AU);
    this->glf->setPoint(pt);
}

void MainWindow::showSource()
{
    this->glf->setPoint(pt);
}

void MainWindow::showContour()
{
    this->glf->setContour(contour);
}

void MainWindow::ordinaryLine()
{
    QLinkedList<QVector3D> ap;
//    qDebug()<<"contour.size:"<<contour.size();
//    for(int i=0; i<contour.size(); i++)
//    {
        QVector<QVector3D> mp;
       // qDebug()<<"contour size: "<<i<<contour[i].size();
        linebs.setPoint(pt);
        linebs.toleranceFitting(15.0);
        qDebug()<<"contour size: "<<contour.size();
        linebs.ordinaryCurves(mp);
        qDebug()<<"contour size: "<<mp.size();
        for(int k=0; k<mp.size(); k++)
            ap.push_back(mp[k]);
    //}
     this->glf->setPoint(ap);
}

void MainWindow::bubbleLine()
{
    QLinkedList<QVector3D> ap;

        QVector<QVector3D> mp;

        linebs.setPoint(pt);
        linebs.toleranceFitting(15.0);

        linebs.bubbleCurves(mp);

        for(int k=0; k<mp.size(); k++)
            ap.push_back(mp[k]);

     this->glf->setPoint(ap);
}

void MainWindow::ordinarySurface()
{

}

void MainWindow::bubbleSurface()
{

}
