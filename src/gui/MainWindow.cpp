#include "MainWindow.h"
#include <QPainter>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
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
      m_settingsStore(std::move(settingsStore)) {

    m_centralWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_centralWidget);

    m_videoLabel = new QLabel("Iniciando transmisión de video...", this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);

    m_devWidget = new DeveloperModeWidget(this);

    m_inputBackend = createSystemInputBackend();
    m_actionDispatcher = std::make_unique<ActionDispatcher>(
        *m_inputBackend, m_runtimeConfig.pointer, m_runtimeConfig.input);
    if (!m_actionDispatcher->initialize()) {
        qWarning() << "[NATIVE INPUT]"
                   << QString::fromStdString(m_actionDispatcher->lastError());
    }
    m_configController = std::make_unique<RuntimeConfigController>(
        m_runtimeConfig, *m_actionDispatcher);

    m_layout->addWidget(m_videoLabel);
    m_layout->addWidget(m_devWidget);

    setCentralWidget(m_centralWidget);
    setWindowTitle("c0ntrol - Ventana Principal");
    resize(800, 600);
    QAction* settingsAction = menuBar()->addAction("Settings…");
    connect(settingsAction, &QAction::triggered,
            this, &MainWindow::openSettingsDialog);

    setupWorker();
}

MainWindow::~MainWindow() {
    if (m_actionDispatcher) m_actionDispatcher->shutdown();
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
    }
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
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
    if (m_actionDispatcher) {
        const ActionDispatchResult dispatched =
            m_actionDispatcher->process(result);
        if (!dispatched.success) {
            qWarning() << "[NATIVE INPUT]"
                       << QString::fromStdString(dispatched.error);
        }
    }

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

    std::string error;
    const bool persisted = reset
        ? m_settingsStore->resetToDefaults(error)
        : m_settingsStore->save(m_runtimeConfig, error);
    if (!persisted) {
        QMessageBox::warning(this, "Settings storage error",
                             QString::fromStdString(error));
    }
    return persisted;
}

void MainWindow::openSettingsDialog() {
    std::string suspensionError;
    if (!m_configController->suspendInput(suspensionError)) {
        QMessageBox::warning(this, "Settings unavailable",
            "Native input could not be safely suspended: " +
            QString::fromStdString(suspensionError));
        return;
    }

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
        }
        return;
    }

    std::string restoreError;
    if (!m_configController->cancelInputSuspension(restoreError)) {
        QMessageBox::warning(this, "Input restore failed",
            QString::fromStdString(restoreError));
    }
}
