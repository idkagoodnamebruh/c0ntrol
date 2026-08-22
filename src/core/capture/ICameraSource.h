#ifndef ICAMERASOURCE_H
#define ICAMERASOURCE_H

#include <string>

enum class CameraReadStatus {
    FRAME,
    RETRYABLE_FAILURE,
    END_OF_STREAM,
    FATAL_ERROR,
    STOPPED,
};

template <typename T>
class ICameraSource {
public:
    virtual ~ICameraSource() = default;

    // open(), read() and close() are called only by the capture producer.
    virtual bool open(std::string& error) = 0;
    virtual CameraReadStatus read(T& frame, std::string& error) = 0;
    // requestStop() must be thread-safe and should unblock read when possible.
    virtual void requestStop() = 0;
    virtual void close() = 0;
};

#endif // ICAMERASOURCE_H
