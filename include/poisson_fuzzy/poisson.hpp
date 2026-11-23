#pragma once
#include "poisson_fuzzy/poisson.hpp"
#include <vector>

namespace pf {

struct Interval {
    double lambda  = 0.0;
    int    A       = 0;
    int    B       = 0;
    double maxProb = 0.0;
    int    width() const noexcept { return B - A; }
};

struct ProcessResult {
    std::vector<Interval> allIntervals;
    std::vector<Interval> validIntervals;   // только широкие (width > 1)
    std::vector<double>   lambdas;
    double                thresholdY = 0.0; // постоянный порог по оси Y
};

struct Config {
    int    count          = 200;
    double lambdaMin      = 1.0;
    double lambdaMax      = 100.0;
    double thresholdRatio = 0.10;   // доля от глобального максимума PMF
    int    K              = 3;
    int    maxIter        = 100;
};

double poissonPMF(int x, double lambda) noexcept;
std::vector<double> buildProfile(double lambda);
double globalMaxPMF(const std::vector<double>& lambdas) noexcept;
ProcessResult processData(const Config& cfg);

} // namespace pf