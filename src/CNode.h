#pragma once
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

#include "U_String.h"

template<typename T>
class CNode
{
public:
    CNode<T>* Parent = nullptr;
    std::string Name;

    CNode() = default;
    CNode(const std::string& name, CNode<T>* parent) : Name(name), Parent(parent) {}

    std::unordered_map<std::string, std::unique_ptr<CNode>> Children;
    T Value;

    std::string GetPath() const
    {
        const CNode<T>* curr = this;
        std::string path;

        while(curr)
        {
            if(!curr->Name.empty())
            {
                path.insert(0, ("." + curr->Name));
            }
            curr = curr->Parent;
        }

        if(!path.empty())
        {
            path.erase(path.begin());
        }
        return path;
    }

    void Trace(const std::function<void(const CNode<T>* nod)>& func, bool first_parent = true)
    {
        if(first_parent)
        {
            func(this);
        }

        for(auto& kv : Children)
        {
            kv.second->Trace(func, first_parent);
        }

        if(!first_parent)
        {
            func(this);
        }
    }

    CNode<T>* GetNode(const std::string& path) const
    {
        std::vector<std::string> nodes;
        StringUtils::split_str(path, '.', nodes);

        if (nodes.empty()) { return nullptr; }

        const CNode<T>* current = this;
        for (std::string& nod : nodes)
        {
            if(StringUtils::IsEmpty(nod)) { continue; }

            auto it = current->Children.find(nod);
            if (it == current->Children.end())
            {
                return nullptr;
            }

            current = it->second.get();
        }
        return const_cast<CNode<T>*>(current); //HACK
    }

    void DeleteNode(CNode<T>* nod, bool cleanup_nodes = true) //TODO fix (maybe cleanup_nodes should be done before deleting the nod itself?)
    {
        if (nod == nullptr || nod->Parent == nullptr)
        {
            return;
        }

        auto it = nod->Parent->Children.begin();
        while (it != nod->Parent->Children.end())
        {
            if (it->second.get() == nod)
            {
                break;
            }
            ++it;
        }

        if (it != nod->Parent->Children.end())
        {
            nod->Parent->Children.erase(it);
        }

        if (cleanup_nodes)
        {
            CNode<T>* current = nod->Parent;
            while (current != nullptr && current->Children.empty())
            {
                CNode<T>* parent = current->Parent;
                if (parent != nullptr)
                {
                    auto it = parent->Children.begin();
                    while (it != parent->Children.end())
                    {
                        if (it->second.get() == current)
                        {
                            parent->Children.erase(it);
                            break;
                        }
                        ++it;
                    }
                }
                current = parent;
            }
        }
    }

    CNode<T>* CreateNode(const std::string& path)
    {
        std::istringstream stream(path);
        std::string segment;
        CNode<T>* current = this;

        while (std::getline(stream, segment, '.'))
        {
            if (current->Children.find(segment) == current->Children.end())
            {
                current->Children[segment] = std::make_unique<CNode<T>>(segment, current);
            }
            current = current->Children[segment].get();
        }

        return current;
    }
};
