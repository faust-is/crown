#include "special.h"
#include "math.h"

#define EXPINT_MAXIT 100 /* Maximum allowed number of iterations */
#define EULER 0.577215664901533 /* Euler’s constant */
#define EXPINT_FPMAX 1.0e+30 /* Close to biggest representable floating-point number */

#define EXPINT_EPS 1.0e-6f /* Desired relative error, not smaller than the machine precision */

/* Exponential integral E(1,x) = -Ei(-x) for x > 0
   can be referred to MATLAB expint(x) function.
   Reference code: William H. Press "Numerical Recipes in C" 3-rd edition,
   Cambridge University Press, 2007
*/
float expint(float x)
{
#ifndef EXPINT_ROUGH
    int i;
    float a,b,c,d,del,fact,h,ans = -1.0;

    if (x <= 0.0) return -1.0;

    if (x > 1.0) {
        b = x + 1.0;
        c = EXPINT_FPMAX;
        d = 1.0 / b;
        h = d;
        for (i = 1;i <= EXPINT_MAXIT;i++) {
            /* Lentz’s algorithm */
            a = -i * i;
            b += 2.0;
            d = 1.0 / (a * d + b);
            c = b + a / c;
            del = c * d;
            h *= del;
            if (fabs(del - 1.0) < EXPINT_EPS) {
                ans = h * exp(-x);
                return ans;
            }
        }
    } else {
        ans = -log(x) - EULER;
        fact = 1.0;
        for (i = 1;i <= EXPINT_MAXIT;i++) {
            fact *= -x/i;
            del = -fact / (i);
            ans += del;
            if (fabs(del) < fabs(ans) * EXPINT_EPS) return ans;
        }
    }
    return ans;
#else
    /* Approximation of the exponential integral due to Malah, 1996 */
    if (x < 0.1) {
        return -2.31 * log10(x) - 0.6;
    } else if (x > 1.0) {
        return pow(10, -0.52 * x - 0.26);
    } else return -1.544 * log10(x) + 0.166; 
#endif
}
