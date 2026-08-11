#include "MainWindow.h"
#include <QPainter>
#include <QDebug>
#include <QMetaObject>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_thread(nullptr), m_worker(nullptr),
      m_currentGesture(GestureType::NONE) {

    m_centralWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_centralWidget);

    m_videoLabel = new QLabel("Iniciando transmisión de video...", this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);

    m_devWidget = new DeveloperModeWidget(this);
    m_cursorCtrl = new CursorController(this);

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
}

void MainWindow::setupWorker() {
    m_thread = new QThread(this);
    m_worker = new VisionWorker();
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &VisionWorker::start);
    connect(m_worker, &VisionWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(m_worker, &VisionWorker::errorOccurred, this, [](const QString& err){
        qWarning() << "[WORKER ERROR]" << err;
    });

    m_thread->start();
}

void MainWindow::onFrameProcessed(const QImage& frame, const Landmarks& landmarks) {
    m_currentGesture = GestureClassifier::classify(landmarks);
    m_cursorCtrl->onLandmarksUpdated(landmarks, m_currentGesture);

    QString gestureStr = "NONE";
    switch (m_currentGesture) {
        case GestureType::POINTING: gestureStr = "POINTING"; break;
        case GestureType::PINCH: gestureStr = "PINCH"; break;
        case GestureType::PALM_OPEN: gestureStr = "PALM_OPEN"; break;
        case GestureType::FIST: gestureStr = "FIST"; break;
        case GestureType::VICTORY: gestureStr = "VICTORY"; break;
        default: break;
    }

    m_devWidget->updateTelemetry(30.0, gestureStr, landmarks);

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
