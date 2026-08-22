#ifndef OPENCVCAMERASOURCE_H
#define OPENCVCAMERASOURCE_H

#include <atomic>
#include <mutex>
#include <opencv2/videoio.hpp>
#include <string>

#include "src/core/capture/ICameraSource.h"

enum class CameraPropertySupport {
    UNKNOWN,
    SUPPORTED,
    NOT_SUPPORTED,
};

struct CameraConfig {
    int index{0};
    int requestedWidth{640};
    int requestedHeight{480};
    double requestedFps{30.0};
    int requestedBufferSize{1};
};

struct CameraInfo {
    CameraConfig requested;
    std::string backendName{"UNKNOWN"};
    double actualWidth{0.0};
    double actualHeight{0.0};
    double actualFps{0.0};
    double actualBufferSize{0.0};
    CameraPropertySupport bufferSizeSupport{CameraPropertySupport::UNKNOWN};
    CameraPropertySupport readTimeoutSupport{CameraPropertySupport::UNKNOWN};
};

class OpenCVCameraSource final : public ICameraSource<cv::Mat> {
public:
    explicit OpenCVCameraSource(CameraConfig config = {});

    bool setConfig(const CameraConfig& config);
    CameraInfo cameraInfo() const;

    bool open(std::string& error) override;
    CameraReadStatus read(cv::Mat& frame, std::string& error) override;
    void requestStop() override;
    void close() override;

private:
    mutable std::mutex m_infoMutex;
    CameraConfig m_config;
    CameraInfo m_info;
    cv::VideoCapture m_capture;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_opened{false};
};

#endif // OPENCVCAMERASOURCE_H
