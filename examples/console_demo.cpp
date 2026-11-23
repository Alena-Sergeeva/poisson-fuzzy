#include <poisson_fuzzy/poisson_fuzzy.hpp>
#include <iostream>
#include <fstream>

int main()
{
    pf::Config cfg;
    cfg.count = 200;
    cfg.lambdaMin = 1.0;
    cfg.lambdaMax = 100.0;
    cfg.thresholdRatio = 0.10;
    cfg.K = 3;
    cfg.maxIter = 100;

    auto r = pf::processData(cfg);
    std::cout << "Generated: " << r.lambdas.size() << "\n"
              << "Y-threshold: " << r.thresholdY << "\n"
              << "Wide intervals: " << r.validIntervals.size() << "\n";

    auto cl = pf::clusterIntervals(r.validIntervals, cfg.K, cfg.maxIter);
    std::cout << "Clusters: " << cl.summaries.size()
              << " (iterations: " << cl.iterations << ")\n";
    for (const auto& s : cl.summaries) {
        std::cout << "  #" << s.id << " n=" << s.count
                  << " C=" << s.meanCenter
                  << " W=" << s.meanWidth
                  << " R=" << s.meanRelWidth
                  << " | " << s.label << "\n";
    }

    pf::FuzzyTriangular a{1, 2, 3}, b{4, 5, 6};
    auto s = pf::fuzzyAdd(a, b);
    std::cout << "a+b = (" << s.a << ", " << s.b << ", " << s.c << ")\n";

    std::ofstream log("poisson_log.txt", std::ios::app);
    log << "Run N=" << r.lambdas.size()
        << " Y=" << r.thresholdY << "\n";
    return 0;
}