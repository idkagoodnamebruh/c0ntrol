#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "src/core/config/RuntimeConfig.h"

class QCheckBox;
class QSpinBox;
class PointerCalibrationDialog;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(RuntimeConfig config,
                            QWidget* parent = nullptr);

    RuntimeConfig runtimeConfig() const;
    bool resetRequested() const { return m_resetRequested; }
    void offerPointerSample(const Point3D& sample);

private:
    void openCalibration();
    void resetControls();
    void loadControls();
    void storeControls();

    RuntimeConfig m_config;
    bool m_resetRequested{false};
    QCheckBox* m_inputEnabled;
    QSpinBox* m_cameraIndex;
    QCheckBox* m_mirrorX;
    QCheckBox* m_mirrorY;
    PointerCalibrationDialog* m_activeCalibration{nullptr};
};

#endif // SETTINGSDIALOG_H
