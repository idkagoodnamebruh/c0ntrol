#ifndef POINTERCALIBRATIONDIALOG_H
#define POINTERCALIBRATIONDIALOG_H

#include <QDialog>
#include <optional>

#include "src/core/calibration/PointerCalibration.h"

class QLabel;
class QPushButton;

class PointerCalibrationDialog final : public QDialog {
    Q_OBJECT

public:
    explicit PointerCalibrationDialog(PointerMappingConfig previous,
                                      QWidget* parent = nullptr);

    void offerSample(const Point3D& sample);
    std::optional<PointerMappingConfig> calibratedConfig() const;

private:
    void beginTopLeft();
    void beginBottomRight();
    void refreshStatus();

    static constexpr std::size_t kSamplesPerCorner = 9;
    PointerCalibration m_calibration;
    std::optional<CalibrationCorner> m_collecting;
    QLabel* m_instructions;
    QLabel* m_status;
    QPushButton* m_topLeftButton;
    QPushButton* m_bottomRightButton;
    QPushButton* m_applyButton;
};

#endif // POINTERCALIBRATIONDIALOG_H
