#pragma once

#include "trajectory_pattern.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace trajectory {

struct Recommendation {
    bool found = false;
    std::string pattern_id;
    double similarity = 0.0;
    double support = 0.0;

    std::vector<
        std::size_t
    > road_segments;
};

inline double weighted_query_coverage(
    const std::vector<std::size_t>& query,
    const std::vector<std::size_t>& candidate
) {
    long double matched_weight = 0.0L;
    long double total_weight = 0.0L;
    long double weight = 1.0L;

    for (const auto segment : query) {
        total_weight += weight;

        if (
            std::find(
                candidate.begin(),
                candidate.end(),
                segment
            )
            != candidate.end()
        ) {
            matched_weight += weight;
        }

        weight *= 10.0L;
    }

    if (total_weight == 0.0L) {
        return 0.0;
    }

    return static_cast<double>(
        matched_weight / total_weight
    );
}

inline void consider_candidate(
    const TrajectoryPattern& pattern,
    const std::vector<std::size_t>& query,
    Recommendation& best
) {
    const double similarity =
        weighted_query_coverage(
            query,
            pattern.road_segments
        );

    if (similarity <= 0.0) {
        return;
    }

    constexpr double kTolerance = 1e-12;

    const bool better_similarity =
        !best.found ||
        similarity >
            best.similarity +
            kTolerance;

    const bool same_similarity =
        best.found &&
        std::abs(
            similarity -
            best.similarity
        ) <= kTolerance;

    const bool better_support =
        same_similarity &&
        pattern.support > best.support;

    if (
        better_similarity ||
        better_support
    ) {
        best.found = true;
        best.pattern_id = pattern.id;
        best.similarity = similarity;
        best.support = pattern.support;
        best.road_segments =
            pattern.road_segments;
    }
}

}  // namespace trajectory
