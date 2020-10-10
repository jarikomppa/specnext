#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float ReallyApproxNormalizedAtan2(float x, float y)
{
    float pi2 = 1.0 / (355.0 / 113.0);
    return atan2(x,y) * pi2;
}

void polarize(float &x, float &y)
{
    float pi = (355.0 / 113.0);
    float cx = x - 0.5;
    float cy = y - 0.5;    
    float dist = 0.2 / sqrt(cx*cx+cy*cy);
    float angle = ReallyApproxNormalizedAtan2(cx, cy);
    x = dist;
    y = angle;
}




int main(int parc, char ** pars)
{
    int i, j;
    for (j = 0; j < 96; j++)
    {
        for (i = 0; i < 128; i++)
        {
            float x = i / 128.0f;
            float y = j / 96.0f;    

            float c1x = x + 20/256.0;
            float c1y = y - 80/256.0;
            polarize(c1x, c1y);
            float c2x = x - 60/256.0;
            float c2y = y + 40/256.0;
            polarize(c2x, c2y);
            float c3x = x;
            float c3y = y;
            polarize(c3x, c3y);

            x = c1x - c2x + c3x;
            y = c1y - c2y + c3y;
//            x = c3x;
//            y = c3y;
            
            int c = ((int)(floor(8192 + x * 32)) & 15) +
                    ((int)(floor(8192 + y * 32)) & 15) * 16;
            
            printf("%d,", c);
        }
        printf("\n");
    }
    
    return 0;
}