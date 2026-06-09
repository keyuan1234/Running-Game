#include "util.h"
#include <stdlib.h>

int rects_overlap(RECT a, RECT b)
{
    return a.left < b.right &&
           a.right > b.left &&
           a.top < b.bottom &&
           a.bottom > b.top;
}

float rand_float(float min_value, float max_value)
{
    float t = (float)rand() / (float)RAND_MAX;
    return min_value + (max_value - min_value) * t;
}

