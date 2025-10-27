#include "SpeedometerQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{







    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    // Create query
    if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS) {
        qDebug() << "Failed to open query!";
        return -1;
    }

    // Add CPU counter
    if (PdhAddCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter) != ERROR_SUCCESS) {
        qDebug() << "Failed to add counter!";
        PdhCloseQuery(query);
        return -1;
    }

    qDebug() << "PDH initialized successfully!";

    PdhCloseQuery(query);








    QApplication app(argc, argv);
    RadialGauge window;
    window.show();
    return app.exec();
}
