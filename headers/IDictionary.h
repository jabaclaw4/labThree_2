#pragma once

#include <cstddef>

// интерфейс словаря
template <typename TKey, typename TElement>
class IDictionary {
public:
    virtual ~IDictionary() = default;

    virtual size_t GetCount() const = 0;

    // по умолчанию вместимость равна количеству
    virtual size_t GetCapacity() const { return GetCount(); }

    virtual TElement& Get(const TKey& key) = 0;
    virtual const TElement& Get(const TKey& key) const = 0;

    virtual bool ContainsKey(const TKey& key) const = 0;
    virtual void Add(const TKey& key, const TElement& element) = 0;
    virtual void Remove(const TKey& key) = 0;
};