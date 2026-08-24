#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QVBoxLayout>
#include <memory>
#include "src/core/actions/ActionDispatcher.h"
#include "src/core/config/ISettingsStore.h"
#include "src/core/config/RuntimeConfigController.h"
#include "src/core/vision/VisionWorker.h"
#include "src/core/gestures/GesturePipeline.h"
#include "src/platform/SystemInputBackendFactory.h"
#include "src/gui/DeveloperModeWidget.h"

class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(RuntimeConfig config,
                        std::unique_ptr<ISettingsStore> settingsStore,
                        QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFrameProcessed(const QImage& frame, const Landmarks& landmarks);
    void onFilteredTrackingFrameProcessed(const HandTrackingFrame& trackingFrame);
    void onPipelineMetricsUpdated(const PipelineMetrics& metrics);
    void openSettingsDialog();

private:
    void setupWorker();
    const GestureObservation* selectPreferredObservation(
        const GesturePipelineResult& result) const;
    bool applyRuntimeConfig(const RuntimeConfig& requested, bool reset);

    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_videoLabel;
    DeveloperModeWidget* m_devWidget;

    QThread* m_thread;
    VisionWorker* m_worker;
    RuntimeConfig m_runtimeConfig;
    GesturePipeline m_gesturePipeline;
    std::unique_ptr<ISystemInputBackend> m_inputBackend;
    std::unique_ptr<ActionDispatcher> m_actionDispatcher;
    std::unique_ptr<RuntimeConfigController> m_configController;
    std::unique_ptr<ISettingsStore> m_settingsStore;
    SettingsDialog* m_activeSettingsDialog{nullptr};
    QString m_currentGestureLabel{"NONE"};
};

#endif // MAINWINDOW_H
