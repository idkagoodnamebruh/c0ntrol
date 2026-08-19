#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct AppSettings {
    int cameraIndex = 0;
    int targetFps = 30;
    int frameWidth = 640;
    int frameHeight = 480;
    
    // Umbrales de Clasificación de Gestos
    double pinchThreshold = 0.05;
    double swipeDistanceThreshold = 0.15;

    // Rutas de Modelos
    std::string modelPath = "models/hand_landmark.onnx";
};

#endif // SETTINGS_H
