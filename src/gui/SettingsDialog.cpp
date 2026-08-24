#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "src/gui/PointerCalibrationDialog.h"

SettingsDialog::SettingsDialog(RuntimeConfig config, QWidget* parent)
    : QDialog(parent), m_config(sanitizeRuntimeConfig(config)) {
    setWindowTitle("c0ntrol settings");
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_inputEnabled = new QCheckBox("Enable native pointer input", this);
    m_swipeScrollEnabled = new QCheckBox("Enable swipe scrolling", this);
    m_scrollNotches = new QSpinBox(this);
    m_scrollNotches->setRange(1, 10);
    m_invertSwipeScroll = new QCheckBox("Invert swipe scroll", this);
    m_cameraIndex = new QSpinBox(this);
    m_cameraIndex->setRange(0, 63);
    m_mirrorX = new QCheckBox("Mirror horizontal axis", this);
    m_mirrorY = new QCheckBox("Mirror vertical axis", this);
    form->addRow(m_inputEnabled);
    form->addRow(m_swipeScrollEnabled);
    form->addRow("Scroll amount (notches)", m_scrollNotches);
    form->addRow(m_invertSwipeScroll);
    form->addRow("Camera index", m_cameraIndex);
    form->addRow(m_mirrorX);
    form->addRow(m_mirrorY);

    auto* calibration = new QPushButton("Calibrate pointer…", this);
    auto* reset = new QPushButton("Reset all defaults", this);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addLayout(form);
    layout->addWidget(calibration);
    layout->addWidget(reset);
    layout->addWidget(buttons);

    connect(calibration, &QPushButton::clicked,
            this, &SettingsDialog::openCalibration);
    connect(reset, &QPushButton::clicked,
            this, &SettingsDialog::resetControls);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        storeControls();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    loadControls();
}

void SettingsDialog::loadControls() {
    m_inputEnabled->setChecked(m_config.input.enabled);
    m_swipeScrollEnabled->setChecked(
        m_config.input.swipeScrollEnabled);
    m_scrollNotches->setValue(m_config.input.scrollNotchesPerSwipe);
    m_invertSwipeScroll->setChecked(m_config.input.invertSwipeScroll);
    m_cameraIndex->setValue(m_config.camera.index);
    m_mirrorX->setChecked(m_config.pointer.mirrorX);
    m_mirrorY->setChecked(m_config.pointer.mirrorY);
}

void SettingsDialog::storeControls() {
    m_config.input.enabled = m_inputEnabled->isChecked();
    m_config.input.swipeScrollEnabled = m_swipeScrollEnabled->isChecked();
    m_config.input.scrollNotchesPerSwipe = m_scrollNotches->value();
    m_config.input.invertSwipeScroll = m_invertSwipeScroll->isChecked();
    m_config.camera.index = m_cameraIndex->value();
    m_config.pointer.mirrorX = m_mirrorX->isChecked();
    m_config.pointer.mirrorY = m_mirrorY->isChecked();
    m_config = sanitizeRuntimeConfig(m_config);
    if (m_config != RuntimeConfig{}) m_resetRequested = false;
}

RuntimeConfig SettingsDialog::runtimeConfig() const {
    return sanitizeRuntimeConfig(m_config);
}

void SettingsDialog::resetControls() {
    m_config = RuntimeConfig{};
    m_resetRequested = true;
    loadControls();
}

void SettingsDialog::openCalibration() {
    storeControls();
    PointerCalibrationDialog dialog(m_config.pointer, this);
    m_activeCalibration = &dialog;
    if (dialog.exec() == QDialog::Accepted) {
        const auto calibrated = dialog.calibratedConfig();
        if (calibrated.has_value()) {
            m_config.pointer = *calibrated;
            m_resetRequested = false;
        }
    }
    m_activeCalibration = nullptr;
    loadControls();
}

void SettingsDialog::offerPointerSample(const Point3D& sample) {
    if (m_activeCalibration != nullptr)
        m_activeCalibration->offerSample(sample);
}
