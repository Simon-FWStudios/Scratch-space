#include "FlatRowPool.h"
#include <functional>

FlatRowPool& FlatRowPool::Get() {
    static thread_local FlatRowPool instance;
    return instance;
}

bool FlatRowPool::LayoutKey::operator==(const LayoutKey& other) const {
    return rowCount == other.rowCount && columnSizes == other.columnSizes;
}

size_t FlatRowPool::LayoutKeyHash::operator()(const LayoutKey& key) const {
    size_t h = std::hash<int>()(key.rowCount);
    for (int s : key.columnSizes) {
        h ^= std::hash<int>()(s) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

std::unique_ptr<FlatRowBlock, std::function<void(FlatRowBlock*)>>
FlatRowPool::Acquire(const std::vector<int>& columnSizes, int rowCount) {
    LayoutKey key{columnSizes, rowCount};
    auto& list = m_blocks[key];

    if (!list.empty()) {
        auto block = std::move(list.back());
        list.pop_back();
        return {block.release(), [this](FlatRowBlock* b) { this->Release(std::unique_ptr<FlatRowBlock>(b)); }};
    }

    auto newBlock = std::make_unique<FlatRowBlock>(rowCount, columnSizes);
    return {newBlock.release(), [this](FlatRowBlock* b) { this->Release(std::unique_ptr<FlatRowBlock>(b)); }};
}

void FlatRowPool::Release(std::unique_ptr<FlatRowBlock> block) {
    LayoutKey key{block->GetSizes(), block->GetRowSize() / block->GetSizes().size()};
    auto& list = m_blocks[key];

    constexpr size_t MAX_BLOCKS_PER_LAYOUT = 16;
    if (list.size() < MAX_BLOCKS_PER_LAYOUT) {
        list.emplace_back(std::move(block));
    }
    // else silently drop block (or log warning if needed)
}
