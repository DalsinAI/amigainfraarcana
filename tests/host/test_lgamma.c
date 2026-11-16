/* Host test of amiga/lgamma.c (ia_lgamma) against the host's lgamma().
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <math.h>
#include <stdio.h>

double ia_lgamma(double x);

int main(void)
{
	static const double xs[] = {
		1e-6, 0.1, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 7.25, 10.0, 33.3, 100.0,
		171.5, 1000.0, 1e5, -0.5, -1.5, -2.25, -10.75,
	};
	int fails = 0;
	for (unsigned i = 0; i < sizeof xs / sizeof xs[0]; i++) {
		const double want = lgamma(xs[i]);
		const double got = ia_lgamma(xs[i]);
		const double err = fabs(got - want) / (fabs(want) > 1.0 ? fabs(want) : 1.0);
		const int ok = err < 1e-12;
		printf("%s lgamma(%g) = %.15g (host %.15g)\n", ok ? "PASS" : "FAIL", xs[i], got, want);
		fails += !ok;
	}
	const int pole = isinf(ia_lgamma(0.0)) && isinf(ia_lgamma(-3.0));
	printf("%s poles at 0 and -3\n", pole ? "PASS" : "FAIL");
	fails += !pole;
	printf("%s (%d failed)\n", fails ? "FAIL" : "PASS", fails);
	return fails != 0;
}
