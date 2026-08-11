#ifndef MATRIXRAINWIDGET_H
#define MATRIXRAINWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <vector>

struct RainDrop {
    int x;
    int y;
    int speed;
    char character;
};

class MatrixRainWidget : public QWidget {
    Q_OBJECT

public:
    explicit MatrixRainWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTimerTimeout();

private:
    QTimer* m_timer;
    std::vector<RainDrop> m_drops;
};

#endif // MATRIXRAINWIDGET_H
