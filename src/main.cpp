#include "frequent_path_tree.hpp"
#include "signature_tree.hpp"
#include "trajectory_pattern.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using trajectory::TrajectoryPattern;

std::vector<std::size_t> parse_segments(
    const std::string& text
) {
    std::vector<std::size_t> segments;
    std::istringstream stream(text);

    std::size_t value = 0;

    while (stream >> value) {
        segments.push_back(value);
    }

    if (segments.empty()) {
        throw std::runtime_error(
            "No road segments were found "
            "in a CSV row."
        );
    }

    return segments;
}

std::vector<TrajectoryPattern>
load_patterns(
    const std::string& path
) {
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error(
            "Unable to open pattern file: " +
            path
        );
    }

    std::vector<TrajectoryPattern> patterns;
    std::string line;

    std::getline(input, line);

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        if (
            !line.empty() &&
            line.back() == '\r'
        ) {
            line.pop_back();
        }

        std::stringstream row(line);

        std::string id;
        std::string support_text;
        std::string segments_text;

        std::getline(row, id, ',');
        std::getline(
            row,
            support_text,
            ','
        );
        std::getline(
            row,
            segments_text
        );

        patterns.emplace_back(
            id,
            std::stod(support_text),
            parse_segments(
                segments_text
            )
        );
    }

    return patterns;
}

struct Query {
    std::string id;

    std::vector<
        std::size_t
    > road_segments;
};

Query load_query(
    const std::string& path
) {
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error(
            "Unable to open query file: " +
            path
        );
    }

    std::string line;

    std::getline(input, line);

    if (!std::getline(input, line)) {
        throw std::runtime_error(
            "The query file contains "
            "no query row."
        );
    }

    if (
        !line.empty() &&
        line.back() == '\r'
    ) {
        line.pop_back();
    }

    std::stringstream row(line);

    Query query;
    std::string segments_text;

    std::getline(row, query.id, ',');
    std::getline(row, segments_text);

    query.road_segments =
        parse_segments(segments_text);

    return query;
}

void print_segments(
    const std::vector<std::size_t>& segments
) {
    for (
        std::size_t i = 0;
        i < segments.size();
        ++i
    ) {
        if (i > 0) {
            std::cout << ' ';
        }

        std::cout << segments[i];
    }
}

void print_recommendation(
    const std::string& label,
    const trajectory::Recommendation&
        recommendation
) {
    std::cout
        << label
        << " recommendation: ";

    if (!recommendation.found) {
        std::cout << "none\n";
        return;
    }

    std::cout
        << recommendation.pattern_id
        << " | similarity="
        << std::fixed
        << std::setprecision(4)
        << recommendation.similarity
        << " | support="
        << std::setprecision(2)
        << recommendation.support
        << " | route=";

    print_segments(
        recommendation.road_segments
    );

    std::cout << '\n';
}

}  // namespace

int main(
    int argc,
    char* argv[]
) {
    try {
        const std::string pattern_path =
            argc > 1
                ? argv[1]
                : "sample_data/"
                  "trajectory_patterns.csv";

        const std::string query_path =
            argc > 2
                ? argv[2]
                : "sample_data/"
                  "query_trajectory.csv";

        const auto patterns =
            load_patterns(pattern_path);

        const auto query =
            load_query(query_path);

        trajectory::FrequentPathTree fpt(3);
        trajectory::SignatureTree
            signature_tree(3);

        const auto fpt_start =
            std::chrono::
                steady_clock::now();

        for (
            const auto& pattern :
            patterns
        ) {
            fpt.insert(pattern);
        }

        const auto fpt_end =
            std::chrono::
                steady_clock::now();

        const auto sg_start =
            std::chrono::
                steady_clock::now();

        for (
            const auto& pattern :
            patterns
        ) {
            signature_tree.insert(pattern);
        }

        const auto sg_end =
            std::chrono::
                steady_clock::now();

        const auto fpt_query_start =
            std::chrono::
                steady_clock::now();

        const auto fpt_result =
            fpt.recommend(
                query.road_segments
            );

        const auto fpt_query_end =
            std::chrono::
                steady_clock::now();

        const auto sg_query_start =
            std::chrono::
                steady_clock::now();

        const auto sg_result =
            signature_tree.recommend(
                query.road_segments
            );

        const auto sg_query_end =
            std::chrono::
                steady_clock::now();

        const auto fpt_build_us =
            std::chrono::duration_cast<
                std::chrono::microseconds
            >(
                fpt_end -
                fpt_start
            ).count();

        const auto sg_build_us =
            std::chrono::duration_cast<
                std::chrono::microseconds
            >(
                sg_end -
                sg_start
            ).count();

        const auto fpt_query_us =
            std::chrono::duration_cast<
                std::chrono::microseconds
            >(
                fpt_query_end -
                fpt_query_start
            ).count();

        const auto sg_query_us =
            std::chrono::duration_cast<
                std::chrono::microseconds
            >(
                sg_query_end -
                sg_query_start
            ).count();

        std::cout
            << "Trajectory Pattern "
               "Index Demo\n"
            << "================"
               "=============\n"
            << "Loaded patterns: "
            << patterns.size()
            << '\n'
            << "Query "
            << query.id
            << ": ";

        print_segments(
            query.road_segments
        );

        std::cout << "\n\n";

        print_recommendation(
            "FPT",
            fpt_result
        );

        print_recommendation(
            "Signature Tree",
            sg_result
        );

        std::cout
            << "\nIndex summary\n"
            << "-------------\n"
            << "FPT nodes/depth: "
            << fpt.node_count()
            << '/'
            << fpt.depth()
            << '\n'
            << "Signature Tree "
               "nodes/depth: "
            << signature_tree.node_count()
            << '/'
            << signature_tree.depth()
            << '\n'
            << "FPT estimated "
               "bitmap bytes: "
            << fpt
                .estimated_bitmap_bytes()
            << '\n'
            << "Signature Tree estimated "
               "bitmap bytes: "
            << signature_tree
                .estimated_bitmap_bytes()
            << '\n'
            << "FPT build/query time "
               "(microseconds): "
            << fpt_build_us
            << '/'
            << fpt_query_us
            << '\n'
            << "Signature Tree build/query "
               "time (microseconds): "
            << sg_build_us
            << '/'
            << sg_query_us
            << '\n';

        if (
            !fpt_result.found ||
            !sg_result.found ||
            fpt_result.pattern_id !=
                sg_result.pattern_id
        ) {
            throw std::runtime_error(
                "The two indexes returned "
                "inconsistent recommendations."
            );
        }

        std::cout
            << "\nAll checks passed.\n";

        return 0;
    } catch (
        const std::exception& error
    ) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }
}
