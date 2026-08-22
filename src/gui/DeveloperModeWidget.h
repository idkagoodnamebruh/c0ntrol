#ifndef DEVELOPERMODEWIDGET_H
#define DEVELOPERMODEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "src/core/metrics/PipelineMetrics.h"
#include "src/core/qt/QtMetaTypes.h"

class DeveloperModeWidget : public QWidget {
    Q_OBJECT

public:
    explicit DeveloperModeWidget(QWidget* parent = nullptr);

public slots:
    void updateTelemetry(const PipelineMetrics& metrics,
                         const QString& gestureName);

private:
    QVBoxLayout* m_mainLayout;
    QLabel* m_captureFpsLabel;
    QLabel* m_processingFpsLabel;
    QLabel* m_dropLabel;
    QLabel* m_latencyLabel;
    QLabel* m_gestureLabel;
    QString m_gestureName;
};

#endif // DEVELOPERMODEWIDGET_H
