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

    auto rowBlock = FlatRowPool::Get().Acquire(fieldSizes, maxRows);

    for (int col = 0; col < fieldCount; ++col) {
        SQLBindCol(hStmt, col + 1, SQL_C_CHAR,
                   rowBlock->GetBindPointer(col, 0),
                   fieldSizes[col], nullptr);
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

            std::string_view f1 = accessor.GetString(0);
            std::string_view f2 = accessor.GetString(1);
            uint64_t key = HashUtils::hash_combine(
                HashUtils::fnv1a_hash(f1.data(), f1.size()),
                HashUtils::fnv1a_hash(f2.data(), f2.size())
            );

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
        std::cout << "Row: " << obj->m_fAuditNo.ToString()
                  << ", " << obj->m_fObjType.ToString() << std::endl;
    }
}
