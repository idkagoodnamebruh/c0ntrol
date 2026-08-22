#include "OpenCVCameraSource.h"

#include <cmath>
#include <exception>
#include <utility>

OpenCVCameraSource::OpenCVCameraSource(CameraConfig config)
    : m_config(config) {
    m_info.requested = config;
}

bool OpenCVCameraSource::setConfig(const CameraConfig& config) {
    if (m_opened.load()) return false;
    std::lock_guard lock(m_infoMutex);
    m_config = config;
    m_info.requested = config;
    return true;
}

CameraInfo OpenCVCameraSource::cameraInfo() const {
    std::lock_guard lock(m_infoMutex);
    return m_info;
}

bool OpenCVCameraSource::open(std::string& error) {
    m_stopRequested.store(false);
    CameraConfig config;
    {
        std::lock_guard lock(m_infoMutex);
        config = m_config;
        m_info = {};
        m_info.requested = config;
    }

    try {
        if (!m_capture.open(config.index)) {
            if (m_capture.isOpened()) m_capture.release();
            error = "CAMERA_OPEN_FAILED: OpenCV could not open camera index " +
                    std::to_string(config.index);
            return false;
        }
        m_opened.store(true);

        (void)m_capture.set(cv::CAP_PROP_FRAME_WIDTH,
                            static_cast<double>(config.requestedWidth));
        (void)m_capture.set(cv::CAP_PROP_FRAME_HEIGHT,
                            static_cast<double>(config.requestedHeight));
        (void)m_capture.set(cv::CAP_PROP_FPS, config.requestedFps);
        const bool bufferSet = m_capture.set(
            cv::CAP_PROP_BUFFERSIZE,
            static_cast<double>(config.requestedBufferSize));

        CameraInfo observed;
        observed.requested = config;
        observed.backendName = m_capture.getBackendName();
        observed.actualWidth = m_capture.get(cv::CAP_PROP_FRAME_WIDTH);
        observed.actualHeight = m_capture.get(cv::CAP_PROP_FRAME_HEIGHT);
        observed.actualFps = m_capture.get(cv::CAP_PROP_FPS);
        observed.actualBufferSize = m_capture.get(cv::CAP_PROP_BUFFERSIZE);
        if (!bufferSet) {
            observed.bufferSizeSupport = CameraPropertySupport::NOT_SUPPORTED;
        } else if (std::isfinite(observed.actualBufferSize) &&
                   std::abs(observed.actualBufferSize -
                            static_cast<double>(config.requestedBufferSize)) <
                       0.5) {
            observed.bufferSizeSupport = CameraPropertySupport::SUPPORTED;
        } else {
            // A backend may accept set() while ignoring or transforming the
            // value. Do not claim support unless get() confirms it.
            observed.bufferSizeSupport = CameraPropertySupport::UNKNOWN;
        }
        // OpenCV documents read timeout as open-only and only for FFmpeg and
        // GStreamer. Camera backends selected by CAP_ANY are therefore left
        // untouched and reported as unknown.
        observed.readTimeoutSupport = CameraPropertySupport::UNKNOWN;
        {
            std::lock_guard lock(m_infoMutex);
            m_info = std::move(observed);
        }
        return true;
    } catch (const cv::Exception& exception) {
        error = "CAMERA_OPEN_FAILED: " + std::string(exception.what());
    } catch (const std::exception& exception) {
        error = "CAMERA_OPEN_FAILED: " + std::string(exception.what());
    }

    if (m_capture.isOpened()) m_capture.release();
    m_opened.store(false);
    return false;
}

CameraReadStatus OpenCVCameraSource::read(cv::Mat& frame,
                                          std::string& error) {
    if (m_stopRequested.load()) return CameraReadStatus::STOPPED;
    try {
        cv::Mat captured;
        if (!m_capture.read(captured) || captured.empty()) {
            if (m_stopRequested.load()) return CameraReadStatus::STOPPED;
            error = "CAMERA_READ_FAILED: OpenCV returned no frame";
            return CameraReadStatus::RETRYABLE_FAILURE;
        }
        if (m_stopRequested.load()) return CameraReadStatus::STOPPED;
        frame = std::move(captured);
        return CameraReadStatus::FRAME;
    } catch (const cv::Exception& exception) {
        error = "CAMERA_READ_FAILED: " + std::string(exception.what());
        return CameraReadStatus::RETRYABLE_FAILURE;
    } catch (const std::exception& exception) {
        error = "CAMERA_READ_FAILED: " + std::string(exception.what());
        return CameraReadStatus::FATAL_ERROR;
    }
}

void OpenCVCameraSource::requestStop() {
    // Do not call release() here: the producer thread exclusively owns the
    // VideoCapture object. Some drivers may still delay stop until read()
    // returns, which is documented as a backend limitation.
    m_stopRequested.store(true);
}

void OpenCVCameraSource::close() {
    if (m_capture.isOpened()) m_capture.release();
    m_opened.store(false);
}
