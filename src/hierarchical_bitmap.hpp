#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

namespace trajectory {

class HierarchicalBitmap {
public:
    static constexpr std::size_t kBranchFactor = 50;
    static constexpr std::size_t kUniverseSize =
        kBranchFactor * kBranchFactor * kBranchFactor;

    bool insert(std::size_t value) {
        const auto [high, middle, low] = locate(value);

        if (!middle_nodes_[high]) {
            middle_nodes_[high] = std::make_unique<MiddleNode>();
            root_bitmap_.set(high);
        }

        auto& middle_node = middle_nodes_[high];

        if (!middle_node->leaf_nodes[middle]) {
            middle_node->leaf_nodes[middle] =
                std::make_unique<LeafNode>();
            middle_node->bitmap.set(middle);
        }

        auto& leaf = middle_node->leaf_nodes[middle];

        if (leaf->bitmap.test(low)) {
            return false;
        }

        leaf->bitmap.set(low);
        ++value_count_;
        return true;
    }

    [[nodiscard]]
    bool contains(std::size_t value) const {
        const auto [high, middle, low] = locate(value);

        const auto& middle_node = middle_nodes_[high];

        if (!middle_node) {
            return false;
        }

        const auto& leaf =
            middle_node->leaf_nodes[middle];

        return leaf && leaf->bitmap.test(low);
    }

    [[nodiscard]]
    std::size_t intersection_count(
        const HierarchicalBitmap& other
    ) const noexcept {
        std::size_t total = 0;

        const auto root_overlap =
            root_bitmap_ & other.root_bitmap_;

        for (std::size_t high = 0;
             high < kBranchFactor;
             ++high) {
            if (!root_overlap.test(high)) {
                continue;
            }

            const auto& left_middle =
                middle_nodes_[high];

            const auto& right_middle =
                other.middle_nodes_[high];

            const auto middle_overlap =
                left_middle->bitmap &
                right_middle->bitmap;

            for (std::size_t middle = 0;
                 middle < kBranchFactor;
                 ++middle) {
                if (!middle_overlap.test(middle)) {
                    continue;
                }

                total += (
                    left_middle
                        ->leaf_nodes[middle]
                        ->bitmap
                    &
                    right_middle
                        ->leaf_nodes[middle]
                        ->bitmap
                ).count();
            }
        }

        return total;
    }

    void union_with(
        const HierarchicalBitmap& other
    ) {
        for (const auto value : other.values()) {
            insert(value);
        }
    }

    [[nodiscard]]
    std::vector<std::size_t> values() const {
        std::vector<std::size_t> result;
        result.reserve(value_count_);

        for (std::size_t high = 0;
             high < kBranchFactor;
             ++high) {
            const auto& middle_node =
                middle_nodes_[high];

            if (!middle_node) {
                continue;
            }

            for (std::size_t middle = 0;
                 middle < kBranchFactor;
                 ++middle) {
                const auto& leaf =
                    middle_node->leaf_nodes[middle];

                if (!leaf) {
                    continue;
                }

                for (std::size_t low = 0;
                     low < kBranchFactor;
                     ++low) {
                    if (leaf->bitmap.test(low)) {
                        result.push_back(
                            high *
                                kBranchFactor *
                                kBranchFactor
                            +
                            middle *
                                kBranchFactor
                            +
                            low
                            +
                            1
                        );
                    }
                }
            }
        }

        return result;
    }

    [[nodiscard]]
    std::size_t size() const noexcept {
        return value_count_;
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return value_count_ == 0;
    }

    [[nodiscard]]
    std::size_t estimated_bitmap_bytes() const noexcept {
        std::size_t bytes =
            sizeof(root_bitmap_);

        for (const auto& middle_node :
             middle_nodes_) {
            if (!middle_node) {
                continue;
            }

            bytes += sizeof(
                middle_node->bitmap
            );

            for (const auto& leaf :
                 middle_node->leaf_nodes) {
                if (leaf) {
                    bytes += sizeof(
                        leaf->bitmap
                    );
                }
            }
        }

        return bytes;
    }

private:
    struct LeafNode {
        std::bitset<kBranchFactor> bitmap;
    };

    struct MiddleNode {
        std::bitset<kBranchFactor> bitmap;

        std::array<
            std::unique_ptr<LeafNode>,
            kBranchFactor
        > leaf_nodes{};
    };

    struct Position {
        std::size_t high;
        std::size_t middle;
        std::size_t low;
    };

    static Position locate(
        std::size_t value
    ) {
        if (value == 0 ||
            value > kUniverseSize) {
            throw std::out_of_range(
                "Road-segment ID must be "
                "between 1 and 125000."
            );
        }

        const std::size_t normalized =
            value - 1;

        const std::size_t high =
            normalized /
            (
                kBranchFactor *
                kBranchFactor
            );

        const std::size_t remainder =
            normalized %
            (
                kBranchFactor *
                kBranchFactor
            );

        const std::size_t middle =
            remainder / kBranchFactor;

        const std::size_t low =
            remainder % kBranchFactor;

        return {
            high,
            middle,
            low
        };
    }

    std::bitset<
        kBranchFactor
    > root_bitmap_;

    std::array<
        std::unique_ptr<MiddleNode>,
        kBranchFactor
    > middle_nodes_{};

    std::size_t value_count_ = 0;
};

}  // namespace trajectory
