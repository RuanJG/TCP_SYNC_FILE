#ifndef EXCELENGINE_H
#define EXCELENGINE_H

#include <QObject>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QAxBase>
#include <QAxObject>
#include <QAxWidget>

#include <QTableWidget>
#include <QTableView>
#include <QTableWidgetItem>
#include <QDebug>


// the execl app type
#define TYPE_EXECL_APP_MS   "Excel.Application"
#define TYPE_EXECL_APP_WPS  "ET.Application"

/**
  *@brief ����һ������Qt��дexcel��װ���࣬ͬʱ�����ڰ�excel�е�����
  *��ʾ�������ϣ����߰ѽ����ϵ�����д��excel�У�ͬGUI���н�������ϵ���£�
  *Qt tableWidget <--> ExcelEngine <--> xls file.
  *
  *@note ExcelEngine��ֻ������/д���ݣ����������������м���
  *@author yaoboyuan 254200341@qq.com
  *@date 2012-4-12
  */
class ExcelEngine : protected QObject
{
public:
    ExcelEngine();
    ~ExcelEngine();

private:
    bool Open(unsigned int nSheet, bool visible, QString type);
public:
    bool Open(QString xlsFile, unsigned int nSheet = 1, bool visible = false, QString type=TYPE_EXECL_APP_MS);
    void Save();                //����xls����
    void Close();               //�ر�xls����
    QString getFilePath();
    bool switchFile(QString file);

    bool SaveDataFrTable(QTableWidget *tableWidget); //�������ݵ�xls
    bool ReadDataToTable(QTableWidget *tableWidget); //��xls��ȡ���ݵ�ui

    QVariant GetCellData(unsigned int row, unsigned int column);                //��ȡָ����Ԫ����
    bool     SetCellData(unsigned int row, unsigned int column, QVariant data); //�޸�ָ����Ԫ����

    unsigned int GetUsedRagneRowCount()const;
    unsigned int GetUsedRagneColumnCount()const;

    bool IsOpen();
    bool IsValid();

    unsigned int getUsedRagneStartRow();
    unsigned int getUsedRagneStartColumn();
    unsigned int getCurrentSheetId();
    unsigned int getSheetCount();
    bool setCellBackgroundColor(unsigned int row, unsigned int column, QColor color);
    bool setCellFontColor(unsigned int row, unsigned int column, QColor color);
    bool openWorkSheet(unsigned int id);
    unsigned int getMaxRowCount();
    unsigned int getMaxColumnCount();


    bool mergeUnit(int row0, QChar col0, int row1, QChar col1);
private:
    QString   sXlsFile;     //xls�ļ�·��
    bool      bIsOpen;      //�Ƿ��Ѵ���
    bool      bIsValid;     //�Ƿ���Ч
    bool      bIsANewFile;  //�Ƿ���һ���½�xls�ļ����������ִ򿪵�excel���Ѵ����ļ������б����½���
    bool      bIsSaveAlready;//��ֹ�ظ�����

    QAxObject *pExcel;      //ָ������excelӦ�ó���
    QAxObject *pWorkbooks;  //ָ����������,excel�кܶ๤����
    QAxObject *pWorkbook;   //ָ��sXlsFile��Ӧ�Ĺ�����
    bool      bIsVisible;   //excel�Ƿ��ɼ�

    QAxObject *pWorksheet;  //ָ���������е�ĳ��sheet����
    unsigned int      mCurrentSheetId;   //��ǰ�򿪵ĵڼ���sheet
    unsigned int       mWorkSheetCount;
    unsigned int       mUsedRagneRowCount;    //����
    unsigned int       mUsedRagneColumnCount; //����
    unsigned int       mUsedRagneStartRow;    //��ʼ�����ݵ����±�ֵ
    unsigned int       mUsedRagneStartColumn; //��ʼ�����ݵ����±�ֵ
    unsigned int       mMaxRowCount;
    unsigned int       mMaxColumnCount;

};



/*
    ExcelEngine excel; //����excl����
    excel.Open(QObject::tr("c:\\Test.xls"),1,false); //����ָ����xls�ļ���ָ��sheet����ָ���Ƿ��ɼ�

    int num = 0;
    for (int i=1; i<=10; i++)
    {
        for (int j=1; j<=10; j++)
        {
           excel.SetCellData(i,j,++num); //�޸�ָ����Ԫ����
        }
    }

    QVarient data = excel.GetCellData(1,1); //����ָ����Ԫ������
    excel.GetCellData(2,2);
    excel.GetCellData(3,3);
    excel.Save(); //����
    excel.Close();
*/

//�������ݵ�tablewidget��
/*
    ExcelEngine excel(QObject::tr("c:\\Import.xls"));
    excel.Open();
    excel.ReadDataToTable(ui->tableWidget); //���뵽widget��
    excel.Close();
*/

//��tablewidget�е����ݵ�����excel��
/*
    ExcelEngine excel(QObject::tr("c:\\Export.xls"));
    excel.Open();
    excel.SaveDataFrTable(ui->tableWidget); //��������
    excel.Close();
*/



/*
 *
 *
 * ����                                    ֵ ����
xlAddIn                                 18 Microsoft Office Excel ������
xlAddIn8                                18 Excel 2007 ������
xlCSV                                    6 CSV
xlCSVMac                                22 Macintosh CSV
xlCSVMSDOS                              24 MSDOS CSV
xlCSVWindows                            23 Windows CSV
xlCurrentPlatformText                -4158 ��ǰƽ̨�ı�
xlDBF2                                   7 DBF2
xlDBF3                                   8 DBF3
xlDBF4                                  11 DBF4
xlDIF                                    9 DIF
xlExcel12                               50 Excel 12
xlExcel2                                16 Excel 2
xlExcel2FarEast                         27 Excel2 FarEast
xlExcel3                                29 Excel3
xlExcel4                                33 Excel4
xlExcel4Workbook                        35 Excel4 ������
xlExcel5                                39 Excel5
xlExcel7                                39 Excel7
xlExcel8                                56 Excel8
xlExcel9795                             43 Excel9795
xlHtml                                  44 HTML ��ʽ
xlIntlAddIn                             26 ���ʼ�����
xlIntlMacro                             25 ���ʺ�
xlOpenXMLAddIn                          55 ���� XML ������
xlOpenXMLTemplate                       54 ���� XML ģ��
xlOpenXMLTemplateMacroEnabled           53 �������õ� XML ģ����
xlOpenXMLWorkbook                       51 ���� XML ������
xlOpenXMLWorkbookMacroEnabled           52 �������õ� XML ��������
xlSYLK                                   2 SYLK
xlTemplate                              17 ģ��
xlTemplate8                             17 ģ�� 8
xlTextMac                               19 Macintosh �ı�
xlTextMSDOS                             21 MSDOS �ı�
xlTextPrinter                           36 ��ӡ���ı�
xlTextWindows                           20 Windows �ı�
xlUnicodeText                           42 Unicode �ı�
xlWebArchive                            45 Web ����
xlWJ2WD1                                14 WJ2WD1
xlWJ3                                   40 WJ3
xlWJ3FJ3                                41 WJ3FJ3
xlWK1                                    5 WK1
xlWK1ALL                                31 WK1ALL
xlWK1FMT                                30 WK1FMT
xlWK3                                   15 WK3
xlWK3FM3                                32 WK3FM3
xlWK4                                   38 WK4
xlWKS                                    4 ������
xlWorkbookDefault                       51 Ĭ�Ϲ�����
xlWorkbookNormal                     -4143 ���湤����
xlWorks2FarEast                         28 Works2 FarEast
xlWQ1                                   34 WQ1
xlXMLSpreadsheet                        46 XML ���ӱ���
 *
 * */


#endif // EXCELENGINE_H
