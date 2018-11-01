#include "excelengine.h"
#include "qt_windows.h"

ExcelEngine::ExcelEngine()
{
    pExcel     = NULL;
    pWorkbooks = NULL;
    pWorkbook  = NULL;
    pWorksheet = NULL;

    sXlsFile     = "";
    mUsedRagneRowCount    = 0;
    mUsedRagneColumnCount = 0;
    mUsedRagneStartRow    = 0;
    mUsedRagneStartColumn = 0;
    mWorkSheetCount = 0;
    mCurrentSheetId = 0;
    mMaxColumnCount = 0;
    mMaxRowCount = 0;

    bIsOpen     = false;
    bIsValid    = false;
    bIsANewFile = false;
    bIsSaveAlready = false;

    HRESULT r = OleInitialize(0);
    if (r != S_OK && r != S_FALSE)
    {
        qDebug("Qt: Could not initialize OLE (error %x)", (unsigned int)r);
    }
}


ExcelEngine::~ExcelEngine()
{
    if ( bIsOpen )
    {
        //析构前，先保存数据，然后关闭workbook
        Close();
    }
    OleUninitialize();
}

/**
  *@brief 打开sXlsFile指定的excel报表,
  *@return true : 打开成功
  *        false: 打开失败
  */
bool ExcelEngine::Open(unsigned int nSheet, bool visible, QString type)
{

    if ( bIsOpen )
    {
        //return bIsOpen;
        Close();
    }

    bIsVisible = visible;

    if ( NULL == pExcel )
    {
        pExcel = new QAxObject(type);
        if ( pExcel )
        {
            bIsValid = true;
        }
        else
        {
            bIsValid = false;
            bIsOpen  = false;
            return bIsOpen;
        }

        pExcel->dynamicCall("SetVisible(bool)", bIsVisible);
    }

    if ( !bIsValid )
    {
        bIsOpen  = false;
        return bIsOpen;
    }

    if ( sXlsFile.isEmpty() )
    {
        bIsOpen  = false;
        return bIsOpen;
    }

    /*如果指向的文件不存在，则需要新建一个*/
    QFile f(sXlsFile);
    if (!f.exists())
    {
        bIsANewFile = true;
    }
    else
    {
        bIsANewFile = false;
    }

    if (!bIsANewFile)
    {
        pWorkbooks = pExcel->querySubObject("WorkBooks"); //获取工作簿
        pWorkbook = pWorkbooks->querySubObject("Open(QString, QVariant)",sXlsFile,QVariant(0)); //打开xls对应的工作簿
    }
    else
    {
        pWorkbooks = pExcel->querySubObject("WorkBooks");     //获取工作簿
        pWorkbooks->dynamicCall("Add");                       //添加一个新的工作薄
        pWorkbook  = pExcel->querySubObject("ActiveWorkBook"); //新建一个xls
    }

    if( openWorkSheet( nSheet ) )
        bIsOpen = true;
    else
        bIsOpen  = false;

    return bIsOpen;
}

/**
  *@brief Open()的重载函数
  */
bool ExcelEngine::Open(QString xlsFile, unsigned int nSheet, bool visible , QString type)
{
    sXlsFile = xlsFile;
    bIsVisible = visible;
    return Open(nSheet,bIsVisible,type);
}




/**
  *@brief 保存表格数据，把数据写入文件
  */
void ExcelEngine::Save()
{
    if ( IsOpen() )
    {
        //should save every time called
        //if (bIsSaveAlready)
        //{
        //    return ;
        //}

        if (!bIsANewFile)
        {
            pWorkbook->dynamicCall("Save()");
        }
        else /*如果该文档是新建出来的，则使用另存为COM接口*/
        {
            //QAxWidget app =
            //pExcel->setProperty("AlertBeforeOverwriting" ,false);
            //pExcel->setProperty("DisplayAlerts",false);

            pWorkbook->dynamicCall("SaveAs(const QString&)", sXlsFile);
            bIsANewFile = false;

            //pWorkbook->dynamicCall("SaveAs (const QString&,int,const QString&,const QString&,bool,bool)",
                      //sXlsFile,56);

        }

        bIsSaveAlready = true;
    }
}

/**
  *@brief 关闭前先保存数据，然后关闭当前Excel COM对象，并释放内存
  */
void ExcelEngine::Close()
{
    //关闭前先保存数据
    Save();

    if ( IsValid() )
    {
        pWorkbook->dynamicCall("Close(bool)", true);
        pExcel->dynamicCall("Quit()");

        delete pExcel;
        pExcel = NULL;

        bIsOpen     = false;
        bIsValid    = false;
        bIsANewFile = false;
        bIsSaveAlready = true;
    }
}



QString ExcelEngine::getFilePath()
{
    if( IsOpen() ){
        return sXlsFile;
    }else{
        return "";
    }
}

bool ExcelEngine::switchFile(QString file)
{

    pWorkbooks = pExcel->querySubObject("WorkBooks"); //获取工作簿
    pWorkbook = pWorkbooks->querySubObject("Open(QString, QVariant)",sXlsFile,QVariant(0)); //打开xls对应的工作簿

    Save();
    pWorkbook->dynamicCall("Close(bool)", true);
    sXlsFile = file;
    pWorkbook = pWorkbooks->querySubObject("Open(QString, QVariant)",sXlsFile,QVariant(0));
    if( openWorkSheet( 1 ) )
        bIsOpen = true;
    else
        bIsOpen  = false;

    return bIsOpen;
}

/**
  *@brief 把tableWidget中的数据保存到excel中
  *@param tableWidget : 指向GUI中的tablewidget指针
  *@return 保存成功与否 true : 成功
  *                  false: 失败
  */
bool ExcelEngine::SaveDataFrTable(QTableWidget *tableWidget)
{
    if ( NULL == tableWidget )
    {
        return false;
    }
    if ( !bIsOpen )
    {
        return false;
    }

    int tableR = tableWidget->rowCount();
    int tableC = tableWidget->columnCount();

    //获取表头写做第一行
    for (int i=0; i<tableC; i++)
    {
        if ( tableWidget->horizontalHeaderItem(i) != NULL )
        {
            this->SetCellData(1,i+1,tableWidget->horizontalHeaderItem(i)->text());
        }
    }

    //写数据
    for (int i=0; i<tableR; i++)
    {
        for (int j=0; j<tableC; j++)
        {
            if ( tableWidget->item(i,j) != NULL )
            {
                this->SetCellData(i+2,j+1,tableWidget->item(i,j)->text());
            }
        }
    }

    //保存
    Save();

    return true;
}

/**
  *@brief 从指定的xls文件中把数据导入到tableWidget中
  *@param tableWidget : 执行要导入到的tablewidget指针
  *@return 导入成功与否 true : 成功
  *                   false: 失败
  */
bool ExcelEngine::ReadDataToTable(QTableWidget *tableWidget)
{
    if ( NULL == tableWidget )
    {
        return false;
    }

    //先把table的内容清空
    int tableColumn = tableWidget->columnCount();
    tableWidget->clear();
    for (int n=0; n<tableColumn; n++)
    {
        tableWidget->removeColumn(0);
    }

    int rowcnt    = mUsedRagneStartRow + mUsedRagneRowCount;
    int columncnt = mUsedRagneStartColumn + mUsedRagneColumnCount;

    //获取excel中的第一行数据作为表头
    QStringList headerList;
    for (int n = mUsedRagneStartColumn; n<columncnt; n++ )
    {
        QAxObject * cell = pWorksheet->querySubObject("Cells(int,int)",mUsedRagneStartRow, n);
        if ( cell )
        {
            headerList<<cell->dynamicCall("Value2()").toString();
        }
    }

    //重新创建表头
    tableWidget->setColumnCount(mUsedRagneColumnCount);
    tableWidget->setHorizontalHeaderLabels(headerList);


    //插入新数据
    for (int i = mUsedRagneStartRow+1, r = 0; i < rowcnt; i++, r++ )  //行
    {
        tableWidget->insertRow(r); //插入新行
        for (int j = mUsedRagneStartColumn, c = 0; j < columncnt; j++, c++ )  //列
        {
            QAxObject * cell = pWorksheet->querySubObject("Cells(int,int)", i, j );//获取单元格

            //在r新行中添加子项数据
            if ( cell )
            {
                tableWidget->setItem(r,c,new QTableWidgetItem(cell->dynamicCall("Value2()").toString()));
            }
        }
    }

    return true;
}

/**
  *@brief 获取指定单元格的数据
  *@param row : 单元格的行号
  *@param column : 单元格的列号
  *@return [row,column]单元格对应的数据
  */
QVariant ExcelEngine::GetCellData(unsigned int row, unsigned int column)
{
    QVariant data;

    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//获取单元格对象
    if ( cell )
    {
        data = cell->dynamicCall("Value2()");
    }

    return data;
}

/**
  *@brief 修改指定单元格的数据
  *@param row : 单元格的行号
  *@param column : 单元格指定的列号
  *@param data : 单元格要修改为的新数据
  *@return 修改是否成功 true : 成功
  *                   false: 失败
  */
bool ExcelEngine::SetCellData(unsigned int row, unsigned int column, QVariant data)
{
    bool op = false;

    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//获取单元格对象
    if ( cell )
    {
        QString strData = data.toString(); //excel 居然只能插入字符串和整型，浮点型无法插入
        cell->dynamicCall("SetValue(const QVariant&)",strData); //修改单元格的数据
        //cell->setProperty("HoriontalAligment", -4108);    // 左对齐（xlLeft）：-4131 ；居中（xlCenter）：-4108 ；右对齐（xlRight）：-4152
        cell->setProperty("ColumnWidth", 15);
        op = true;
    }
    else
    {
        op = false;
    }

    return op;
}


bool ExcelEngine::setCellBackgroundColor(unsigned int row, unsigned int column, QColor color)
{
    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//获取单元格对象
    if ( cell )
    {
        QAxObject* interior = cell->querySubObject("Interior");
        interior->setProperty("Color", color);   //设置单元格背景色
        return true;
    }

    return false;
}

bool ExcelEngine::setCellFontColor(unsigned int row, unsigned int column, QColor color)
{
    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//获取单元格对象
    if ( cell )
    {
        QAxObject *font = cell->querySubObject("Font");  //获取单元格字体
        font->setProperty("Color", color);  //设置单元格字体颜色（红色）
        /*
         * QAxObject* border = cell->querySubObject("Borders");
        border->setProperty("Color", QColor(0, 0, 255));   //设置单元格边框色（蓝色）
        */
        return true;
    }

    return false;
}

bool ExcelEngine::openWorkSheet( unsigned int id )
{

    if( !pWorkbook  )
        return false;

    QAxObject *worksheets = pWorkbook->querySubObject("WorkSheets");  //Sheets也可换用WorkSheets
    mWorkSheetCount = worksheets->property("Count").toInt();

    if( id > mWorkSheetCount )
        return false;

    pWorksheet = pWorkbook->querySubObject("Sheets(int)", id);
    if( NULL ==  pWorksheet ){
        bIsOpen = false;
        return false;
    }

    mCurrentSheetId = id;

    //至此已打开，开始获取相应属性
    //因为excel可以从任意行列填数据而不一定是从0,0开始，因此要获取首行列下标
    QAxObject *usedrange = pWorksheet->querySubObject("UsedRange");//获取该sheet的使用范围对象
    QAxObject *rows = usedrange->querySubObject("Rows");
    QAxObject *columns = usedrange->querySubObject("Columns");
    mUsedRagneStartRow    = usedrange->property("Row").toInt();    //第一行的起始位置
    mUsedRagneStartColumn = usedrange->property("Column").toInt(); //第一列的起始位置
    mUsedRagneRowCount    = rows->property("Count").toInt();       //获取行数
    mUsedRagneColumnCount = columns->property("Count").toInt();    //获取列数

    //get max row/column count
    rows = pWorksheet->querySubObject("Rows");
    columns = pWorksheet->querySubObject("Columns");
    mMaxRowCount    = rows->property("Count").toInt();       //获取行数
    mMaxColumnCount = columns->property("Count").toInt();    //获取列数

    return true;
}


//QChar(3 - 1 + 'A')); ?//初始列
bool ExcelEngine::mergeUnit(int row0,QChar col0, int row1, QChar col1)
{
    if( !IsOpen() ){
        return false;
    }
    QString merge_cell;
    merge_cell.append(col0);//初始列
    merge_cell.append(QString::number(row0));//初始行
    merge_cell.append(":");
    merge_cell.append(col1);//终止列
    merge_cell.append(QString::number(row1));//终止行
    QAxObject *merge_range = pWorksheet->querySubObject("Range(const QString&)", merge_cell);
    merge_range->setProperty("HorizontalAlignment", -4108);
    merge_range->setProperty("VerticalAlignment", -4108);
    merge_range->setProperty("WrapText", true);
    merge_range->setProperty("MergeCells", true); //合并单元格
    //merge_range->setProperty("MergeCells", false); ?//拆分单元格

    return true;
}


/**
  *@brief 判断excel是否已被打开
  *@return true : 已打开
  *        false: 未打开
  */
bool ExcelEngine::IsOpen()
{
    return bIsOpen;
}

/**
  *@brief 判断excel COM对象是否调用成功，excel是否可用
  *@return true : 可用
  *        false: 不可用
  */
bool ExcelEngine::IsValid()
{
    return bIsValid;
}

/**
  *@brief 获取excel的行数
  */
unsigned int ExcelEngine::GetUsedRagneRowCount()const
{
    return mUsedRagneRowCount;
}

unsigned int ExcelEngine::getUsedRagneStartRow()
{
    return mUsedRagneStartRow;
}

/**
  *@brief 获取excel的列数
  */
unsigned int ExcelEngine::GetUsedRagneColumnCount()const
{
    return mUsedRagneColumnCount;
}

unsigned int ExcelEngine::getUsedRagneStartColumn()
{
    return mUsedRagneStartColumn;
}


unsigned int ExcelEngine::getCurrentSheetId()
{
    return mCurrentSheetId;
}

unsigned int ExcelEngine::getSheetCount()
{
    return mWorkSheetCount;
}

unsigned int ExcelEngine::getMaxRowCount()
{
    return mMaxRowCount;
}


unsigned int ExcelEngine::getMaxColumnCount()
{
    return mMaxColumnCount;
}
