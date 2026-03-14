#include <OpenSteer/Annotation.h>
#include <OpenSteer/Draw.h>

namespace OpenSteer
{
    bool enableAnnotation = false;
    bool drawPhaseActive = false;

    void drawLine(const Vec3 &startPoint, const Vec3 &endPoint, const Color &color)
    {
        (void)startPoint;
        (void)endPoint;
        (void)color;
    }

    void deferredDrawLine(const Vec3 &startPoint, const Vec3 &endPoint, const Color &color)
    {
        (void)startPoint;
        (void)endPoint;
        (void)color;
    }
}
