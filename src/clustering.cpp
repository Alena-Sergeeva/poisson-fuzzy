#include "poisson_fuzzy/clustering.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>

namespace pf {

ClusterResult clusterIntervals(const std::vector<Interval>& intervals,
                               int K, int maxIter)
{
    ClusterResult out;
    const int N = static_cast<int>(intervals.size());
    if (N == 0 || K <= 0) return out;
    if (K > N) K = N;

    for (const auto& inv : intervals) {
        auto probs = buildProfile(inv.lambda);
        int A = std::max(0, inv.A);
        int B = std::min(inv.B, (int)probs.size() - 1);
        if (B < A) B = A;
        double mass = 0.0;
        for (int x = A; x <= B; ++x) mass += probs[x];

        CurveFeatures f;
        f.lambda = inv.lambda; f.A = A; f.B = B;
        f.W = B - A; f.C = 0.5 * (A + B); f.M = mass;
        f.R = f.W / std::max(f.C, 1.0);
        out.items.push_back(f);
    }

    std::vector<std::array<double,3>> feat(N), z(N);
    for (int i = 0; i < N; ++i) {
        feat[i][0] = out.items[i].C;
        feat[i][1] = out.items[i].R;
        feat[i][2] = out.items[i].M;
    }
    std::array<double,3> mean{0,0,0}, sd{0,0,0};
    for (int i = 0; i < N; ++i) for (int d = 0; d < 3; ++d) mean[d] += feat[i][d];
    for (int d = 0; d < 3; ++d) mean[d] /= N;
    for (int i = 0; i < N; ++i) for (int d = 0; d < 3; ++d) {
        const double df = feat[i][d] - mean[d];
        sd[d] += df * df;
    }
    for (int d = 0; d < 3; ++d) {
        sd[d] = std::sqrt(sd[d] / N);
        if (sd[d] < 1e-9) sd[d] = 1.0;
    }
    for (int i = 0; i < N; ++i) for (int d = 0; d < 3; ++d)
        z[i][d] = (feat[i][d] - mean[d]) / sd[d];

    std::vector<std::array<double,3>> cent(K);
    for (int k = 0; k < K; ++k) {
        int idx = static_cast<int>((k + 0.5) * N / K);
        if (idx >= N) idx = N - 1;
        cent[k] = z[idx];
    }

    std::vector<int> labels(N, -1);
    for (int iter = 0; iter < maxIter; ++iter) {
        bool changed = false;
        for (int i = 0; i < N; ++i) {
            int best = 0; double bestD = std::numeric_limits<double>::max();
            for (int k = 0; k < K; ++k) {
                double d = 0.0;
                for (int dd = 0; dd < 3; ++dd) {
                    const double t = z[i][dd] - cent[k][dd];
                    d += t * t;
                }
                if (d < bestD) { bestD = d; best = k; }
            }
            if (labels[i] != best) { labels[i] = best; changed = true; }
        }
        out.iterations = iter + 1;
        if (!changed) break;

        std::array<double,3> zero{0,0,0};
        std::vector<std::array<double,3>> sums(K, zero);
        std::vector<int> cnt(K, 0);
        for (int i = 0; i < N; ++i) {
            for (int d = 0; d < 3; ++d) sums[labels[i]][d] += z[i][d];
            cnt[labels[i]]++;
        }
        for (int k = 0; k < K; ++k)
            if (cnt[k] > 0)
                for (int d = 0; d < 3; ++d) cent[k][d] = sums[k][d] / cnt[k];
    }
    for (int i = 0; i < N; ++i) out.items[i].cluster = labels[i];

    std::vector<ClusterSummary> raw(K);
    for (int k = 0; k < K; ++k) raw[k].id = k;
    for (int i = 0; i < N; ++i) {
        int k = labels[i];
        raw[k].count++;
        raw[k].meanCenter   += out.items[i].C;
        raw[k].meanWidth    += out.items[i].W;
        raw[k].meanMass     += out.items[i].M;
        raw[k].meanRelWidth += out.items[i].R;
    }
    std::vector<ClusterSummary> ne;
    for (int k = 0; k < K; ++k) {
        if (raw[k].count == 0) continue;
        raw[k].meanCenter   /= raw[k].count;
        raw[k].meanWidth    /= raw[k].count;
        raw[k].meanMass     /= raw[k].count;
        raw[k].meanRelWidth /= raw[k].count;
        ne.push_back(raw[k]);
    }

    std::sort(ne.begin(), ne.end(),
              [](const ClusterSummary& a, const ClusterSummary& b) {
                  auto fa = fuzzyFromInterval(a.meanCenter - a.meanWidth * 0.5,
                                              a.meanCenter + a.meanWidth * 0.5);
                  auto fb = fuzzyFromInterval(b.meanCenter - b.meanWidth * 0.5,
                                              b.meanCenter + b.meanWidth * 0.5);
                  return fuzzyCompare(fa, fb) < 0;
              });

    std::vector<int> oldToNew(K, -1);
    for (int nid = 0; nid < (int)ne.size(); ++nid) {
        oldToNew[ne[nid].id] = nid;
        ne[nid].id = nid;
    }
    for (auto& it : out.items)
        if (it.cluster >= 0 && it.cluster < K) it.cluster = oldToNew[it.cluster];

    const int S = (int)ne.size();
    for (int p = 0; p < S; ++p) {
        std::string intensity =
            (S == 1) ? "Средняя интенсивность"
          : (p == 0) ? "Низкая интенсивность"
          : (p == S - 1) ? "Высокая интенсивность"
          : "Средняя интенсивность";

        std::string stability =
            (ne[p].meanRelWidth > 0.6) ? "нестабильный поток"
          : (ne[p].meanRelWidth < 0.25) ? "стабильный поток"
          : "умеренная изменчивость";

        std::ostringstream oss;
        oss << intensity << ", " << stability
            << ", покрытие " << (ne[p].meanMass * 100.0) << "%";
        ne[p].label = oss.str();
    }
    out.summaries = ne;
    return out;
}

} // namespace pf