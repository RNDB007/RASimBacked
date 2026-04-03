#include <QCoreApplication>
#include "CommunicationLayer.h"
#include "RASimServerManager.h"
#include "SimulationController.h"
#include <QDir>

void CleanTempDirs()
{
    QString tempPath = QCoreApplication::applicationDirPath() + "/TempSim";
    QDir dir(tempPath);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    dir.mkpath(".");
    qInfo() << "Temp directory ready:" << tempPath;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    CleanTempDirs();

    CommunicationLayer commLayer;
    quint16 port = 8888;
    if (!commLayer.startListening(port)) {
        qCritical() << "Failed to start listening on port" << port;
        return 1;
    }
    qInfo() << "RASimBackend listening on port" << port;

    RASimServerManager* simManager = RASimServerManager::getInstance();
    SimulationController controller(&commLayer, simManager);

    return app.exec();
}
