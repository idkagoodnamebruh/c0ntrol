#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QVBoxLayout>
#include "src/core/vision/VisionWorker.h"
#include "src/core/gestures/GestureClassifier.h"
#include "src/core/actions/CursorController.h"
#include "src/gui/DeveloperModeWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFrameProcessed(const QImage& frame, const Landmarks& landmarks);

private:
    void setupWorker();

    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_videoLabel;
    DeveloperModeWidget* m_devWidget;

    QThread* m_thread;
    VisionWorker* m_worker;
    CursorController* m_cursorCtrl;
    GestureType m_currentGesture;
};

#endif // MAINWINDOW_H
