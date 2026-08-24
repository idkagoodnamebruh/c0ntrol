#include <QApplication>
#include <QDebug>
#include "src/gui/MainWindow.h"
#include "src/platform/qt/QtSettingsStore.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("c0ntrol");
    QCoreApplication::setApplicationName("c0ntrol");

    auto settingsStore = std::make_unique<QtSettingsStore>();
    const SettingsLoadResult loaded = settingsStore->load();
    if (!loaded.warning.empty()) {
        qWarning() << "[SETTINGS]"
                   << QString::fromStdString(loaded.warning);
    }
    MainWindow window(loaded.config, std::move(settingsStore));
    window.show();

    return app.exec();
}
