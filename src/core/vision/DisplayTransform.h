#ifndef DISPLAYTRANSFORM_H
#define DISPLAYTRANSFORM_H

#include <algorithm>

class DisplayTransform {
public:
    static void mapToDisplay(double normX, double normY, int displayWidth, int displayHeight, int& outX, int& outY) {
        outX = static_cast<int>(normX * displayWidth);
        outY = static_cast<int>(normY * displayHeight);

        outX = std::clamp(outX, 0, displayWidth - 1);
        outY = std::clamp(outY, 0, displayHeight - 1);
    }
};

#endif // DISPLAYTRANSFORM_H
