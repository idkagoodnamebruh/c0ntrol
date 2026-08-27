#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <string_view>
#include "src/core/runtime/BuildMetadata.h"
#include "src/core/runtime/RuntimePaths.h"
#include "src/gui/MainWindow.h"
#include "src/platform/qt/QtSettingsStore.h"

int main(int argc, char* argv[]) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--version") {
            std::cout << BuildMetadata::versionText();
            return 0;
        }
        if (argument == "--help" || argument == "-h") {
            std::cout << BuildMetadata::helpText();
            return 0;
        }
    }

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("c0ntrol");
    QCoreApplication::setApplicationName("c0ntrol");

    auto settingsStore = std::make_unique<QtSettingsStore>();
    const SettingsLoadResult loaded = settingsStore->load();
    if (!loaded.warning.empty()) {
        qWarning() << "[SETTINGS]"
                   << QString::fromStdString(loaded.warning);
    }
    const ResolvedRuntimePaths runtimePaths = RuntimePaths::forCurrentPackage(
        QCoreApplication::applicationFilePath().toStdString());
    MainWindow window(loaded.config, std::move(settingsStore),
                      runtimePaths.model.string());
    window.show();

    return app.exec();
}
