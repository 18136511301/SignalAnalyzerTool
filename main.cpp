#include "SignalAnalyzerTool.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SignalAnalyzerTool w;
    w.show();
    return a.exec();
}
