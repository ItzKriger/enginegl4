#include "CComponentPriority.h"

bool CComponentPriority::ShouldExecuteAfter(const std::string& other) const { return false; }
int CComponentPriority::GetGroup() const { return 0; }

int CDefaultPriority::GetSortingKey() const { return 0; }

CIndexedPriority::CIndexedPriority(int priority) : Priority(priority) {}
int CIndexedPriority::GetSortingKey() const { return Priority; }

int CFirstPriority::GetSortingKey() const { return INT_MAX; }
int CLastPriority::GetSortingKey() const { return INT_MIN; }

CAfterPriority::CAfterPriority(std::vector<std::string> deps)  : Dependencies(std::move(deps)) {}
int CAfterPriority::GetSortingKey() const { return 0; }
bool CAfterPriority::ShouldExecuteAfter(const std::string& other) const { return std::find(Dependencies.begin(), Dependencies.end(), other) != Dependencies.end(); }
