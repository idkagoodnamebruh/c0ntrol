#include "MainWindow.h"
#include <QPainter>
#include <QDebug>
#include <QMetaObject>
#include "src/core/qt/QtMetaTypes.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_thread(nullptr), m_worker(nullptr) {

    m_centralWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_centralWidget);

    m_videoLabel = new QLabel("Iniciando transmisión de video...", this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);

    m_devWidget = new DeveloperModeWidget(this);

    m_inputBackend = createSystemInputBackend();
    m_actionDispatcher =
        std::make_unique<ActionDispatcher>(*m_inputBackend);
    if (!m_actionDispatcher->initialize()) {
        qWarning() << "[NATIVE INPUT]"
                   << QString::fromStdString(m_actionDispatcher->lastError());
    }

    m_layout->addWidget(m_videoLabel);
    m_layout->addWidget(m_devWidget);

    setCentralWidget(m_centralWidget);
    setWindowTitle("c0ntrol - Ventana Principal");
    resize(800, 600);

    setupWorker();
}

MainWindow::~MainWindow() {
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
    }
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
    if (m_actionDispatcher) m_actionDispatcher->shutdown();
}

void MainWindow::setupWorker() {
    // Required for the queued VisionWorker -> GUI connection. The declaration
    // lives in a Qt-only bridge so core tests remain independent of Qt.
    qRegisterMetaType<Landmarks>("Landmarks");
    qRegisterMetaType<HandTrackingFrame>("HandTrackingFrame");

    m_thread = new QThread(this);
    m_worker = new VisionWorker();
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(m_thread, &QThread::started, m_worker, &VisionWorker::start);
    connect(m_worker, &VisionWorker::filteredTrackingFrameProcessed, this,
            &MainWindow::onFilteredTrackingFrameProcessed);
    connect(m_worker, &VisionWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(m_worker, &VisionWorker::errorOccurred, this, [](const QString& err){
        qWarning() << "[WORKER ERROR]" << err;
    });

    m_thread->start();
}

void MainWindow::onFrameProcessed(const QImage& frame, const Landmarks& landmarks) {
    m_devWidget->updateTelemetry(30.0, m_currentGestureLabel, landmarks);

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

    const GestureObservation* selected = nullptr;
    for (std::size_t i = 0; i < result.observationCount; ++i) {
        const auto& observation = result.observations[i];
        if (selected == nullptr || observation.handedness == Handedness::RIGHT)
            selected = &observation;
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

    for (std::size_t i = 0; i < result.events.count; ++i) {
        const GestureEvent& event = result.events.events[i];
        const char* eventName = "POINTER_INACTIVE";
        switch (event.type) {
            case GestureEventType::POINTER_ACTIVE: eventName = "POINTER_ACTIVE"; break;
            case GestureEventType::POINTER_INACTIVE: eventName = "POINTER_INACTIVE"; break;
            case GestureEventType::PINCH_BEGIN: eventName = "PINCH_BEGIN"; break;
            case GestureEventType::PINCH_END: eventName = "PINCH_END"; break;
            case GestureEventType::PINCH_CANCEL: eventName = "PINCH_CANCEL"; break;
        }
        qInfo() << "[GESTURE EVENT]" << eventName
                << (event.handedness == Handedness::LEFT ? "LEFT" : "RIGHT")
                << event.timestampUs;
    }
}
