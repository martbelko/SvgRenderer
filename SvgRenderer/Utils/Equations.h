#pragma once

#include <cassert>
#include <complex>
#include <cmath>

#define M_PI 3.14159265358979323846

namespace SvgRenderer::Equations {

	struct CubicRoot
	{
		float x1, x2, x3;
	};

	// ax + b = 0
	static CubicRoot SolveEq(float a, float b)
	{
		assert(a != 0.0f);
		return CubicRoot{
			.x1 = -b / a,
			.x2 = std::numeric_limits<float>::max(),
			.x3 = std::numeric_limits<float>::max()
		};
	}

	static CubicRoot SolveQuadratic(float a, float b, float c)
	{
		if (a == 0.0f)
		{
			return SolveEq(b, c);
		}

		float disc = b * b - 4.0f * a * c;
		if (disc < 0.0f)
		{
			return CubicRoot{
				.x1 = std::numeric_limits<float>::max(),
				.x2 = std::numeric_limits<float>::max(),
				.x3 = std::numeric_limits<float>::max()
			};
		}

		if (disc == 0.0f)
		{
			float x = -b / (2.0f * a);
			return CubicRoot{
				.x1 = x,
				.x2 = std::numeric_limits<float>::max(),
				.x3 = std::numeric_limits<float>::max()
			};
		}

		float sqrtdisc = std::sqrt(disc);
		float x1 = (-b + sqrtdisc) / (2.0f * a);
		float x2 = (-b - sqrtdisc) / (2.0f * a);
		return CubicRoot{
			.x1 = x1,
			.x2 = x2,
			.x3 = std::numeric_limits<float>::max()
		};
	}

	static CubicRoot SolveCubic(float a, float b, float c, float d)
	{
		if (a == 0.0f)
		{
			return SolveQuadratic(b, c, d);
		}

		// TODO: Add automatic conversion to quadratic equation and solve it, not assert
		// assert(d != 0.0f);

		float x1_real, x2_real, x3_real;
		float x2_imag, x3_imag;
		auto makeReturn = [&]() -> CubicRoot
		{
			return CubicRoot{
				.x1 = x1_real,
				.x2 = x2_imag != 0 ? std::numeric_limits<float>::max() : x2_real,
				.x3 = x3_imag != 0 ? std::numeric_limits<float>::max() : x3_real
			};
		};

		b /= a;
		c /= a;
		d /= a;

		float disc, q, r, dum1, s, t, term1, r13;
		q = (3.0 * c - (b * b)) / 9.0;
		r = -(27.0 * d) + b * (9.0 * c - 2.0 * (b * b));
		r /= 54.0;
		disc = q * q * q + r * r;
		term1 = (b / 3.0);

		if (disc > 0)   // One root real, two are complex
		{
			s = r + std::sqrt(disc);
			s = s < 0 ? -std::cbrt(-s) : std::cbrt(s);
			t = r - std::sqrt(disc);
			t = t < 0 ? -std::cbrt(-t) : std::cbrt(t);
			x1_real = -term1 + s + t;
			term1 += (s + t) / 2.0;
			x3_real = x2_real = -term1;
			term1 = std::sqrt(3.0) * (-t + s) / 2;
			x2_imag = term1;
			x3_imag = -term1;
		}
		// The remaining options are all real
		else if (disc == 0)  // All roots real, at least two are equal.
		{
			x3_imag = x2_imag = 0;
			r13 = r < 0 ? -std::cbrt(-r) : std::cbrt(r);
			x1_real = -term1 + 2.0 * r13;
			x3_real = x2_real = -(r13 + term1);
		}
		// Only option left is that all roots are real and unequal (to get here, q < 0)
		else
		{
			x3_imag = x2_imag = 0;
			q = -q;
			dum1 = q * q * q;
			dum1 = std::acos(r / std::sqrt(dum1));
			r13 = 2.0 * std::sqrt(q);
			x1_real = -term1 + r13 * std::cos(dum1 / 3.0);
			x2_real = -term1 + r13 * std::cos((dum1 + 2.0 * M_PI) / 3.0f);
			x3_real = -term1 + r13 * std::cos((dum1 + 4.0 * M_PI) / 3.0f);
		}

		return makeReturn();
	}

}
