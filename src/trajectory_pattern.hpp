#pragma once

#include "hierarchical_bitmap.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace trajectory {

struct TrajectoryPattern {
    std::string id;
    double support = 0.0;
    std::vector<std::size_t> road_segments;

    TrajectoryPattern(
        std::string pattern_id,
        double pattern_support,
        std::vector<std::size_t> segments
    )
        : id(std::move(pattern_id)),
          support(pattern_support),
          road_segments(std::move(segments)) {
        validate();
    }

private:
    void validate() const {
        if (id.empty()) {
            throw std::invalid_argument(
                "Pattern ID must not be empty."
            );
        }

        if (road_segments.empty()) {
            throw std::invalid_argument(
                "A trajectory pattern must "
                "contain road segments."
            );
        }

        if (!std::isfinite(support) ||
            support < 0.0 ||
            support > 1.0) {
            throw std::invalid_argument(
                "Support must be a finite value "
                "between 0 and 1."
            );
        }

        for (const auto segment :
             road_segments) {
            if (
                segment == 0 ||
                segment >
                    HierarchicalBitmap::
                        kUniverseSize
            ) {
                throw std::out_of_range(
                    "Road-segment ID must be "
                    "between 1 and 125000."
                );
            }
        }
    }
};

}  // namespace trajectory
