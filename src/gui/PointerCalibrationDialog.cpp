#include "PointerCalibrationDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

PointerCalibrationDialog::PointerCalibrationDialog(
    PointerMappingConfig previous, QWidget* parent)
    : QDialog(parent), m_calibration(previous, kSamplesPerCorner) {
    setWindowTitle("Pointer calibration");
    auto* layout = new QVBoxLayout(this);
    m_instructions = new QLabel(
        "Use the pointing gesture. Collect nine stable samples at the "
        "top-left, then nine at the bottom-right of the desired active region.",
        this);
    m_instructions->setWordWrap(true);
    m_status = new QLabel(this);
    m_topLeftButton = new QPushButton("Collect top-left", this);
    m_bottomRightButton = new QPushButton("Collect bottom-right", this);
    m_bottomRightButton->setEnabled(false);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_applyButton = buttons->addButton("Apply calibration",
                                       QDialogButtonBox::AcceptRole);
    m_applyButton->setEnabled(false);

    layout->addWidget(m_instructions);
    layout->addWidget(m_status);
    layout->addWidget(m_topLeftButton);
    layout->addWidget(m_bottomRightButton);
    layout->addWidget(buttons);

    connect(m_topLeftButton, &QPushButton::clicked,
            this, &PointerCalibrationDialog::beginTopLeft);
    connect(m_bottomRightButton, &QPushButton::clicked,
            this, &PointerCalibrationDialog::beginBottomRight);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    refreshStatus();
}

void PointerCalibrationDialog::beginTopLeft() {
    m_calibration.resetSamples();
    m_collecting = CalibrationCorner::TOP_LEFT;
    m_topLeftButton->setEnabled(false);
    m_bottomRightButton->setEnabled(false);
    m_applyButton->setEnabled(false);
    refreshStatus();
}

void PointerCalibrationDialog::beginBottomRight() {
    m_collecting = CalibrationCorner::BOTTOM_RIGHT;
    m_bottomRightButton->setEnabled(false);
    m_applyButton->setEnabled(false);
    refreshStatus();
}

void PointerCalibrationDialog::offerSample(const Point3D& sample) {
    if (!m_collecting.has_value()) return;
    if (!m_calibration.addSample(*m_collecting, sample)) return;
    if (m_calibration.sampleCount(*m_collecting) >= kSamplesPerCorner) {
        if (*m_collecting == CalibrationCorner::TOP_LEFT) {
            m_bottomRightButton->setEnabled(true);
        }
        m_collecting.reset();
    }
    refreshStatus();
}

std::optional<PointerMappingConfig>
PointerCalibrationDialog::calibratedConfig() const {
    return m_calibration.result();
}

void PointerCalibrationDialog::refreshStatus() {
    const auto topLeft = m_calibration.sampleCount(
        CalibrationCorner::TOP_LEFT);
    const auto bottomRight = m_calibration.sampleCount(
        CalibrationCorner::BOTTOM_RIGHT);
    QString state = "Idle";
    if (m_collecting == CalibrationCorner::TOP_LEFT)
        state = "Collecting top-left";
    else if (m_collecting == CalibrationCorner::BOTTOM_RIGHT)
        state = "Collecting bottom-right";
    else if (m_calibration.ready() && !m_calibration.result().has_value())
        state = "Rejected: active region is too small or degenerate";
    else if (m_calibration.result().has_value())
        state = "Calibration ready";

    m_status->setText(QString("%1 — top-left %2/%3, bottom-right %4/%3")
                          .arg(state)
                          .arg(topLeft)
                          .arg(kSamplesPerCorner)
                          .arg(bottomRight));
    m_applyButton->setEnabled(m_calibration.result().has_value());
}
