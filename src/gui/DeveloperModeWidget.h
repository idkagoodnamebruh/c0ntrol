#ifndef DEVELOPERMODEWIDGET_H
#define DEVELOPERMODEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTextEdit>
#include "src/core/gestures/Landmarks.h"

class DeveloperModeWidget : public QWidget {
    Q_OBJECT

public:
    explicit DeveloperModeWidget(QWidget* parent = nullptr);

public slots:
    void updateTelemetry(double fps, const QString& gestureName, const Landmarks& landmarks);

private:
    QVBoxLayout* m_mainLayout;
    QLabel* m_fpsLabel;
    QLabel* m_gestureLabel;
    QTextEdit* m_logText;

    double m_fps;
    QString m_gestureName;
};

#endif // DEVELOPERMODEWIDGET_H
