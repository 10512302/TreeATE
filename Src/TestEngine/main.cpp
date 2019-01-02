///
/// @brief         TestEngine main
/// @author        David Yin  2018-12 willage.yin@163.com
/// 
/// @license       GNU GPL v3
///
/// Distributed under the GNU GPL v3 License
/// (See accompanying file LICENSE or copy at
/// http://www.gnu.org/licenses/gpl.html)
///

#include "stdinc.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

#include "unitmgr.h"
#include "testrunner.h"
#include "resultmgr.h"
#include "testctrl.h"

// Let's debug informaion output to the log file
void customMessageHandler(QtMsgType type, const QMessageLogContext & logContext, const QString& str)
{
    QString strLogDir = qApp->applicationDirPath() + "\\Log\\TestEngine";
    QDir dir(strLogDir);
    if(!dir.exists())
    {
        dir.mkpath(strLogDir);
    }

    QDateTime currDate = QDateTime::currentDateTime();
    QString fName = currDate.toString("yyyy-MM-dd");
    fName = QString("%1\\%2.txt").arg(strLogDir).arg(fName);

    QFile outFile(fName);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);

    QString strType;
    switch(type)
    {
    case QtDebugMsg:
        strType = "Debug";
        break;
    case QtWarningMsg:
        strType = "Warning";
        break;
    case QtCriticalMsg:
        strType = "Critical";
        break;
    case QtFatalMsg:
        strType = "Fatal";
        break;
    case QtInfoMsg:
        strType = "Info";
        break;
    }

    ts << "[" << currDate.toString(TREEATE_DATETIME_FORMAT)
       << "] " << strType << ": "
       << logContext.file << " " << " - " << logContext.line << ": "
       << str << "\r\n";
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageHandler);
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("TestEngine");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(TA_TR("Copyright 2019 David Yin.\r\nTreeATE TestEngine. It's based-command-line test executer"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption startOption(
            QStringList() << "t" << "start-test",
            TA_TR("Start the test <item> to test."),
            TA_TR("item"));
    QCommandLineOption listOption(
            QStringList() << "l" << "list-item",
            TA_TR("List the test items information."));
    QCommandLineOption startMultiOption(
            QStringList() << "m" << "multi-items",
            TA_TR("Start multi-items in the <file> to test.\r\ncontent e.g.:\r\n/ProjectName/TestSuiteName/TestCase1\r\n/ProjectName/TestSuiteName/TestCase2\r\n..."),
            TA_TR("file"));
    QCommandLineOption parameterOption(
            QStringList() << "p" << "parameters",
            TA_TR("Specify the public parameters <file> for current test project."),
            TA_TR("file"));
    QCommandLineOption barcodeOpt(
            QStringList() << "b" << "barcode",
            TA_TR("Enter the <barcode> of UUT for test."),
            TA_TR("barcode"));
    QCommandLineOption userOpt(
            QStringList() << "u" << "user",
            TA_TR("Enter the <user> for test."),
            TA_TR("user"));
    QCommandLineOption stationOpt(
            QStringList() << "s" << "station",
            TA_TR("The <station> is unique in a manufactory."),
            "station");
    QCommandLineOption workLineOpt(
            QStringList() << "w" << "workline",
            TA_TR("The <workline> is unique in a manufactory."),
            "workline");
    QCommandLineOption fstopOpt(
            QStringList() << "S" << "Stop",
            TA_TR("Stop the current testing when it's failed"));
    parser.addOption(startOption);
    parser.addOption(startMultiOption);
    parser.addOption(parameterOption);
    parser.addOption(listOption);
    parser.addOption(barcodeOpt);
    parser.addOption(userOpt);
    parser.addOption(stationOpt);
    parser.addOption(workLineOpt);
    parser.addOption(fstopOpt);
    parser.addPositionalArgument("project", "Enter the project file name to test.");
    parser.process(a);

    if(argc <= 1) {
        parser.showHelp();
        return TA_ERR_NEED_PARA;
    }

    UnitMgr utMgr;
    const QStringList argLst = parser.positionalArguments();
    if(argLst.count() <= 0) {
        cerr << "Please enter the project file name or --help." << endl;
        return TA_ERR_NO_PROJECT;
    }
    else {
        QString prjFile = argLst.at(0);

        // loading project file and parser it.
        if(!utMgr.LoadUnitConfig(prjFile)) {
            cerr << utMgr.getLastError().toStdString() << endl;
            return TA_ERR_LOAD_UNITS;
        }
    }

    if(parser.isSet(parameterOption)) {
        QString strItem = parser.value(parameterOption);
        if(!utMgr.LoadPublicPara(strItem)) {
            cerr << utMgr.getLastError().toStdString() << endl;
            return TA_ERR_LOAD_PARA;
        }
    }

    bool bStopWhenFailed = false;
    if(parser.isSet(fstopOpt)) {
        bStopWhenFailed = true;
    }

    TestRunner run(&utMgr);
    if(!run.initScript(utMgr.getPrjPath()))
    {
        cerr << run.getLastError().toStdString() << endl;
        return TA_ERR_INIT_RUNNER;
    }

    ResultMgr rstMgr;
    if(!rstMgr.InitResult(parser.value(userOpt),
                          parser.value(stationOpt),
                          parser.value(workLineOpt),
                          parser.value(barcodeOpt))) {
        cerr << rstMgr.getLastError().toStdString() << endl;
        return TA_ERR_INIT_RESULT;
    }

    QStringList selPath;
    if(parser.isSet(startOption))
    {
        QString strItem = parser.value(startOption);
        selPath = utMgr.selectedUnitForPath(strItem);
    }
    else if(parser.isSet(startMultiOption))
    {
        QString strItem = parser.value(startMultiOption);        
        selPath = utMgr.selectedUnit(strItem);
    }
    else if(parser.isSet(listOption))
    {
        utMgr.printUnitToStd();
        // Upload the test result of history to server when list the test items.
        (void)rstMgr.UploadResultToSvr();
        return TA_LIST_OK;
    }


    if(selPath.isEmpty()) {
        cerr << utMgr.getLastError().toStdString() << endl;
        return TA_ERR_UNSELECTED;
    }

    TestCtrl testCtrl(&run);
    testCtrl.start();

    if(!run.runner(selPath, rstMgr, bStopWhenFailed))
    {
        rstMgr.ExitResult();
        cerr << run.getLastError().toStdString() << endl;
        return TA_ERR_RUNNING;
    }

    rstMgr.ExitResult();
    return TA_OK;
}