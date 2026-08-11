#ifndef MINIMALISTMODEWIDGET_H
#define MINIMALISTMODEWIDGET_H

#include <QWidget>
#include <QString>

class MinimalistModeWidget : public QWidget {
    Q_OBJECT

public:
    explicit MinimalistModeWidget(QWidget* parent = nullptr);

public slots:
    void updateHUD(const QString& gestureName);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_gestureName;
};

#endif // MINIMALISTMODEWIDGET_H
