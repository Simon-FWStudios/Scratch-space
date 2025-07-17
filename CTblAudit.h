#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "DBField.h"

class SQLHSTMT;
class FlatRowBlock;

class CTblAudit {
public:
    explicit CTblAudit(void* db) : m_pDatabase(db) {}

    // Caches all rows in a hash map using up to 3 key fields
    bool CacheTableOptimised(SQLHSTMT hStmt, const char* sql, int maxRows);

    // Iterates and prints cached row values
    void IterateCachedRows();

    // Row field storage
    std::vector<CDBField*> m_aFields;

    // Key field examples (optional access)
    CDBStringField m_fAuditNo;
    CDBStringField m_fObjType;

private:
    void* m_pDatabase;

    // Optimised cache storage with hashed keys
    std::unordered_map<uint64_t, std::unique_ptr<CTblAudit>> m_OptimisedCache;
};
