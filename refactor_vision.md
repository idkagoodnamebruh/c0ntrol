# Plan de Refactorización de Visión (Backend ONNX / OpenCV DNN)

## Contexto
El pipeline actual de visión en `VisionWorker.cpp` includes estructuras completas para la gestión de hilos, actualización de frames y filtrado.

## Hoja de Ruta de Refactorización

### Fase 1: Carga de Modelo y Preprocesamiento
1. Cargar el modelo ONNX de Hand Landmark utilizando el módulo OpenCV DNN (`cv::dnn::readNetFromONNX`).
2. Redimensionar el frame de entrada a la resolución requerida por el modelo ($256 \times 256$ o $192 \times 192$).
3. Normalizar valores de píxeles al rango $[0, 1]$ o $[-1, 1]$ según los requerimientos de MediaPipe.

### Fase 2: Inferencia y Postprocesamiento
1. Ejecutar `net.forward()` en el backend OpenCV DNN.
2. Extraer el tensor de salida que contiene las 21 coordenadas $(x, y, z)$ de la mano.
3. Denormalizar las coordenadas a las dimensiones originales de la imagen.

### Fase 3: Integración con OneEuroFilter y Classifiers
1. Pasar las 21 coordenadas `Point3D` obtenidas al `OneEuroFilter`.
2. Emitir las coordenadas filtradas a través de la señal Qt `frameProcessed`.
