#include "poisson_fuzzy/fuzzy.hpp"
#include <cmath>
#include <algorithm>

namespace pf {

FuzzyTriangular fuzzyAdd(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept
{ return {x.a + y.a, x.b + y.b, x.c + y.c}; }

FuzzyTriangular fuzzySub(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept
{ return {x.a - y.c, x.b - y.b, x.c - y.a}; }

FuzzyTriangular fuzzyMul(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept
{ return {x.a * y.a, x.b * y.b, x.c * y.c}; }

FuzzyTriangular fuzzyDiv(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept
{
    if (std::fabs(y.a) < 1e-9 || std::fabs(y.b) < 1e-9 || std::fabs(y.c) < 1e-9)
        return {0, 0, 0};
    return {x.a / y.c, x.b / y.b, x.c / y.a};
}

int fuzzyCompare(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept
{
    const double cx = x.center(), cy = y.center();
    if (cx < cy - 1e-9) return -1;
    if (cx > cy + 1e-9) return  1;
    return 0;
}

FuzzyTriangular fuzzyFromInterval(double A, double B) noexcept
{
    if (A > B) std::swap(A, B);
    return {A, 0.5 * (A + B), B};
}

} // namespace pf