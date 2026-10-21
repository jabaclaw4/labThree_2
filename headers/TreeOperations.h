#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "BinaryTree.h"
#include "TraverseOrder.h"

// map строит новое дерево применяя функцию к каждому ключу
// если функция даёт одинаковые ключи для разных узлов
// второе добавление бросит invalid_argument
template <typename TKey, typename TElement>
BinaryTree<TKey, TElement> Map(const BinaryTree<TKey, TElement>& source,
                               std::function<TKey(const TKey&)> f) {
    BinaryTree<TKey, TElement> result;
    source.Traverse(TraverseOrder::InOrder, [&result, &f](const TKey& key, const TElement& element) {
        result.Add(f(key), element);
    });
    return result;
}

// where оставляет только узлы для которых pred вернул true
// обход in-order чтобы новое дерево строилось в возрастающем порядке и не перестраивалось
template <typename TKey, typename TElement>
BinaryTree<TKey, TElement> Where(const BinaryTree<TKey, TElement>& source,
                                 std::function<bool(const TKey&, const TElement&)> pred) {
    BinaryTree<TKey, TElement> result;
    source.Traverse(TraverseOrder::InOrder,
                    [&result, &pred](const TKey& key, const TElement& element) {
                        if (pred(key, element)) {
                            result.Add(key, element);
                        }
                    });
    return result;
}

// reduce свёртка коллекции в одно значение
// начальное значение start подставляется первым
template <typename TAcc, typename TKey, typename TElement>
TAcc Reduce(const BinaryTree<TKey, TElement>& source,
            std::function<TAcc(const TAcc&, const TKey&, const TElement&)> f,
            TAcc start) {
    TAcc acc = start;
    source.Traverse(TraverseOrder::InOrder,
                    [&acc, &f](const TKey& key, const TElement& element) {
                        acc = f(acc, key, element);
                    });
    return acc;
}

// сериализация в строку формата key1:element1|key2:element2
// порядок обхода по умолчанию in-order
template <typename TKey, typename TElement>
std::string ToString(const BinaryTree<TKey, TElement>& source,
                     TraverseOrder order = TraverseOrder::InOrder) {
    std::ostringstream out;
    bool first = true;
    source.Traverse(order, [&out, &first](const TKey& key, const TElement& element) {
        if (!first) {
            out << "|";
        }
        first = false;
        out << key << ":" << element;
    });
    return out.str();
}

// чтение из строки обратное к ToString
// при некорректном формате бросает invalid_argument
template <typename TKey, typename TElement>
BinaryTree<TKey, TElement> FromString(const std::string& s) {
    BinaryTree<TKey, TElement> result;
    if (s.empty()) {
        return result;
    }

    size_t pos = 0;
    while (pos < s.size()) {
        size_t pairEnd = s.find('|', pos); //если он доходит до конца строки и не находит | он возвращает std::string::npos
        if (pairEnd == std::string::npos) {
            pairEnd = s.size();
        }
        std::string pair = s.substr(pos, pairEnd - pos);
        size_t colon = pair.find(':');
        if (colon == std::string::npos) {
            throw std::invalid_argument("FromString: missing ':' in pair: " + pair);
        }

        TKey key{};
        TElement element{};
        std::istringstream keyStream(pair.substr(0, colon));
        std::istringstream elementStream(pair.substr(colon + 1));

        if (!(keyStream >> key)) {
            throw std::invalid_argument("FromString: cannot parse key from: " + pair);
        }
        if (!(elementStream >> element)) {
            throw std::invalid_argument("FromString: cannot parse element from: " + pair);
        }
        if (result.ContainsKey(key)) {
            throw std::invalid_argument("FromString: duplicate key");
        }
        result.Add(key, element);

        pos = pairEnd + 1;
    }
    return result;
}

// извлечение поддерева с корнем в узле с ключом rootKey
// возвращает новое дерево с этим поддеревом
// если корня нет возвращает пустое дерево
template <typename TKey, typename TElement>
BinaryTree<TKey, TElement> ExtractSubtree(const BinaryTree<TKey, TElement>& source,
                                          const TKey& rootKey,
                                          TraverseOrder order = TraverseOrder::InOrder) {
    BinaryTree<TKey, TElement> result;
    source.ForEachFrom(rootKey, order,
                       [&result](const TKey& key, const TElement& element) {
                           result.Add(key, element);
                       });
    return result;
}

// проверка что sub содержится в source как поддерево
// обходим все узлы source в pre-order и сравниваем обход от каждого узла с обходом sub
// пустое sub считается содержащимся в любом дереве
template <typename TKey, typename TElement>
bool ContainsSubtree(const BinaryTree<TKey, TElement>& source,
                     const BinaryTree<TKey, TElement>& sub) {
    if (sub.IsEmpty()) {
        return true;
    }
    if (source.IsEmpty()) {
        return false;
    }

    DynamicArray<std::pair<TKey, TElement>> subSeq = sub.Traverse(TraverseOrder::PreOrder);
    size_t subSize = subSeq.GetSize();

    DynamicArray<TKey> keysInSource;
    source.Traverse(TraverseOrder::PreOrder,
                    [&keysInSource](const TKey& key, const TElement&) {
                        keysInSource.PushBack(key);
                    });

    for (size_t i = 0; i < keysInSource.GetSize(); ++i) {
        DynamicArray<std::pair<TKey, TElement>> candidate;
        source.ForEachFrom(keysInSource[i], TraverseOrder::PreOrder,
                           [&candidate](const TKey& key, const TElement& element) {
                               candidate.PushBack(std::make_pair(key, element));
                           });

        if (candidate.GetSize() != subSize) {
            continue;
        }
        bool match = true;
        for (size_t j = 0; j < subSize; ++j) {
            if (!(candidate[j].first == subSeq[j].first) ||
                !(candidate[j].second == subSeq[j].second)) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

// слияние двух деревьев
// при совпадении ключей бросаем исключение
template <typename TKey, typename TElement>
BinaryTree<TKey, TElement> Merge(const BinaryTree<TKey, TElement>& a,
                                 const BinaryTree<TKey, TElement>& b) {
    BinaryTree<TKey, TElement> result = a;
    b.Traverse(TraverseOrder::InOrder,
               [&result](const TKey& key, const TElement& element) {
                   if (result.ContainsKey(key)) {
                       throw std::invalid_argument("Merge: duplicate key");
                   }
                   result.Add(key, element);
               });
    return result;
}