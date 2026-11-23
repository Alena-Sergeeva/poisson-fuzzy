#pragma once
#include <utility>

namespace pf {

/// Треугольное нечёткое число (a, b, c).
struct FuzzyTriangular {
    double a = 0.0;  // левая граница
    double b = 0.0;  // мода (центр)
    double c = 0.0;  // правая граница

    double center() const noexcept { return (a + b + c) / 3.0; }
    double width()  const noexcept { return c - a; }
};

FuzzyTriangular fuzzyAdd(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept;
FuzzyTriangular fuzzySub(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept;
FuzzyTriangular fuzzyMul(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept;
FuzzyTriangular fuzzyDiv(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept;
int             fuzzyCompare(const FuzzyTriangular& x, const FuzzyTriangular& y) noexcept;
FuzzyTriangular fuzzyFromInterval(double A, double B) noexcept;

} // namespace pf