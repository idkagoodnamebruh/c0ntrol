#ifndef C0NTROL_MEDIAPIPE_BRIDGE_H
#define C0NTROL_MEDIAPIPE_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#if defined(C0NTROL_MP_BRIDGE_BUILD)
#define C0NTROL_MP_API __declspec(dllexport)
#else
#define C0NTROL_MP_API __declspec(dllimport)
#endif
#else
#define C0NTROL_MP_API __attribute__((visibility("default")))
#endif

#define C0NTROL_MP_MAX_HANDS 2
#define C0NTROL_MP_LANDMARKS 21
#define C0NTROL_MP_ERROR_SIZE 512

typedef struct { float x, y, z; } C0ntrolMpPoint;
typedef struct {
  C0ntrolMpPoint normalized[C0NTROL_MP_LANDMARKS];
  C0ntrolMpPoint world[C0NTROL_MP_LANDMARKS];
  int has_world;
  int handedness; /* 0 unknown, 1 left, 2 right */
  float handedness_score;
} C0ntrolMpHand;
typedef struct {
  C0ntrolMpHand hands[C0NTROL_MP_MAX_HANDS];
  int hand_count;
} C0ntrolMpResult;
typedef struct C0ntrolMpHandle C0ntrolMpHandle;

C0NTROL_MP_API C0ntrolMpHandle* c0ntrol_mp_create(
    const char* model_path, int num_hands, float detection_confidence,
    float presence_confidence, float tracking_confidence,
    char* error, int error_size);
C0NTROL_MP_API int c0ntrol_mp_detect_video(
    C0ntrolMpHandle* handle, const uint8_t* rgb, int width, int height,
    int stride, int64_t timestamp_ms, C0ntrolMpResult* result,
    char* error, int error_size);
C0NTROL_MP_API int c0ntrol_mp_close(
    C0ntrolMpHandle* handle, char* error, int error_size);

#ifdef __cplusplus
}
#endif

#undef C0NTROL_MP_API
#endif
