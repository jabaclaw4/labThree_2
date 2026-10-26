#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <sstream>
#include "DynamicArray.h"
#include "IDictionary.h"
#include "TraverseOrder.h"

// бинарное дерево поиска
// все операции без рекурсии потому что на упорядоченных данных дерево вырождается в список
// рекурсия глубиной в сотни тысяч вызовов переполнила бы стек
// от вырождения защищает периодическая перестройка в сбалансированное дерево
template <typename TKey, typename TElement>
class BinaryTree : public IDictionary<TKey, TElement> {
private:
    struct Node {
        TKey key;
        TElement element;
        Node* left;
        Node* right;

        Node(const TKey& k, const TElement& e) : key(k), element(e), left(nullptr), right(nullptr) {}
    };

    Node* root_;
    size_t count_;
    size_t height_;

    Node* FindNode(const TKey& key) const {
        Node* cur = root_;
        while (cur != nullptr) {
            if (key < cur->key) {
                cur = cur->left;
            } else if (cur->key < key) {
                cur = cur->right;
            } else {
                return cur;
            }
        }
        return nullptr;
    }

    // удаляем узлы через свой стек чтобы деструктор тоже не уходил в рекурсию
    void FreeAll() {
        if (root_ == nullptr) {
            return;
        }
        DynamicArray<Node*> stack;
        stack.PushBack(root_);
        while (!stack.IsEmpty()) {
            Node* node = stack[stack.GetSize() - 1];
            stack.PopBack();
            if (node->left != nullptr) {
                stack.PushBack(node->left);
            }
            if (node->right != nullptr) {
                stack.PushBack(node->right);
            }
            delete node;
        }
        root_ = nullptr;
        count_ = 0;
        height_ = 0;
    }

    // перестройка в сбалансированное дерево если высота превысила порог
    // порог 4 * log2(n + 1) + 1 выбран чтобы случайное дерево не перестраивалось слишком часто
    void Rebalance() {
        size_t limit = static_cast<size_t>(4.0 * std::log2(static_cast<double>(count_ + 1))) + 1;
        if (height_ <= limit) {
            return;
        }

        // собираем узлы в порядке возрастания ключей без рекурсии
        DynamicArray<Node*> nodes;
        DynamicArray<Node*> stack;
        Node* cur = root_;
        while (cur != nullptr || !stack.IsEmpty()) {
            while (cur != nullptr) {
                stack.PushBack(cur);
                cur = cur->left;
            }
            cur = stack[stack.GetSize() - 1];
            stack.PopBack();
            nodes.PushBack(cur);
            cur = cur->right;
        }

        // сборка сбалансированного дерева по середине
        // глубина рекурсии тут log2(n) поэтому переполнения не будет
        struct Builder {
            static Node* Build(const DynamicArray<Node*>& nodes, size_t lo, size_t hi) {
                if (lo >= hi) {
                    return nullptr;
                }
                size_t mid = lo + (hi - lo) / 2;
                Node* node = nodes[mid];
                node->left = Build(nodes, lo, mid);
                node->right = Build(nodes, mid + 1, hi);
                return node;
            }
        };
        root_ = Builder::Build(nodes, 0, nodes.GetSize());
        height_ = GetHeight();
    }

    // сериализация узла по заданному формату
    std::string SerializeNode(Node* node, const std::string& format) const {
        if (node == nullptr) return "";
        std::string res;

        // определяем какие буквы использовать (русские или английские)
        std::string kSym = (format.find("К") != std::string::npos) ? "К" : "K";
        std::string lSym = (format.find("Л") != std::string::npos) ? "Л" : "L";
        std::string pSym = (format.find("П") != std::string::npos) ? "П" : "P";

        std::ostringstream ss;
        ss << node->key;
        std::string keyStr = ss.str();

        for (size_t i = 0; i < format.size(); ) {
            if (format.compare(i, kSym.size(), kSym) == 0) {
                res += keyStr;
                i += kSym.size();
            } else if (format.compare(i, lSym.size(), lSym) == 0) {
                res += SerializeNode(node->left, format);
                i += lSym.size();
            } else if (format.compare(i, pSym.size(), pSym) == 0) {
                res += SerializeNode(node->right, format);
                i += pSym.size();
            } else {
                res += format[i];
                i++;
            }
        }
        return res;
    }

    // обход дерева для вывода пар узел родитель
    void PrintEdges(std::ostream& os, Node* node, Node* parent) const {
        if (node == nullptr) return;
        if (parent != nullptr) {
            os << parent->key << "->" << node->key << "|";
        }
        PrintEdges(os, node->left, node);
        PrintEdges(os, node->right, node);
    }

    // единый универсальный обход на основе машины состояний
    // вместо дублирования кода мы используем стек который хранит состояние узла
    // 0 только зашли в узел
    // 1 вернулись из первой ветки
    // 2 вернулись из второй ветки
    template <typename Visitor>
    void TraverseFromNode(Node* start, TraverseOrder order, Visitor visit) const {
        if (start == nullptr) {
            return;
        }

        // определяем стратегию обхода на основе переданного порядка
        // в какую сторону идти сначала (зеркально или нет)
        bool mirrored = (order == TraverseOrder::ReversePreOrder ||
                         order == TraverseOrder::ReverseInOrder ||
                         order == TraverseOrder::ReversePostOrder);

        // на каком шаге вызывать функцию visit
        // 0 = PreOrder (сразу), 1 = InOrder (в середине), 2 = PostOrder (в конце)
        int visitState = 0;
        if (order == TraverseOrder::InOrder || order == TraverseOrder::ReverseInOrder) {
            visitState = 1;
        } else if (order == TraverseOrder::PostOrder || order == TraverseOrder::ReversePostOrder) {
            visitState = 2;
        }

        // структура для стека
        struct StackFrame {
            Node* node;
            int state;
            StackFrame() : node(nullptr), state(0) {} // обязательный конструктор для DynamicArray
            StackFrame(Node* n, int s) : node(n), state(s) {}
        };

        DynamicArray<StackFrame> stack;
        stack.PushBack(StackFrame(start, 0));

        while (!stack.IsEmpty()) {
            // берем копию а не ссылку потому что PushBack может перевыделить память
            StackFrame frame = stack[stack.GetSize() - 1];
            Node* cur = frame.node;

            if (frame.state == 0) {
                // состояние 0 мы только что зашли в узел
                if (visitState == 0) {
                    visit(cur->key, cur->element);
                }
                stack[stack.GetSize() - 1].state = 1; // переводим в следующее состояние

                Node* firstChild = mirrored ? cur->right : cur->left;
                if (firstChild != nullptr) {
                    stack.PushBack(StackFrame(firstChild, 0));
                }
            }
            else if (frame.state == 1) {
                // состояние 1 мы вернулись из первой ветки (левой или правой)
                if (visitState == 1) {
                    visit(cur->key, cur->element);
                }
                stack[stack.GetSize() - 1].state = 2; // переводим в следующее состояние

                Node* secondChild = mirrored ? cur->left : cur->right;
                if (secondChild != nullptr) {
                    stack.PushBack(StackFrame(secondChild, 0));
                }
            }
            else {
                // состояние 2 мы вернулись из второй ветки обе ветки обойдены
                if (visitState == 2) {
                    visit(cur->key, cur->element);
                }
                stack.PopBack(); // узел полностью обработан удаляем из стека
            }
        }
    }

public:
    class Iterator {
    private:
        DynamicArray<std::pair<TKey, TElement>> items_;
        size_t position_;

    public:
        Iterator() : items_(), position_(0) {}

        explicit Iterator(DynamicArray<std::pair<TKey, TElement>> items)
                : items_(std::move(items)), position_(0) {}

        bool HasNext() const { return position_ < items_.GetSize(); }

        const std::pair<TKey, TElement>& Current() const {
            if (!HasNext()) {
                throw std::out_of_range("BinaryTree::Iterator out of range");
            }
            return items_[position_];
        }

        const std::pair<TKey, TElement>& Next() {
            if (!HasNext()) {
                throw std::out_of_range("BinaryTree::Iterator out of range");
            }
            return items_[position_++];
        }

        void Reset() { position_ = 0; }

        size_t GetRemaining() const { return items_.GetSize() - position_; }
    };

    BinaryTree() : root_(nullptr), count_(0), height_(0) {}

    // глубокое копирование форма дерева сохраняется
    BinaryTree(const BinaryTree& other) : root_(nullptr), count_(0), height_(0) {
        count_ = other.count_;
        height_ = other.height_;
        if (other.root_ == nullptr) {
            return;
        }
        root_ = new Node(other.root_->key, other.root_->element);
        DynamicArray<std::pair<Node*, Node*>> stack;
        stack.PushBack(std::make_pair(other.root_, root_));
        while (!stack.IsEmpty()) {
            std::pair<Node*, Node*> top = stack[stack.GetSize() - 1];
            stack.PopBack();
            Node* src = top.first;
            Node* dst = top.second;
            if (src->left != nullptr) {
                dst->left = new Node(src->left->key, src->left->element);
                stack.PushBack(std::make_pair(src->left, dst->left));
            }
            if (src->right != nullptr) {
                dst->right = new Node(src->right->key, src->right->element);
                stack.PushBack(std::make_pair(src->right, dst->right));
            }
        }
    }

    BinaryTree& operator=(const BinaryTree& other) {
        if (this != &other) {
            BinaryTree tmp(other);
            FreeAll();
            root_ = tmp.root_;
            count_ = tmp.count_;
            height_ = tmp.height_;
            tmp.root_ = nullptr;
            tmp.count_ = 0;
            tmp.height_ = 0;
        }
        return *this;
    }

    BinaryTree(BinaryTree&& other) noexcept
            : root_(other.root_), count_(other.count_), height_(other.height_) {
        other.root_ = nullptr;
        other.count_ = 0;
        other.height_ = 0;
    }

    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if (this != &other) {
            FreeAll();
            root_ = other.root_;
            count_ = other.count_;
            height_ = other.height_;
            other.root_ = nullptr;
            other.count_ = 0;
            other.height_ = 0;
        }
        return *this;
    }

    ~BinaryTree() override {
        FreeAll();
    }

    size_t GetCount() const override { return count_; }

    bool IsEmpty() const { return count_ == 0; }

    TElement& Get(const TKey& key) override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("BinaryTree::Get key not found");
        }
        return node->element;
    }

    const TElement& Get(const TKey& key) const override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("BinaryTree::Get key not found");
        }
        return node->element;
    }

    bool ContainsKey(const TKey& key) const override {
        return FindNode(key) != nullptr;
    }

    void Add(const TKey& key, const TElement& element) override {
        if (root_ == nullptr) {
            root_ = new Node(key, element);
            count_ = 1;
            height_ = 1;
            return;
        }
        // идём по дереву одним указателем и держим глубину
        // как только нашли пустое место вешаем туда новый узел
        Node* cur = root_;
        size_t depth = 1;
        while (true) {
            if (key < cur->key) {
                if (cur->left == nullptr) {
                    cur->left = new Node(key, element);
                    ++depth;
                    break;
                }
                cur = cur->left;
            } else if (cur->key < key) {
                if (cur->right == nullptr) {
                    cur->right = new Node(key, element);
                    ++depth;
                    break;
                }
                cur = cur->right;
            } else {
                throw std::invalid_argument("BinaryTree::Add key already exists");
            }
            ++depth;
        }
        if (depth > height_) {
            height_ = depth;
        }
        ++count_;
        Rebalance();
    }

    void Remove(const TKey& key) override {
        Node** link = &root_;
        while (*link != nullptr) {
            if (key < (*link)->key) {
                link = &(*link)->left;
            } else if ((*link)->key < key) {
                link = &(*link)->right;
            } else {
                break;
            }
        }
        if (*link == nullptr) {
            throw std::out_of_range("BinaryTree::Remove key not found");
        }

        Node* node = *link;
        if (node->left == nullptr) {
            *link = node->right;
        } else if (node->right == nullptr) {
            *link = node->left;
        } else {
            // два потомка берём минимум правого поддерева
            Node** successorLink = &node->right;
            while ((*successorLink)->left != nullptr) {
                successorLink = &(*successorLink)->left;
            }
            Node* successor = *successorLink;
            *successorLink = successor->right;
            successor->left = node->left;
            successor->right = node->right;
            *link = successor;
        }
        delete node;
        --count_;
        Rebalance();
    }

    template <typename Visitor>
    void ForEachInRange(const TKey& lo, const TKey& hi, Visitor visit) const {
        DynamicArray<Node*> stack;
        Node* cur = root_;
        while (cur != nullptr || !stack.IsEmpty()) {
            while (cur != nullptr) {
                if (cur->key < lo) {
                    cur = cur->right;
                } else {
                    stack.PushBack(cur);
                    cur = cur->left;
                }
            }
            if (stack.IsEmpty()) {
                break;
            }
            Node* node = stack[stack.GetSize() - 1];
            stack.PopBack();
            if (hi < node->key) {
                break;
            }
            visit(node->key, node->element);
            cur = node->right;
        }
    }

    // обход всего дерева с вызовом visitor для каждого узла
    template <typename Visitor>
    void Traverse(TraverseOrder order, Visitor visit) const {
        TraverseFromNode(root_, order, visit);
    }

    // обход поддерева с корнем в узле с ключом rootKey
    // если такого узла нет ничего не делает
    template <typename Visitor>
    void ForEachFrom(const TKey& rootKey, TraverseOrder order, Visitor visit) const {
        Node* start = FindNode(rootKey);
        if (start == nullptr) {
            return;
        }
        TraverseFromNode(start, order, visit);
    }

    // обёртка обхода в массив пар
    DynamicArray<std::pair<TKey, TElement>> Traverse(TraverseOrder order) const {
        DynamicArray<std::pair<TKey, TElement>> result;
        Traverse(order, [&result](const TKey& key, const TElement& element) {
            result.PushBack(std::make_pair(key, element));
        });
        return result;
    }

    Iterator Begin(TraverseOrder order = TraverseOrder::InOrder) const {
        return Iterator(Traverse(order));
    }

    size_t GetHeight() const {
        if (root_ == nullptr) {
            return 0;
        }
        DynamicArray<Node*> level;
        level.PushBack(root_);
        size_t height = 0;
        while (!level.IsEmpty()) {
            ++height;
            DynamicArray<Node*> next;
            for (size_t i = 0; i < level.GetSize(); ++i) {
                if (level[i]->left != nullptr) {
                    next.PushBack(level[i]->left);
                }
                if (level[i]->right != nullptr) {
                    next.PushBack(level[i]->right);
                }
            }
            level = std::move(next);
        }
        return height;
    }

    // сохранение дерева по строке формата
    std::string ToStringByFormat(const std::string& format) const {
        return SerializeNode(root_, format);
    }

    // сохранение дерева в виде списка пар узел родитель
    std::string ToStringParentList() const {
        std::ostringstream os;
        PrintEdges(os, root_, nullptr);
        return os.str();
    }


};