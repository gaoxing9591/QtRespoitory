#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "qcustomplot.h"
#include "mytracer.h"
#include "framelesshelper.h"
#include "myxlsx.h"
#include "myCustomPlot.h"
#include <QSerialPort>        //提供访问串口的功能
#include <QSerialPortInfo>    //提供系统中存在的串口的信息

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    void    createChart(); //创建图表
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool saveData(QString *names);

void layoutConfig();


private slots:

    /*开始按钮*/
    void on_but_start_clicked();

    /*导入按钮*/
    void on_but_imp_clicked();

    /*保存按钮*/
    void on_but_sava_clicked();

    /*截图*/
    void on_but_screenshot_clicked();

    /*读取串口数据*/
    void serialPort_readyRead();


private:
    Ui::MainWindow *ui;
    bool button = false;
    QSerialPort serial;
    QTimer *timer;
    myCustomPlot *customPlot ;
     QVBoxLayout *central = NULL;

    int size;
    int timeId;
    int current_val = 0;
    int position_val = 0;
    int max = 15;
    int min =5 ;
    int x_pos = 0;

    QVector<float> m_flistX;//20个变量统一以时间为横坐标
    QVector<float> m_flistY[20];
    float m_drawingTime=0;
    QString m_varNames[20];
    QSerialPort *m_serial = nullptr;
    float m_tmpDrawingTime=0;
    int m_tmpEnd=0;

    int remain_data_flag = 0;
    //临时缓冲变量
    QByteArray buffer_tmp;
    int current_val_max = 0;
    int current_val_min = 15000;
    int failed_falg = 0;

    void setupPlot();

    QString lastFileDialogPath; //上次文件对话框路径



};
#endif // MAINWINDOW_H
