#pragma once

#include "route_recommender.hpp"
#include "trajectory_pattern.hpp"

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trajectory {

class DenseSignatureBitmap {
public:
    static constexpr std::size_t
        kUniverseSize = 125000;

    bool insert(std::size_t value) {
        validate(value);

        const std::size_t position =
            value - 1;

        if (bits_.test(position)) {
            return false;
        }

        bits_.set(position);
        ++value_count_;
        return true;
    }

    [[nodiscard]]
    bool contains(std::size_t value) const {
        validate(value);
        return bits_.test(value - 1);
    }

    [[nodiscard]]
    std::size_t intersection_count(
        const DenseSignatureBitmap& other
    ) const noexcept {
        return (
            bits_ & other.bits_
        ).count();
    }

    void union_with(
        const DenseSignatureBitmap& other
    ) noexcept {
        bits_ |= other.bits_;
        value_count_ = bits_.count();
    }

    [[nodiscard]]
    std::size_t size() const noexcept {
        return value_count_;
    }

    [[nodiscard]]
    constexpr std::size_t
    estimated_bitmap_bytes() const noexcept {
        return sizeof(bits_);
    }

private:
    static void validate(
        std::size_t value
    ) {
        if (
            value == 0 ||
            value > kUniverseSize
        ) {
            throw std::out_of_range(
                "Road-segment ID must be "
                "between 1 and 125000."
            );
        }
    }

    std::bitset<
        kUniverseSize
    > bits_;

    std::size_t value_count_ = 0;
};

class SignatureTree {
public:
    explicit SignatureTree(
        std::size_t node_capacity = 3
    )
        : node_capacity_(
              std::max<std::size_t>(
                  2,
                  node_capacity
              )
          ),
          root_(
              std::make_unique<Node>(true)
          ) {}

    void insert(
        const TrajectoryPattern& pattern
    ) {
        entries_.push_back(
            Entry{
                pattern,
                make_bitmap(
                    pattern.road_segments
                )
            }
        );

        const std::size_t entry_index =
            entries_.size() - 1;

        auto sibling =
            insert_recursive(
                *root_,
                entry_index
            );

        if (sibling) {
            auto new_root =
                std::make_unique<Node>(false);

            new_root->children.push_back(
                std::move(root_)
            );

            new_root->children.push_back(
                std::move(sibling)
            );

            rebuild_aggregate(*new_root);
            root_ = std::move(new_root);
        }
    }

    [[nodiscard]]
    Recommendation recommend(
        const std::vector<std::size_t>& query
    ) const {
        Recommendation best;

        const auto query_bitmap =
            make_bitmap(query);

        search_recursive(
            *root_,
            query,
            query_bitmap,
            best
        );

        return best;
    }

    [[nodiscard]]
    std::size_t pattern_count() const noexcept {
        return entries_.size();
    }

    [[nodiscard]]
    std::size_t node_count() const noexcept {
        return count_nodes(*root_);
    }

    [[nodiscard]]
    std::size_t depth() const noexcept {
        return tree_depth(*root_);
    }

    [[nodiscard]]
    std::size_t estimated_bitmap_bytes()
        const noexcept {
        std::size_t total =
            sum_node_bitmap_bytes(*root_);

        for (const auto& entry : entries_) {
            total +=
                entry.bitmap
                    .estimated_bitmap_bytes();
        }

        return total;
    }

private:
    struct Entry {
        TrajectoryPattern pattern;
        DenseSignatureBitmap bitmap;
    };

    struct Node {
        explicit Node(bool is_leaf)
            : leaf(is_leaf) {}

        bool leaf;
        DenseSignatureBitmap aggregate;

        std::vector<
            std::size_t
        > entry_indices;

        std::vector<
            std::unique_ptr<Node>
        > children;
    };

    static DenseSignatureBitmap make_bitmap(
        const std::vector<std::size_t>& segments
    ) {
        DenseSignatureBitmap bitmap;

        for (const auto segment : segments) {
            bitmap.insert(segment);
        }

        return bitmap;
    }

    [[nodiscard]]
    std::size_t missing_count(
        const DenseSignatureBitmap& aggregate,
        const Entry& entry
    ) const {
        std::size_t missing = 0;

        for (
            const auto segment :
            entry.pattern.road_segments
        ) {
            if (!aggregate.contains(segment)) {
                ++missing;
            }
        }

        return missing;
    }

    [[nodiscard]]
    std::size_t choose_child(
        const Node& node,
        const Entry& entry
    ) const {
        std::size_t best_index = 0;

        std::size_t best_missing =
            std::numeric_limits<
                std::size_t
            >::max();

        std::size_t best_area =
            std::numeric_limits<
                std::size_t
            >::max();

        for (
            std::size_t i = 0;
            i < node.children.size();
            ++i
        ) {
            const auto& child =
                *node.children[i];

            const std::size_t missing =
                missing_count(
                    child.aggregate,
                    entry
                );

            const std::size_t area =
                child.aggregate.size();

            if (
                missing < best_missing ||
                (
                    missing == best_missing &&
                    area < best_area
                )
            ) {
                best_index = i;
                best_missing = missing;
                best_area = area;
            }
        }

        return best_index;
    }

    std::unique_ptr<Node> insert_recursive(
        Node& node,
        std::size_t entry_index
    ) {
        if (node.leaf) {
            node.entry_indices.push_back(
                entry_index
            );

            node.aggregate.union_with(
                entries_[entry_index].bitmap
            );

            if (
                node.entry_indices.size() >
                node_capacity_
            ) {
                return split_node(node);
            }

            return nullptr;
        }

        const std::size_t child_index =
            choose_child(
                node,
                entries_[entry_index]
            );

        auto sibling =
            insert_recursive(
                *node.children[child_index],
                entry_index
            );

        if (sibling) {
            node.children.push_back(
                std::move(sibling)
            );
        }

        rebuild_aggregate(node);

        if (
            node.children.size() >
            node_capacity_
        ) {
            return split_node(node);
        }

        return nullptr;
    }

    std::unique_ptr<Node> split_node(
        Node& node
    ) {
        auto sibling =
            std::make_unique<Node>(
                node.leaf
            );

        if (node.leaf) {
            const std::size_t split_at =
                node.entry_indices.size() / 2;

            sibling->entry_indices.insert(
                sibling->entry_indices.end(),
                node.entry_indices.begin() +
                    static_cast<std::ptrdiff_t>(
                        split_at
                    ),
                node.entry_indices.end()
            );

            node.entry_indices.erase(
                node.entry_indices.begin() +
                    static_cast<std::ptrdiff_t>(
                        split_at
                    ),
                node.entry_indices.end()
            );
        } else {
            const std::size_t split_at =
                node.children.size() / 2;

            for (
                std::size_t i = split_at;
                i < node.children.size();
                ++i
            ) {
                sibling->children.push_back(
                    std::move(
                        node.children[i]
                    )
                );
            }

            node.children.erase(
                node.children.begin() +
                    static_cast<std::ptrdiff_t>(
                        split_at
                    ),
                node.children.end()
            );
        }

        rebuild_aggregate(node);
        rebuild_aggregate(*sibling);

        return sibling;
    }

    void rebuild_aggregate(Node& node) {
        node.aggregate =
            DenseSignatureBitmap{};

        if (node.leaf) {
            for (
                const auto index :
                node.entry_indices
            ) {
                node.aggregate.union_with(
                    entries_[index].bitmap
                );
            }
        } else {
            for (
                const auto& child :
                node.children
            ) {
                node.aggregate.union_with(
                    child->aggregate
                );
            }
        }
    }

    void search_recursive(
        const Node& node,
        const std::vector<std::size_t>& query,
        const DenseSignatureBitmap& query_bitmap,
        Recommendation& best
    ) const {
        if (
            node.aggregate.intersection_count(
                query_bitmap
            ) == 0
        ) {
            return;
        }

        if (node.leaf) {
            for (
                const auto index :
                node.entry_indices
            ) {
                consider_candidate(
                    entries_[index].pattern,
                    query,
                    best
                );
            }

            return;
        }

        std::vector<
            std::pair<
                std::size_t,
                const Node*
            >
        > ranked_children;

        ranked_children.reserve(
            node.children.size()
        );

        for (
            const auto& child :
            node.children
        ) {
            const std::size_t overlap =
                child
                    ->aggregate
                    .intersection_count(
                        query_bitmap
                    );

            if (overlap > 0) {
                ranked_children.emplace_back(
                    overlap,
                    child.get()
                );
            }
        }

        std::sort(
            ranked_children.begin(),
            ranked_children.end(),
            [](
                const auto& left,
                const auto& right
            ) {
                return left.first >
                    right.first;
            }
        );

        for (
            const auto& item :
            ranked_children
        ) {
            search_recursive(
                *item.second,
                query,
                query_bitmap,
                best
            );
        }
    }

    static std::size_t count_nodes(
        const Node& node
    ) noexcept {
        std::size_t total = 1;

        for (
            const auto& child :
            node.children
        ) {
            total += count_nodes(*child);
        }

        return total;
    }

    static std::size_t tree_depth(
        const Node& node
    ) noexcept {
        if (node.leaf) {
            return 1;
        }

        std::size_t maximum = 0;

        for (
            const auto& child :
            node.children
        ) {
            maximum = std::max(
                maximum,
                tree_depth(*child)
            );
        }

        return 1 + maximum;
    }

    static std::size_t
    sum_node_bitmap_bytes(
        const Node& node
    ) noexcept {
        std::size_t total =
            node.aggregate
                .estimated_bitmap_bytes();

        for (
            const auto& child :
            node.children
        ) {
            total +=
                sum_node_bitmap_bytes(
                    *child
                );
        }

        return total;
    }

    std::size_t node_capacity_;
    std::vector<Entry> entries_;
    std::unique_ptr<Node> root_;
};

}  // namespace trajectory
