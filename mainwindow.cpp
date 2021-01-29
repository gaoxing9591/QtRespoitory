#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QTime"
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <stdlib.h>
#include <stdio.h>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    //开始按钮初始化为绿色
    ui->but_start->setStyleSheet("background: rgb(0,255,0)");
    customPlot = new myCustomPlot(this);

    //总布局
    setFixedSize(672,435);
    this->setAutoFillBackground(true);
    QPalette newPalette2(this->palette());
    newPalette2.setColor(QPalette::Background, QColor(Qt::black));
    this->setPalette(newPalette2);

    //widget设置布局
    ui->widget_2->setAutoFillBackground(true);
    QPalette newPalette(ui->widget_2->palette());
    newPalette.setColor(QPalette::Background, QColor(135,206,235));
    ui->widget_2->setPalette(newPalette);

    //widget2设置布局
    ui->widget->setAutoFillBackground(true);
    QPalette newPalette1(ui->widget->palette());
    newPalette1.setColor(QPalette::Background, QColor(135,206,235));
    ui->widget->setPalette(newPalette1);


    //通过QSerialPortInfo查找可用串口
    ui->com_port->clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->com_port->addItem(info.portName());
        qDebug() << "portName:" << info.portName();
        qDebug() << "isBusy:" << info.isBusy();
        qDebug() << "isValid:" << info.isValid();
    }

    //连接信号和槽
    QObject::connect(&serial, &QSerialPort::readyRead, this, &MainWindow::serialPort_readyRead);

    x_pos = 0;

    //调用绘图
    setupPlot();



}
/*===================绘图器====================*/
void MainWindow::setupPlot()
{

    //设置绘图器坐标位置
    customPlot->move(5,125);
    //设置绘图器固定大小
    customPlot->resize(663,280);
    //设置是否显示鼠标追踪器
    customPlot->showTracer(true);

    //添加标题布局元素：
    customPlot->plotLayout()->insertRow(0);
    customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, "位移时间关系曲线图", QFont("黑体", 12, QFont::Bold)));

    //customPlot->setBackground(Qt::gray);
    //设定右上角图形标注可见
    customPlot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(9);
    //设定右上角图形标注的字体
    customPlot->legend->setFont(legendFont);
    customPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    customPlot->legend->setSelectedTextColor(Qt::blue);
    customPlot->legend->setSelectableParts(QCPLegend::spItems);
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);

    //让左边和下边轴与上边和右边同步改变范围
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    //允许用户使用鼠标拖动轴范围，使用鼠标滚轮缩放并通过单击选择图形：
    customPlot->setInteractions(QCP::iRangeDrag |
                                QCP::iRangeZoom |
                                QCP::iSelectPlottables |
                                QCP::iSelectAxes |
                                QCP::iSelectLegend);


    //设置x轴标签名称
    customPlot->xAxis->setLabel("时间(time)");
    //设置y轴标签名称
    customPlot->yAxis->setLabel("压力(bar)");
    //显示刻度标签
    customPlot->xAxis->setTickLabels(true);
    //显示刻度格数
    customPlot->xAxis->ticker()->setTickCount(5);
    //显示x轴子网格线
    customPlot->xAxis->grid()->setSubGridVisible(true);
    //显示要轴子网格线
    customPlot->yAxis->grid()->setSubGridVisible(true);
    //可读性优于设置
    customPlot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssReadability);

    //设置x轴范围
    customPlot->xAxis->setRange(0, 3000);
    //设置y轴范围
    customPlot->yAxis->setRange(0, 2100);

    QPen pen;
    int i = 1;
    pen.setWidth(2);
    pen.setColor(QColor(qSin(i * 1 + 1.2) * 80 + 80, qSin(i*0.3 + 0) * 80 + 80, qSin(i*0.3 + 1.5) * 80 + 80));
    customPlot->addGraph();
    customPlot->graph(0)->setPen(pen);
    customPlot->graph(0)->setName(QStringLiteral("压力"));
}
void MainWindow::layoutConfig()
{

    //设置顶级布局
    central = new QVBoxLayout();
    central->addWidget(ui->widget);
    central->addWidget(ui->widget_2);
    ui->centralwidget->setLayout(central);

}
/*==============开始按钮=================*/
void MainWindow::on_but_start_clicked()
{
    if(!button){
        ui->but_start->setText("开始");
        serial.setPortName(ui->com_port->currentText());
        serial.setBaudRate(ui->com_baud->currentText().toUInt());
        serial.setFlowControl(QSerialPort::NoFlowControl);
        if(!serial.open(QIODevice::ReadWrite)){
            QMessageBox::about(NULL,"提示","无法打开串口!");
            return;
        }

        x_pos = 0;
        remain_data_flag = 0;
        QByteArray buffer_tmp;
        current_val_max = 0;
        current_val_min = 15000;
        failed_falg = 0;

        //开启时端口号,波特率下拉不可操作
        ui->com_port->setEnabled(false);
        ui->com_baud->setEnabled(false);
        ui->but_start->setText("停止");
        ui->but_start->setStyleSheet("background: rgb(255,0,0)");

        //端口运行时,显示运行正常
        ui->line_state->setText("正常");
        //运行正常时,字体颜色为绿色
        ui->line_state->setStyleSheet("color:green");
        button = true;
    }
    else {
        ui->but_start->setText("停止");
        //运行停止按钮颜色为红色
        ui->but_start->setStyleSheet("background: rgb(0,255,0)");
        serial.close();

        //关闭时,端口号,波特率下拉可操作
        ui->com_port->setEnabled(true);
        ui->com_baud->setEnabled(true);
        ui->but_start->setText("开始");
        //端口关闭行时,显示运行停止,字体颜色为红色
        ui->line_state->setText("停止");
        ui->line_state->setStyleSheet("color:red");
        button = false;

    }
}
/*==============打开文件按钮=================*/
void MainWindow::on_but_imp_clicked()
{
    QString filePath=QFileDialog::getOpenFileName(this,"open",
                                                  QCoreApplication::applicationDirPath(),"XLSX File(*.xlsx);;CSV File(*.csv);;TXT File(*.txt);;All File(*.*)");
    if(!filePath.isEmpty())
    {
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly))
        {
            file.readLine();//第一行 变量名 这里舍弃,也可以读入后显示，这里不做实现

            QString tmp;
            float X=0;
            for (; !(tmp=file.readLine()).isEmpty();)
            {
                QStringList dataStr = tmp.split(',');

                X = dataStr[0].toFloat();
                int paramCount = dataStr.size()-2;//去掉开头的时间变量/////////和多余的‘,’产生的多余数据  参见数据保存函数//

                m_flistX.append(X);//所有变量的时间X索引

                for (int j=0;j<paramCount;j++)
                {

                    m_flistY[j].append(dataStr[j+1].toFloat());
                }
            }
            m_tmpEnd=static_cast<int>(m_tmpDrawingTime =m_drawingTime=X);
        }

    }
}


/*==============保存文件按钮=================*/
void MainWindow::on_but_sava_clicked()
{
    //    if(this->saveData(m_varNames))
    //    {
    //        QMessageBox::information(NULL,"提示","保存成功");
    //    }
    //    else{
    //        QMessageBox::information(NULL,"提示","保存失败 ");
    //    }
    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("保存绘图数据-选择文件路径"),
                                                    lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")+".xlsx",
                                                    "XLSX File(*.xlsx);;CSV File(*.csv);;TXT File(*.txt);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".xlsx") &&
            !savePath.endsWith(".csv") &&
            !savePath.endsWith(".txt")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),tr("尚未支持的文件格式。请选择xlsx或者csv或者txt格式文件。"));
        return;
    }

    //记录路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

    bool ok = false;
    if(savePath.endsWith(".xlsx")){
        if(MyXlsx::write(customPlot, savePath))
            ok = true;
    }else if(savePath.endsWith(".csv")){
        if(customPlot->saveGraphAsTxt(savePath,','))
            ok = true;
    }else if(savePath.endsWith(".txt")){
        if(customPlot->saveGraphAsTxt(savePath,' '))
            ok = true;
    }

    //    if(ok){
    //        QString str = "\nSave successful in " + savePath + "\n";
    //        BrowserBuff.append(str);
    //        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
    //        printToTextBrowser();
    //    }else{
    //        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    //    }

}

//bool MainWindow::saveData(QString *names)
//{

//    QString savePath = QFileDialog::getSaveFileName(this,
//                                                    tr("保存数据"),
//                                                    "/PINYI-"+QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")+".xlsx",
//                                                    "XLSX File(*.xlsx);;CSV File(*.csv);;TXT File(*.txt);;All File(*.*)");
//    QFile file(savePath);
//    if(file.open(QIODevice::WriteOnly))
//    {
//        QString content;

//        //**变量名**
//        content.append("Time"); content.append(',');
//        for(int i=0;i<20;i++)
//        {
//            content.append(names[i]);
//            content.append(',');
//        }
//        content.append('\n');

//        //**数据**
//        int length=static_cast<int>(m_drawingTime);
//        int index;
//        for(index=0;index<20;index++)
//            if(m_flistY[index].size()==0) break;
//        for (int i = 0; i < length; i++)
//        {
//            content.append(QString::number(static_cast<double>(m_flistX[i])));
//            content.append(',');
//            for(int j=0;j<index;j++)
//            {
//                content.append(QString::number(static_cast<double>( m_flistY[j].at(i))));
//                content.append(',');
//            }
//            //先去掉多余的逗号
//            //content.remove(content.length()-1);//这里没有去掉多余的逗号，因为去掉之后会出现不可预知的数据保存错误
//            content.append('\n');

//            file.write(content.toLocal8Bit());
//            content.clear();
//        }
//        file.close();
//        return true;
//    }
//    else
//    {
//        return false;
//    }

//}


/*==============截图文件=================*/
void MainWindow::on_but_screenshot_clicked()
{
    QString folder = QCoreApplication::applicationDirPath() + "/SavedImage";
    QString fileName = "/PINYI" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz") + ".png";
    QDir dir(folder);

    if (!dir.exists())
    {
        dir.mkdir(folder);
    }


    QPixmap p =  this->grab(QRect(0,0,width(),height()));


    if(p.save(folder + fileName,"png"))
    {
        QMessageBox::information(NULL,"提示","保存成功");
    }
    else
    {
        QMessageBox::information(NULL,"提示","保存失败");
    }

}

/*============读取串口数据==============*/
void MainWindow::serialPort_readyRead()
{
    bool ok;
    QByteArray buffer_tmp2;
    int size_tmp = 0;

    //从接收缓冲区中读取数据
    QByteArray buffer = serial.readAll();

    if (remain_data_flag == 0)
    {
        buffer_tmp = buffer;

    }
    else
    {
        buffer_tmp += buffer;

    }

    //如果从缓冲区接收的字节长度==13,正常解析数据
    if (buffer_tmp.size() == 13)
    {
        qDebug() << buffer_tmp;
        remain_data_flag = 0;

        QStringList list =QString(buffer_tmp).split(" ",QString::SkipEmptyParts);
        current_val = ((list.at(0).toUInt(&ok,16)<<8) | (list.at(1).toUInt(&ok,16)<<0));
        position_val = ((list.at(2).toUInt(&ok,16)<<8) | (list.at(3).toUInt(&ok,16)<<0));
        customPlot->graph(0)->addData(x_pos,position_val);
        customPlot->replot();
        x_pos+=10;

    }
    //如果接收的字节数小于13,记录在remain_data_flag
    else if (buffer_tmp.size() < 13)
    {
        remain_data_flag = 1;

    }
    //如果接收的字节数大于13
    else if (buffer_tmp.size() > 13)
    {
        //buffer_tmp "1F FE 04 C2 \r\n1F FE 04 C2 \r\n1F FE 04 C2\r\n" size = 41
        while (buffer_tmp.size() > 13)
        {

            buffer_tmp2 = buffer_tmp.left(13);
            //buffer_tmp2 = "1F FE 04 C2 \r"
            qDebug() << buffer_tmp2;
            //buffer_tmp = buffer_tmp.right(13);
            //size_tmp 41-13 = 28 //继续循环
            size_tmp = buffer_tmp.size()-13;
            buffer_tmp = buffer_tmp.right(size_tmp);
            QStringList list =QString(buffer_tmp2).split(" ",QString::SkipEmptyParts);
            current_val = ((list.at(0).toUInt(&ok,16)<<8) | (list.at(1).toUInt(&ok,16)<<0));
            position_val = ((list.at(2).toUInt(&ok,16)<<8) | (list.at(3).toUInt(&ok,16)<<0));
            customPlot->graph(0)->addData(x_pos,position_val);
            customPlot->replot();
            x_pos+=10;
        }
        remain_data_flag = 1;//记录一次
    }


    if (current_val_max < current_val)
    {
        current_val_max = current_val;
        ui->line_max->setText(QString::number(current_val/1000.0)+"A");
    }

    if (current_val_min > current_val)
    {
        current_val_min = current_val;
        ui->line_min->setText(QString::number(current_val/1000.0)+"A");
    }

    if ((failed_falg == 0) &&
            ((current_val_max > 15000) ||(current_val_min < 5000)))
    {
        failed_falg = 1;
        ui->line_pass->setStyleSheet("color:red");
        ui->line_pass->setText("FALI");
    }

    qDebug() << "x_pos:"<< x_pos;

    if (x_pos >= 2990)
    {
        if (failed_falg == 0)
        {
            ui->line_pass->setStyleSheet("color:green");
            ui->line_pass->setText("PASS");
        }

    }

}


MainWindow::~MainWindow()
{
    delete ui;
}

