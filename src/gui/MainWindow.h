#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QVBoxLayout>
#include "src/core/vision/VisionWorker.h"
#include "src/core/gestures/GesturePipeline.h"
#include "src/core/actions/CursorController.h"
#include "src/gui/DeveloperModeWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFrameProcessed(const QImage& frame, const Landmarks& landmarks);
    void onFilteredTrackingFrameProcessed(const HandTrackingFrame& trackingFrame);

private:
    void setupWorker();

    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_videoLabel;
    DeveloperModeWidget* m_devWidget;

    QThread* m_thread;
    VisionWorker* m_worker;
    CursorController* m_cursorCtrl;
    GesturePipeline m_gesturePipeline;
    QString m_currentGestureLabel{"NONE"};
};

#endif // MAINWINDOW_H
