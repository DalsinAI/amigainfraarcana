/* lgamma() for libnix, which has none: libstdc++'s
 * std::binomial_distribution (src/random.cpp) calls it. ln|Gamma(x)| by
 * the Lanczos approximation (g = 7, nine terms, about 15 digits), with
 * Euler's reflection for x < 0.5. tests/host/test_lgamma.c checks it
 * against the host C library.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <math.h>

static const double lanczos_g = 7.0;
static const double lanczos_c[9] = {
	0.99999999999980993,
	676.5203681218851,
	-1259.1392167224028,
	771.32342877765313,
	-176.61502916214059,
	12.507343278686905,
	-0.13857109526572012,
	9.9843695780195716e-6,
	1.5056327351493116e-7,
};

#define LG_PI 3.14159265358979323846
#define LG_LN_SQRT_2PI 0.91893853320467274178

double ia_lgamma(double x)
{
	if (isnan(x))
		return x;
	if (isinf(x))
		return HUGE_VAL;
	if (x <= 0.0 && floor(x) == x)
		return HUGE_VAL;		/* a pole */
	if (x < 0.5) {
		/* Gamma(x) Gamma(1 - x) = pi / sin(pi x) */
		const double s = sin(LG_PI * x);
		return log(LG_PI / fabs(s)) - ia_lgamma(1.0 - x);
	}
	x -= 1.0;
	double a = lanczos_c[0];
	const double t = x + lanczos_g + 0.5;
	for (int i = 1; i < 9; i++)
		a += lanczos_c[i] / (x + (double)i);
	return LG_LN_SQRT_2PI + (x + 0.5) * log(t) - t + log(a);
}

#ifdef __amigaos__
double lgamma(double x)
{
	return ia_lgamma(x);
}
#endif
