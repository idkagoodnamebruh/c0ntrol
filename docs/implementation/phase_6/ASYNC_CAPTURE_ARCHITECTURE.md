# Asynchronous capture architecture

```text
Capture producer thread
  OpenCVCameraSource / VideoCapture
              |
              v
   LatestFrameSlot<cv::Mat>
       logical capacity 1
              |
              v
 VisionWorker Qt thread (5 ms bounded poll)
              |
       BGR -> RGB once
              |
          MediaPipe
              |
      LandmarkFilterBank
              |
       GesturePipeline
              |
      ActionDispatcher
```

## Boundaries and ownership

`AsyncCapture<cv::Mat>` owns both the producer `std::jthread` and its
`ICameraSource<cv::Mat>`. `OpenCVCameraSource` owns `cv::VideoCapture`. The
producer thread performs `open`, every `read`, and `close`/`release`; neither
the GUI thread nor the VisionWorker consumer accesses the `VideoCapture`.

The latest slot is protected by a short mutex held only while replacing or
moving one `CapturedFrame`. No mutex is held during camera read, color
conversion, tracking or GUI work. The capture rate tracker has a separate
short metrics lock.

## Frame lifetime

The source reads into a fresh local BGR `cv::Mat` and moves it into the slot.
Publishing a newer frame destroys the previous pending reference. Consumption
moves the sole pending frame out and empties the slot, so capture cannot mutate
the pixels MediaPipe is reading. The consumer performs one BGR-to-RGB
conversion only after a frame wins the slot. A final `QImage::copy()` gives a
queued GUI signal independent storage.

## Consumer wakeup

VisionWorker polls the capacity-one slot with a precise 5 ms Qt timer. The
callback performs no camera read and returns immediately when the slot is
empty. This avoids a queued signal per captured frame and caps pending camera
work at one frame. The interval is a latency/polling choice, not reported as
camera or processing FPS.

## Failure policy

Five consecutive retryable reads, or one fatal read, transitions capture to
`FAILED`. Retryable failures use a cancelable 10 ms condition-variable backoff,
so a source that returns immediately cannot busy-spin. VisionWorker emits the
retained structured error once and stops tracking. Complex automatic reconnect
is intentionally deferred; an explicit later `start()` reopens the source.

`requestStop()` is part of the source contract. The fake source proves that a
cooperative blocking read is canceled and joined. `OpenCVCameraSource` sets a
stop flag but does not call `release()` concurrently with `read()`; a real
backend that never returns from `read()` can still delay shutdown. That
driver-level limitation was not validated without a camera.
