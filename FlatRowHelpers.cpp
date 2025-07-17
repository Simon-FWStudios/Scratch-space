#include "FlatRowHelpers.h"
#include "DBField.h"  // This must define CDBField and its CopyBuffer method

FlatRowBlock::FlatRowBlock(int rowCount, const std::vector<int>& columnSizes)
    : m_rowCount(rowCount), m_columnSizes(columnSizes)
{
    m_rowSize = 0;
    for (int size : m_columnSizes) {
        m_offsets.push_back(m_rowSize);
        m_rowSize += size;
    }

    m_bufferSize = m_rowSize * m_rowCount;
    m_buffer = std::make_unique<char[]>(m_bufferSize);
    std::memset(m_buffer.get(), 0, m_bufferSize);
}

char* FlatRowBlock::GetBindPointer(int colIndex, int rowIndex) {
    return m_buffer.get() + (rowIndex * m_rowSize + m_offsets[colIndex]);
}

char* FlatRowBlock::GetRowPointer(int rowIndex) {
    return m_buffer.get() + rowIndex * m_rowSize;
}

int FlatRowBlock::GetRowSize() const {
    return m_rowSize;
}

int FlatRowBlock::GetFieldOffset(int colIndex) const {
    return m_offsets[colIndex];
}

RowAccessor::RowAccessor(char* rowPtr, const std::vector<int>& offsets, const std::vector<int>& sizes)
    : m_rowPtr(rowPtr), m_offsets(offsets), m_sizes(sizes) {}

const char* RowAccessor::GetFieldPtr(int colIndex) const {
    return m_rowPtr + m_offsets[colIndex];
}

std::string_view RowAccessor::GetString(int colIndex) const {
    return std::string_view(GetFieldPtr(colIndex), strnlen(GetFieldPtr(colIndex), m_sizes[colIndex]));
}

int RowAccessor::GetFieldLength(int colIndex) const {
    return m_sizes[colIndex];
}

FlatRowCacheBridge::FlatRowCacheBridge(std::vector<CDBField*>& fields, const RowAccessor& accessor)
    : m_fields(fields), m_accessor(accessor) {}

void FlatRowCacheBridge::PopulateFields() {
    for (size_t i = 0; i < m_fields.size(); ++i) {
        const char* ptr = m_accessor.GetFieldPtr(static_cast<int>(i));
        int len = m_accessor.GetFieldLength(static_cast<int>(i));
        m_fields[i]->CopyBuffer(ptr, len);
    }
}
