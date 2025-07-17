#pragma once
#include <vector>
#include <memory>
#include <string_view>
#include <cstring>

class FlatRowBlock {
public:
    FlatRowBlock(int rowCount, const std::vector<int>& columnSizes);

    char* GetBindPointer(int colIndex, int rowIndex);
    char* GetRowPointer(int rowIndex);
    int GetRowSize() const;
    int GetFieldOffset(int colIndex) const;

private:
    int m_rowCount;
    int m_rowSize;
    int m_bufferSize;
    std::vector<int> m_columnSizes;
    std::vector<int> m_offsets;
    std::unique_ptr<char[]> m_buffer;
};

class RowAccessor {
public:
    RowAccessor(char* rowPtr, const std::vector<int>& offsets, const std::vector<int>& sizes);

    const char* GetFieldPtr(int colIndex) const;
    std::string_view GetString(int colIndex) const;
    int GetFieldLength(int colIndex) const;

private:
    char* m_rowPtr;
    const std::vector<int>& m_offsets;
    const std::vector<int>& m_sizes;
};

class CDBField;  // Forward declaration

class FlatRowCacheBridge {
public:
    FlatRowCacheBridge(std::vector<CDBField*>& fields, const RowAccessor& accessor);
    void PopulateFields();

private:
    std::vector<CDBField*>& m_fields;
    const RowAccessor& m_accessor;
};
