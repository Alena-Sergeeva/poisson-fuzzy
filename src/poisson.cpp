#include "poisson_fuzzy/poisson.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace pf {

double poissonPMF(int x, double lambda) noexcept
{
    if (x < 0 || lambda <= 0) return 0.0;
    return std::exp(x * std::log(lambda) - lambda - std::lgamma(x + 1.0));
}

std::vector<double> buildProfile(double lambda)
{
    int maxX = static_cast<int>(lambda + 5.0 * std::sqrt(lambda) + 10);
    if (maxX < 1) maxX = 1;
    std::vector<double> p(maxX);
    for (int x = 0; x < maxX; ++x) p[x] = poissonPMF(x, lambda);
    return p;
}

double globalMaxPMF(const std::vector<double>& lambdas) noexcept
{
    double m = 0.0;
    for (double lam : lambdas) {
        int maxX = static_cast<int>(lam + 5.0 * std::sqrt(lam) + 10);
        for (int x = 0; x < maxX; ++x) {
            const double p = poissonPMF(x, lam);
            if (p > m) m = p;
        }
    }
    return m;
}

ProcessResult processData(const Config& cfg)
{
    ProcessResult r;
    r.lambdas.reserve(cfg.count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(cfg.lambdaMin, cfg.lambdaMax);
    for (int i = 0; i < cfg.count; ++i) r.lambdas.push_back(dis(gen));
    std::sort(r.lambdas.begin(), r.lambdas.end());

    r.thresholdY = cfg.thresholdRatio * globalMaxPMF(r.lambdas);

    for (double lam : r.lambdas) {
        int maxX = static_cast<int>(lam + 5.0 * std::sqrt(lam) + 10);
        int A = -1, B = -1;
        for (int x = 0; x < maxX; ++x) {
            if (poissonPMF(x, lam) >= r.thresholdY) {
                if (A == -1) A = x;
                B = x;
            }
        }
        r.allIntervals.push_back({lam, A, B, r.thresholdY});
        if (A >= 0 && (B - A) > 1)
            r.validIntervals.push_back({lam, A, B, r.thresholdY});
    }
    return r;
}

} // namespace pf