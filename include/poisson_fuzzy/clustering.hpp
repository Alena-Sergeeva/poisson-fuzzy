#pragma once
#include "poisson.hpp"
#include <vector>
#include <string>

namespace pf {

struct CurveFeatures {
    double lambda  = 0.0;
    int    A = 0, B = 0, W = 0;
    double C = 0.0, M = 0.0, R = 0.0;
    int    cluster = -1;
};

struct ClusterSummary {
    int         id = 0;
    int         count = 0;
    double      meanCenter = 0.0;
    double      meanWidth = 0.0;
    double      meanMass = 0.0;
    double      meanRelWidth = 0.0;
    std::string label;
};

struct ClusterResult {
    std::vector<CurveFeatures>   items;
    std::vector<ClusterSummary>  summaries;
    int                          iterations = 0;
};

ClusterResult clusterIntervals(const std::vector<Interval>& intervals,
                               int K, int maxIter);

} // namespace pf