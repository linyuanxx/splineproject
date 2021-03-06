#include "linespline.h"
#include <cmath>
#include <cv.h>
#include <QDebug>
LineSpline::LineSpline()
{
    this->n = 3;
    this->p = 3;
    this->MinSrcN = 50;
    this->_distance = 10.1;
}

void LineSpline::setPoint(const QVector<QVector3D> &line)
{
    this->_line.clear();
    this->_kt.clear();
    this->_cpt.clear();
    this->_line = line;
}

void LineSpline::setKnot(const QVector<float> kt, int np)
{
    this->_kt.clear();
    this->_kt = kt;
    this->n = np;
}

void LineSpline::getControlPoints(QVector<QVector3D> &cpt)
{
    cpt = this->_cpt;
}

void LineSpline::getKnot(QVector<float> &kt)
{
    kt = this->_kt;
}

void LineSpline::calKnot()
{
    this->_kt.clear();
    this->du.clear();
    QVector3D dvt;
    float L = 0.f;
    for(unsigned int i=1; i<this->_line.size(); i++)
    {
        dvt = _line[i]-_line[i-1];
        L += dvt.length();
    }
    du.push_back(0.f);
    for(unsigned int i=1; i<this->_line.size()-1; i++)
    {
        dvt = _line[i]-_line[i-1];
        du.push_back(du[i-1]+dvt.length()/L);
    }
    du.push_back(1.f);

    //Tiller and Pigger 的节点矢量二次均衡方法
    // n+1 为控制点的个数；
    float *um = (float*)malloc(sizeof(float)*(n+p+2));
    for(int i=0; i<=p; i++)
    {
        um[i] = du[0];
        um[i+n+1] = du[du.size()-1];
    }
    float inc = 1.f*du.size()/(n+1);
    int low =0, high = 0;
    float d = -1;
    float *w = (float*)malloc(sizeof(float)*(n+1));
    for(int i=0; i<=n; i++)
    {
        d = d+inc;
        high = (int)floor(d+0.5);
        float sum = 0.f;
        for(int j=low; j<=high; j++)
            sum = sum+du[j];
        w[i] = sum/(high-low+1);
        low = high +1;
    }
    for(int i=1; i<= n-p; i++)
    {
        float sum = 0.f;
        for(int j=i; j<=i+p-1; j++)
            sum = sum + w[j];
        um[i+p] = sum/p;
    }
    for(int i=0; i<n+p+2; i++)
    {
        _kt.push_back(um[i]);
    }
    free(um);
    free(w);
}

void LineSpline::calNi(float um, QVector<float> &ni)
{
    ni.clear();
    float *U = new float[n+p+2];
    float *N = new float[n+1];
    for(int i=0; i<_kt.size(); i++)
        U[i] = _kt.at(i);
    AllBasisFuns(n, p, um, U, N);
    for(int i=0; i<n+1; i++)
        ni.push_back(N[i]);
    delete U;
    delete N;
}

void LineSpline::calApproxi()
{
    this->_cpt.clear();
    QVector<float> Ni;    //存储基底函数计算的值；
    int row = _line.size();
    CvMat *src1 = cvCreateMat(row, n+1, CV_32FC1);
    CvMat *src2 = cvCreateMat(row, 3, CV_32FC1);
    CvMat *dst = cvCreateMat(n+1, 3, CV_32FC1);
    for(int i=0; i<row; i++)
    {
        this->calNi(du[i], Ni);
        for(int j=0; j<n+1; j++)
            cvmSet(src1, i, j, Ni[j]);
        cvmSet(src2, i, 0, _line[i].x());
        cvmSet(src2, i, 1, _line[i].y());
        cvmSet(src2, i, 2, _line[i].z());
    }
    cvSolve(src1, src2, dst, CV_SVD);
    for(int i=0; i<n+1; i++)
    {
        float x = cvmGet(dst, i, 0);
        float y = cvmGet(dst, i, 1);
        float z = cvmGet(dst, i, 2);
        _cpt.push_back(QVector3D(x, y, z));
    }

    cvReleaseMat(&src1);
    cvReleaseMat(&src2);
    cvReleaseMat(&dst);
}

void LineSpline::toleranceFitting(float tolerance)
{
    if(_line.size()<this->MinSrcN)
        return;
    //qDebug()<<"line size: "<<_line.size();
    while(true)
    {
        QVector<float> Ni;
        this->calKnot();
        this->calApproxi();
         qDebug()<<"line size: "<<_line.size()<<_cpt.size();
        float minthr = 0.f;
        float x, y, z;
        int size = du.size();
        for(int i=0; i<size; i++)
        {
            x = y = z = 0.f;
            this->calNi(du[i], Ni);
            for(int j=0; j<n+1; j++)
            {
                float ni = Ni[j];
                x += _cpt[j].x()*ni;
                y += _cpt[j].y()*ni;
                z += _cpt[j].z()*ni;
            }
            QVector3D dvt = QVector3D(x-_line[i].x(),y-_line[i].y(),z-_line[i].z());
            float b = dvt.length();
            if(b > minthr)
                minthr = b;
        }
        qDebug()<<"minthr: "<<minthr;
        if(minthr > tolerance)
        {
            this->n = n+1;
            if(n >= _line.size())
                break;
        }else
            break;
    }
}

void LineSpline::numberFitting(int num)
{
    if(_line.size()<this->MinSrcN)
        return;
    this->n = num;
    this->calKnot();
    this->calApproxi();
}

void LineSpline::knotnumFitting(QVector<float> dn, int num)
{
    if(_line.size()<this->MinSrcN)
        return;
    this->n = num;
    this->_kt = dn;
    this->calApproxi();
}

void LineSpline::ordinaryCurves(QVector<QVector3D> &ap)
{
    ap.clear();
    //qDebug()<<this->_cpt.size()<<"cpt sizesssssssss";
    if(_cpt.size() < 4)
        return;
    float x,y,z;
    QVector<float> Ni;
    for(float s=0.0; s <= 1.0; s+= 0.01)
    {
        this->calNi(s, Ni);
        x = y = z = 0.f;
         qDebug()<<_cpt.size()<<Ni.size();
        for(int j=0; j<Ni.size(); j++)
        {
            float ni = Ni[j];
            x += _cpt[j].x()*ni;
            y += _cpt[j].y()*ni;
            z += _cpt[j].z()*ni;
        }
        ap.push_back(QVector3D(x,y,z));
    }
}

QVector3D LineSpline::pointOnCurves(float s)
{
    float x, y, z;
    x=y=z=0.f;
    QVector<float> Ni;
    this->calNi(s, Ni);
    for(int j=0; j<n+1; j++)
    {
        float ni = Ni[j];
        x += _cpt[j].x()*ni;
        y += _cpt[j].y()*ni;
        z += _cpt[j].z()*ni;
    }
    return QVector3D(x, y, z);
}

void LineSpline::bubblePointOnCurves(float s1, float s2)
{
    QVector3D p1, p2;
    p1 = this->pointOnCurves(s1);
    p2 = this->pointOnCurves(s2);
    QVector3D div = QVector3D(p1.x()-p2.x(), p1.y()-p2.y(), p1.z()-p2.z());
    float length = div.length();
    if(length < this->_distance)
        return;
    else
    {
        this->_ap.push_back(pointOnCurves((s1+s2)/2));
        bubblePointOnCurves(s1, (s1+s2)/2);
        bubblePointOnCurves((s1+s2)/2, s2);
    }
}

void LineSpline::bubbleCurves(QVector<QVector3D> &ap)
{
    ap.push_back(pointOnCurves(0.f));
    this->bubblePointOnCurves(0.f, 1.f);
    for(int i=0; i<_ap.size(); i++)
        ap.push_back(_ap[i]);
    ap.push_back(pointOnCurves(1.f));
    this->_ap.clear();
}
