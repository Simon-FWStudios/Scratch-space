#include "FlatRowHelpers.h"
#include "FlatRowPool.h"
#include "HashUtils.h"
#include "CTblAudit.h"
#include "DBField.h"
#include <sqlext.h>
#include <unordered_map>
#include <memory>
#include <iostream>

bool CTblAudit::CacheTableOptimised(SQLHSTMT hStmt, const char* sql, int maxRows) {
    const int fieldCount = m_aFields.GetSize();
    std::vector<int> fieldSizes(fieldCount);
    std::vector<CDBField*> fields(fieldCount);

    for (int i = 0; i < fieldCount; ++i) {
        fields[i] = m_aFields[i];
        fieldSizes[i] = fields[i]->GetBindLength();
    }

    // Thread-local FlatRowPool usage
    auto rowBlock = FlatRowPool::Get().Acquire(fieldSizes, maxRows);

    std::vector<SQLLEN> indicators(fieldCount * maxRows);

    for (int col = 0; col < fieldCount; ++col) {
        SQLBindCol(hStmt, col + 1, SQL_C_CHAR,
                   rowBlock->GetBindPointer(col, 0),
                   fieldSizes[col], &indicators[col * maxRows]);
    }

    SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)(SQLLEN)maxRows, 0);
    SQLULEN rowsFetched = 0;
    SQLSetStmtAttr(hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);

    if (SQLExecDirect(hStmt, (SQLCHAR*)sql, SQL_NTS) != SQL_SUCCESS) {
        std::cerr << "SQLExecDirect failed" << std::endl;
        return false;
    }

    m_OptimisedCache.clear();

    while (SQLFetch(hStmt) == SQL_SUCCESS || SQLFetch(hStmt) == SQL_SUCCESS_WITH_INFO) {
        for (SQLULEN row = 0; row < rowsFetched; ++row) {
            RowAccessor accessor(rowBlock->GetRowPointer(row), rowBlock->GetOffsets(), rowBlock->GetSizes());

            uint64_t key = 0;
            bool first = true;

            // Allow up to 3 fields to form the cache key
            for (int k = 0; k < 3; ++k) {
                if (k >= fieldCount) break;
                auto* field = fields[k];
                const char* ptr = accessor.GetFieldPtr(k);
                int len = accessor.GetFieldLength(k);

                if (indicators[k * maxRows + row] == SQL_NULL_DATA) continue;

                uint64_t partHash = 0;
                if (dynamic_cast<CDBStringField*>(field)) {
                    partHash = HashUtils::fnv1a_hash(ptr, std::strlen(ptr));
                } else if (dynamic_cast<CDBLongField*>(field)) {
                    long val = std::strtol(ptr, nullptr, 10);
                    partHash = std::hash<long>()(val);
                }

                key = first ? partHash : HashUtils::hash_combine(key, partHash);
                first = false;
            }

            if (key == 0) {
                std::cerr << "Skipping row with no keyable fields
";
                continue;
            }

            std::unique_ptr<CTblAudit> obj = std::make_unique<CTblAudit>(m_pDatabase);
            FlatRowCacheBridge bridge(obj->m_aFields, accessor);
            bridge.PopulateFields();
            m_OptimisedCache[key] = std::move(obj);
        }
    }

    return true;
}

void CTblAudit::IterateCachedRows() {
    for (const auto& [key, obj] : m_OptimisedCache) {
        std::cout << "Row: ";
        for (int i = 0; i < obj->m_aFields.GetSize(); ++i) {
            CDBField* f = obj->m_aFields[i];
            std::cout << f->ToString() << " | ";
        }
        std::cout << "\n";
    }
}
