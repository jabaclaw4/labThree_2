#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <sstream>
#include "DynamicArray.h"
#include "IDictionary.h"
#include "TraverseOrder.h"

// шаги обхода для итератора
enum class Step { NODE, LEFT, RIGHT };

// предварительное объявление чтобы дерево могло возвращать итератор
template <typename TKey, typename TElement>
class TreeIterator;

// бинарное дерево поиска
// все операции без рекурсии потому что на упорядоченных данных дерево вырождается в список
// рекурсия глубиной в сотни тысяч вызовов переполнила бы стек
// от вырождения защищает периодическая перестройка в сбалансированное дерево
template <typename TKey, typename TElement>
class BinaryTree : public IDictionary<TKey, TElement> {
public:
    struct Node {
        TKey key;
        TElement element;
        Node* left;
        Node* right;

        Node(const TKey& k, const TElement& e) : key(k), element(e), left(nullptr), right(nullptr) {}
    };

private:
    Node* root_;
    size_t count_;
    size_t height_;

    // поиск узла по ключу итеративно
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

public:
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

    // обход всего дерева через ленивый итератор
    // здесь больше нет машины состояний и switch
    // функция просто дергает Next() у итератора
    template <typename Visitor>
    void Traverse(TraverseOrder order, Visitor visit) const {
        TreeIterator<TKey, TElement> it(root_, order);
        while (it.HasNext()) {
            auto pair = it.Next();
            visit(pair.first, pair.second);
        }
    }

    // обход поддерева с корнем в узле с ключом rootKey
    // если такого узла нет ничего не делает
    template <typename Visitor>
    void ForEachFrom(const TKey& rootKey, TraverseOrder order, Visitor visit) const {
        Node* start = FindNode(rootKey);
        if (start == nullptr) {
            return;
        }
        TreeIterator<TKey, TElement> it(start, order);
        while (it.HasNext()) {
            auto pair = it.Next();
            visit(pair.first, pair.second);
        }
    }

    // обёртка обхода в массив пар (заменяет старый Traverse который возвращал массив)
    DynamicArray<std::pair<TKey, TElement>> ToArray(TraverseOrder order) const {
        DynamicArray<std::pair<TKey, TElement>> result;
        Traverse(order, [&result](const TKey& key, const TElement& element) {
            result.PushBack(std::make_pair(key, element));
        });
        return result;
    }

    // создание итератора для заданного порядка
    TreeIterator<TKey, TElement> Begin(TraverseOrder order = TraverseOrder::InOrder) const {
        return TreeIterator<TKey, TElement>(root_, order);
    }

    // высота считается по уровням без рекурсии
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

    // геттер для корня нужен для графической отрисовки
    Node* GetRoot() const { return root_; }
};

// ==============================================================================
// ИТЕРАТОР (Data-Driven)
// ==============================================================================
// ленивый итератор который заранее находит следующий узел
// благодаря этому HasNext() просто проверяет указатель
// а Next() отдает данные и делает один шаг вперед
template <typename TKey, typename TElement>
class TreeIterator {
private:
    using Node = typename BinaryTree<TKey, TElement>::Node;

    // фрейм хранит узел и индекс текущего шага из сценария
    struct Frame {
        Node* node;
        int stepIndex;
    };

    static constexpr int kStepsCount = 3;

    // таблица сценариев обхода (data-driven)
    // вместо кучи if мы просто говорим в каком порядке посещать
    // порядок строк совпадает с порядком enum TraverseOrder
    static constexpr Step kScenarios[6][kStepsCount] = {
            {Step::NODE,  Step::LEFT,  Step::RIGHT},  // PreOrder
            {Step::LEFT,  Step::NODE,  Step::RIGHT},  // InOrder
            {Step::LEFT,  Step::RIGHT, Step::NODE },  // PostOrder
            {Step::NODE,  Step::RIGHT, Step::LEFT },  // ReversePreOrder
            {Step::RIGHT, Step::NODE,  Step::LEFT },  // ReverseInOrder
            {Step::RIGHT, Step::LEFT,  Step::NODE }   // ReversePostOrder
    };

    DynamicArray<Frame> stack_;
    const Step* steps_;
    Node* current_ = nullptr;

    // поиск следующего узла для выдачи
    void Advance() {
        current_ = nullptr;
        while (!stack_.IsEmpty()) {
            Frame& frame = stack_[stack_.GetSize() - 1];

            // если все шаги для узла сделаны удаляем его
            if (frame.stepIndex >= kStepsCount) {
                stack_.PopBack();
                continue;
            }

            // забираем данные до PushBack так как ссылка frame может стать невалидной
            Node* cur = frame.node;
            Step step = steps_[frame.stepIndex++];

            if (step == Step::NODE) {
                current_ = cur;
                return;
            }

            Node* child = (step == Step::LEFT) ? cur->left : cur->right;
            if (child != nullptr) {
                stack_.PushBack({child, 0});
            }
        }
    }

public:
    TreeIterator(Node* root, TraverseOrder order)
            : steps_(kScenarios[static_cast<int>(order)]) {
        if (root != nullptr) {
            stack_.PushBack({root, 0});
        }
        Advance();
    }

    bool HasNext() const { return current_ != nullptr; }

    std::pair<TKey, TElement> Next() {
        if (current_ == nullptr) {
            throw std::out_of_range("End of iterator");
        }
        std::pair<TKey, TElement> result{current_->key, current_->element};
        Advance();
        return result;
    }
};

// определение статического массива (требуется для некоторых версий C++)
template <typename TKey, typename TElement>
constexpr Step TreeIterator<TKey, TElement>::kScenarios[6][TreeIterator<TKey, TElement>::kStepsCount];