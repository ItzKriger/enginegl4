#pragma once
#include <string>
#include <vector>
#include <algorithm>

class CComponentPriority
{
public:
    virtual ~CComponentPriority() = default;
    virtual int GetSortingKey() const = 0;

    virtual bool ShouldExecuteAfter(const std::string& other) const;
    virtual int GetGroup() const; //TODO what is it supposed to mean?
};

class CDefaultPriority : public CComponentPriority
{
public:
    int GetSortingKey() const override;
};

class CIndexedPriority : public CComponentPriority
{
public:
    CIndexedPriority(int priority = 0);
    int GetSortingKey() const override;
private:
    int Priority = 0;
};

class CFirstPriority : public CComponentPriority
{
public:
    int GetSortingKey() const override;
};

class CLastPriority : public CComponentPriority
{
public:
    int GetSortingKey() const override;
};

class CAfterPriority : public CComponentPriority
{
public:
    CAfterPriority(std::vector<std::string> deps);

    int GetSortingKey() const override;
    bool ShouldExecuteAfter(const std::string& other) const override;
private:
    std::vector<std::string> Dependencies;
};
