#ifndef CURSORCONTROLLER_H
#define CURSORCONTROLLER_H

#include <QObject>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QRect>
#include <algorithm>
#include "src/core/gestures/Landmarks.h"
#include "src/core/qt/QtMetaTypes.h"

class CursorController : public QObject {
    Q_OBJECT

public:
    explicit CursorController(QObject* parent = nullptr);

    void setEnabled(bool enabled);
    bool isEnabled() const;

public slots:
    void onPointerUpdated(const Point3D& pointer, bool active);
    void moveCursor(int x, int y);
    void performClick(Qt::MouseButton button);

signals:
    void cursorMoved(int x, int y);
    void clickPerformed(Qt::MouseButton button);

private:
    bool m_enabled;
    QRect m_screenGeometry;
    int m_lastX;
    int m_lastY;
};

#endif // CURSORCONTROLLER_H
