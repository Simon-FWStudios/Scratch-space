#pragma once
#include "FlatRowHelpers.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

class FlatRowPool {
public:
    static FlatRowPool& Get();

    std::unique_ptr<FlatRowBlock, std::function<void(FlatRowBlock*)>>
    Acquire(const std::vector<int>& columnSizes, int rowCount);

    void Release(std::unique_ptr<FlatRowBlock> block);

private:
    struct LayoutKey {
        std::vector<int> columnSizes;
        int rowCount;

        bool operator==(const LayoutKey& other) const;
    };

    struct LayoutKeyHash {
        size_t operator()(const LayoutKey& key) const;
    };

    using BlockList = std::vector<std::unique_ptr<FlatRowBlock>>;
    std::unordered_map<LayoutKey, BlockList, LayoutKeyHash> m_blocks;

    FlatRowPool() = default;
    ~FlatRowPool() = default;

    FlatRowPool(const FlatRowPool&) = delete;
    FlatRowPool& operator=(const FlatRowPool&) = delete;
};
