#include "SpeedometerQt.h"
#include <QtWidgets/QApplication>
#include <QFile>

QString set_styling();

int main(int argc, char *argv[])
{

    

    QApplication app(argc, argv);
    RadialGauge window;
    QString style = set_styling();

    window.setStyleSheet(style);

    window.show();
    return app.exec();
}

QString set_styling()
{
    
    QFile style_file("./style.css");

    if (style_file.open(QFile::ReadOnly))
    {
        QString style_sheet = QLatin1String(style_file.readAll());
        style_file.close();
        return style_sheet;
    }

    qDebug() << "Could not open stylesheet file!";
    return QString();
}
