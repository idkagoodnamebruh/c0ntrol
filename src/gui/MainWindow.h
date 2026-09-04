#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>
#include <string>
#include "src/core/config/ISettingsStore.h"
#include "src/core/config/RuntimeConfigController.h"
#include "src/core/input/NativeInputRuntime.h"
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
                        std::string handModelPath,
                        QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFrameProcessed(const QImage& frame, const Landmarks& landmarks);
    void onFilteredTrackingFrameProcessed(const HandTrackingFrame& trackingFrame);
    void onPipelineMetricsUpdated(const PipelineMetrics& metrics);
    void openSettingsDialog();
    void refreshNativeInputStatus();

private:
    void setupWorker();
    const GestureObservation* selectPreferredObservation(
        const GesturePipelineResult& result) const;
    bool applyRuntimeConfig(const RuntimeConfig& requested, bool reset);
    bool persistConfig(const RuntimeConfig& config, bool reset);
    void showSettingsDialog();
    void requestCurrentNativeInputConfiguration();

    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_videoLabel;
    DeveloperModeWidget* m_devWidget;

    QThread* m_thread;
    VisionWorker* m_worker;
    RuntimeConfig m_runtimeConfig;
    GesturePipeline m_gesturePipeline;
    std::unique_ptr<NativeInputRuntime> m_nativeInputRuntime;
    std::unique_ptr<RuntimeConfigController> m_configController;
    std::unique_ptr<ISettingsStore> m_settingsStore;
    std::string m_handModelPath;
    QTimer* m_nativeInputStatusTimer{nullptr};
    SettingsDialog* m_activeSettingsDialog{nullptr};
    bool m_persistedInputEnabled{false};
    bool m_settingsOpenPending{false};
    bool m_restoreInputAfterSettings{false};
    QString m_currentGestureLabel{"NONE"};
};

#endif // MAINWINDOW_H
