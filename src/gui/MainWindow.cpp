#include "MainWindow.h"
#include <QPainter>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QStatusBar>
#include <QTimer>
#include "src/core/qt/QtMetaTypes.h"
#include "src/gui/SettingsDialog.h"

MainWindow::MainWindow(RuntimeConfig config,
                       std::unique_ptr<ISettingsStore> settingsStore,
                       QWidget* parent)
    : QMainWindow(parent),
      m_thread(nullptr),
      m_worker(nullptr),
      m_runtimeConfig(sanitizeRuntimeConfig(config)),
      m_gesturePipeline(m_runtimeConfig.gestures,
                        m_runtimeConfig.dynamicGestures),
      m_settingsStore(std::move(settingsStore)),
      m_persistedInputEnabled(m_runtimeConfig.input.enabled) {

    m_centralWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_centralWidget);

    m_videoLabel = new QLabel("Iniciando transmisión de video...", this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);

    m_devWidget = new DeveloperModeWidget(this);

    // NativeInputRuntime forces an initially disabled dispatcher and owns both
    // it and the backend on its dedicated worker. Construction cannot open a
    // portal or call SendInput.
    m_nativeInputRuntime = std::make_unique<NativeInputRuntime>(
        createSystemInputBackend(), m_runtimeConfig.pointer,
        m_runtimeConfig.input);
    m_configController = std::make_unique<RuntimeConfigController>(
        m_runtimeConfig);

    m_layout->addWidget(m_videoLabel);
    m_layout->addWidget(m_devWidget);

    setCentralWidget(m_centralWidget);
    setWindowTitle("c0ntrol - Ventana Principal");
    resize(800, 600);
    QAction* settingsAction = menuBar()->addAction("Settings…");
    connect(settingsAction, &QAction::triggered,
            this, &MainWindow::openSettingsDialog);

    setupWorker();

    m_nativeInputStatusTimer = new QTimer(this);
    m_nativeInputStatusTimer->setInterval(50);
    connect(m_nativeInputStatusTimer, &QTimer::timeout,
            this, &MainWindow::refreshNativeInputStatus);
    m_nativeInputStatusTimer->start();
    refreshNativeInputStatus();

    // A persisted opt-in is retried only after Qt's event loop can render and
    // process input. First-run disabled configuration schedules nothing.
    if (m_runtimeConfig.input.enabled) {
        QTimer::singleShot(0, this, [this] {
            requestCurrentNativeInputConfiguration();
        });
    }
}

MainWindow::~MainWindow() {
    if (m_nativeInputStatusTimer) m_nativeInputStatusTimer->stop();
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
    }
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
    if (m_nativeInputRuntime) m_nativeInputRuntime->shutdown();
}

void MainWindow::setupWorker() {
    // Required for the queued VisionWorker -> GUI connection. The declaration
    // lives in a Qt-only bridge so core tests remain independent of Qt.
    qRegisterMetaType<Landmarks>("Landmarks");
    qRegisterMetaType<HandTrackingFrame>("HandTrackingFrame");
    qRegisterMetaType<PipelineMetrics>("PipelineMetrics");

    m_thread = new QThread(this);
    m_worker = new VisionWorker(m_runtimeConfig.camera,
                                m_runtimeConfig.filtering);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(m_thread, &QThread::started, m_worker, &VisionWorker::start);
    connect(m_worker, &VisionWorker::filteredTrackingFrameProcessed, this,
            &MainWindow::onFilteredTrackingFrameProcessed);
    connect(m_worker, &VisionWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(m_worker, &VisionWorker::metricsUpdated, this,
            &MainWindow::onPipelineMetricsUpdated);
    connect(m_worker, &VisionWorker::errorOccurred, this, [](const QString& err){
        qWarning() << "[WORKER ERROR]" << err;
    });

    m_thread->start();
}

void MainWindow::onFrameProcessed(const QImage& frame, const Landmarks& landmarks) {
    // Dibujar landmarks sobre la imagen de cámara
    QImage overlayFrame = frame.copy();
    QPainter painter(&overlayFrame);
    painter.setPen(QPen(Qt::green, 4));

    for (const auto& pt : landmarks.points) {
        int x = static_cast<int>(pt.x * overlayFrame.width());
        int y = static_cast<int>(pt.y * overlayFrame.height());
        painter.drawEllipse(QPoint(x, y), 3, 3);
    }
    painter.end();

    m_videoLabel->setPixmap(QPixmap::fromImage(overlayFrame));
}

void MainWindow::onPipelineMetricsUpdated(const PipelineMetrics& metrics) {
    m_devWidget->updateTelemetry(metrics, m_currentGestureLabel);
}

void MainWindow::onFilteredTrackingFrameProcessed(
    const HandTrackingFrame& trackingFrame) {
    const GesturePipelineResult result = m_gesturePipeline.process(trackingFrame);
    if (m_nativeInputRuntime) m_nativeInputRuntime->submitLatest(result);

    const GestureObservation* selected = selectPreferredObservation(result);
    if (m_activeSettingsDialog != nullptr && selected != nullptr &&
        selected->pointerActive) {
        m_activeSettingsDialog->offerPointerSample(selected->pointerPoint);
    }

    m_currentGestureLabel = "NONE";
    if (selected != nullptr) {
        switch (selected->pose) {
            case StaticGesture::OPEN_HAND:
                m_currentGestureLabel = "OPEN_HAND";
                break;
            case StaticGesture::POINTING:
                m_currentGestureLabel = selected->pinchActive
                    ? "POINTING+PINCH" : "POINTING";
                break;
            case StaticGesture::PINCH:
                m_currentGestureLabel = "PINCH";
                break;
            default:
                break;
        }
    }
}

const GestureObservation* MainWindow::selectPreferredObservation(
    const GesturePipelineResult& result) const {
    const GestureObservation* fallback = nullptr;
    for (std::size_t i = 0; i < result.observationCount; ++i) {
        const auto& observation = result.observations[i];
        if (!observation.valid) continue;
        if (observation.handedness == m_runtimeConfig.input.preferredHand)
            return &observation;
        fallback = &observation;
    }
    return fallback;
}

bool MainWindow::applyRuntimeConfig(const RuntimeConfig& requested,
                                    bool reset) {
    const RuntimeConfigApplyResult result =
        m_configController->inputSuspended()
        ? m_configController->completeInputSuspension(requested, reset)
        : (reset ? m_configController->resetToDefaults()
                 : m_configController->apply(requested));
    if (!result.success) {
        QMessageBox::warning(this, "Settings not applied",
            QString::fromStdString(result.error));
        return false;
    }

    m_runtimeConfig = m_configController->current();
    if (result.changes.gesturesChanged ||
        result.changes.dynamicGesturesChanged) {
        m_gesturePipeline = GesturePipeline(m_runtimeConfig.gestures,
                                            m_runtimeConfig.dynamicGestures);
    }
    if (m_worker != nullptr &&
        (result.changes.cameraRestartRequired ||
         result.changes.filteringChanged)) {
        const CameraConfig camera = m_runtimeConfig.camera;
        const LandmarkFilterConfig filtering = m_runtimeConfig.filtering;
        QMetaObject::invokeMethod(m_worker, [this, camera, filtering] {
            m_worker->applyConfiguration(camera, filtering);
        }, Qt::BlockingQueuedConnection);
    }

    // An opt-in is persisted only after this activation reaches READY. Other
    // settings are stored immediately with an effective enabled=false value.
    RuntimeConfig persistedConfig = m_runtimeConfig;
    if (persistedConfig.input.enabled) persistedConfig.input.enabled = false;
    const bool persisted = persistConfig(persistedConfig, reset);
    m_persistedInputEnabled = false;
    requestCurrentNativeInputConfiguration();
    return persisted;
}

void MainWindow::openSettingsDialog() {
    if (m_activeSettingsDialog != nullptr || m_settingsOpenPending) return;

    const NativeInputStatus inputStatus = m_nativeInputRuntime->status();
    m_restoreInputAfterSettings = m_runtimeConfig.input.enabled &&
        inputStatus.state != NativeInputState::FAILED;
    std::string suspensionError;
    if (!m_configController->suspendInput(suspensionError)) {
        QMessageBox::warning(this, "Settings unavailable",
            "Native input could not be safely suspended: " +
            QString::fromStdString(suspensionError));
        return;
    }

    m_nativeInputRuntime->requestEnabled(false);
    if (inputStatus.state == NativeInputState::READY ||
        inputStatus.state == NativeInputState::STOPPING) {
        // READY may own a button. Wait asynchronously for the worker's release
        // boundary instead of entering a modal dialog or blocking the GUI.
        m_settingsOpenPending = true;
        return;
    }

    // ACTIVATING cannot own a native button and desired=false immediately
    // makes frame submission non-dispatching. Its eventual result is stale.
    showSettingsDialog();
}

void MainWindow::showSettingsDialog() {
    m_settingsOpenPending = false;

    SettingsDialog dialog(m_runtimeConfig, this);
    m_activeSettingsDialog = &dialog;
    const int result = dialog.exec();
    m_activeSettingsDialog = nullptr;
    if (result == QDialog::Accepted) {
        if (!applyRuntimeConfig(dialog.runtimeConfig(),
                                dialog.resetRequested())) {
            std::string restoreError;
            if (!m_configController->cancelInputSuspension(restoreError)) {
                qWarning() << "[NATIVE INPUT] restore after failed settings:"
                           << QString::fromStdString(restoreError);
            }
            if (m_restoreInputAfterSettings)
                requestCurrentNativeInputConfiguration();
        }
        return;
    }

    std::string restoreError;
    if (!m_configController->cancelInputSuspension(restoreError)) {
        QMessageBox::warning(this, "Input restore failed",
            QString::fromStdString(restoreError));
    }
    if (m_restoreInputAfterSettings)
        requestCurrentNativeInputConfiguration();
}

bool MainWindow::persistConfig(const RuntimeConfig& config, bool reset) {
    std::string error;
    const bool persisted = reset
        ? m_settingsStore->resetToDefaults(error)
        : m_settingsStore->save(config, error);
    if (!persisted) {
        QMessageBox::warning(this, "Settings storage error",
                             QString::fromStdString(error));
    }
    return persisted;
}

void MainWindow::requestCurrentNativeInputConfiguration() {
    if (!m_nativeInputRuntime) return;
    m_nativeInputRuntime->requestConfiguration(m_runtimeConfig.pointer,
                                               m_runtimeConfig.input);
}

void MainWindow::refreshNativeInputStatus() {
    if (!m_nativeInputRuntime) return;
    const NativeInputStatus inputStatus = m_nativeInputRuntime->status();
    QString message = "Native input: " + QString::fromLatin1(
        nativeInputStateName(inputStatus.state));
    if (inputStatus.state == NativeInputState::ACTIVATING)
        message = "Native input: Activating…";
    if (inputStatus.state == NativeInputState::FAILED &&
        !inputStatus.error.empty()) {
        message += " — " +
            QString::fromStdString(inputStatus.error).left(160);
    }
    statusBar()->showMessage(message);

    if (inputStatus.state == NativeInputState::READY &&
        m_runtimeConfig.input.enabled && !m_persistedInputEnabled) {
        if (persistConfig(m_runtimeConfig, false))
            m_persistedInputEnabled = true;
    } else if (inputStatus.state == NativeInputState::FAILED &&
               m_runtimeConfig.input.enabled) {
        qWarning() << "[NATIVE INPUT]"
                   << QString::fromStdString(inputStatus.error);
        RuntimeConfig safeConfig = m_runtimeConfig;
        safeConfig.input.enabled = false;
        const RuntimeConfigApplyResult applied =
            m_configController->apply(safeConfig);
        if (applied.success) m_runtimeConfig = m_configController->current();
        persistConfig(safeConfig, false);
        m_persistedInputEnabled = false;
    }

    if (m_settingsOpenPending &&
        (inputStatus.state == NativeInputState::DISABLED ||
         inputStatus.state == NativeInputState::FAILED)) {
        m_settingsOpenPending = false;
        QTimer::singleShot(0, this, [this] { showSettingsDialog(); });
    }
    m_lastNativeInputState = inputStatus.state;
}
