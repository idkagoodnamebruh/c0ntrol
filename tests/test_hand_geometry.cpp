#include <iostream>
#include <cassert>
#include "src/core/gestures/GestureClassifier.h"

void testGestureClassification() {
    Landmarks lm;
    lm.points.resize(21);

    // Configurar coordenadas para simular PINCH (pulgar 4 e índice 8 muy cerca)
    lm.points[4] = Point3D(0.50, 0.50, 0.0);
    lm.points[8] = Point3D(0.51, 0.51, 0.0);

    GestureType type = GestureClassifier::classify(lm);
    assert(type == GestureType::PINCH);

    // Configurar POINTING
    lm.points[4] = Point3D(0.20, 0.50, 0.0);
    lm.points[8] = Point3D(0.50, 0.20, 0.0); // Dedo índice extendido hacia arriba
    lm.points[6] = Point3D(0.50, 0.40, 0.0);
    
    lm.points[12] = Point3D(0.60, 0.60, 0.0); // Medio doblado
    lm.points[10] = Point3D(0.60, 0.50, 0.0);

    lm.points[16] = Point3D(0.70, 0.60, 0.0); // Anular doblado
    lm.points[14] = Point3D(0.70, 0.50, 0.0);

    lm.points[20] = Point3D(0.80, 0.60, 0.0); // Meñique doblado
    lm.points[18] = Point3D(0.80, 0.50, 0.0);

    type = GestureClassifier::classify(lm);
    assert(type == GestureType::POINTING);

    std::cout << "[PASS] testGestureClassification" << std::endl;
}

int main() {
    testGestureClassification();
    return 0;
}
