#pragma once

template<typename Class>
class CIteratableBase
{
public:
	virtual ~CIteratableBase() {}

	virtual bool operator==(const Class& other) const { return false; };
	virtual bool operator!=(const Class& other) const { return false; };
	virtual bool operator<(const Class& other) const { return false; };
	virtual bool operator>(const Class& other) const { return false; };
	virtual bool operator<=(const Class& other) const { return false; };
	virtual bool operator>=(const Class& other) const { return false; };
};
