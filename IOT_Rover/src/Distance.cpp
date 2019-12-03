#include "Distance.h"

Distance::Distance():
    _proximityThreshold(DEFAULT_THRESHOLD),
    _hysteresis(DEFAULT_HYSTERESIS),
    _isWithinRange(false)
{

}

uint8_t Distance::Init()
{
    return begin();
}

uint16_t Distance::GetDistance()
{
    update();
    return _distance;
}

bool Distance::IsWithinThreshold()
{
    update();
    return _isWithinThreshold;
}

bool Distance::IsWithinRange()
{
    update();
    return _isWithinRange;
}

void Distance::update()
{
    rangingTest(&_measure, false);
    if (_measure.RangeStatus != 4)
    {
        _distance = _measure.RangeMilliMeter;
        _isWithinRange = true;
    } else
    {
        _isWithinRange = false;
    }
    uint16_t threshold;
    if (_isWithinThreshold)
    {
        threshold = _proximityThreshold + _hysteresis;
    }
    else
    {
        threshold = _proximityThreshold - _hysteresis;
    }
    _isWithinThreshold = (_distance < threshold);
}
