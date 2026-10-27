#include "Tests.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include "BinaryTree.h"
#include "DynamicArray.h"
#include "TestUtils.h"
#include "TraverseOrder.h"
#include "TreeOperations.h"

int g_testFailures = 0;
int g_testChecks = 0;

namespace {

    void TestBinaryTreeBasic() {
        BinaryTree<int, int> tree;
        CHECK(tree.IsEmpty());
        CHECK_THROWS(tree.Get(1), std::out_of_range);
        CHECK_THROWS(tree.Remove(1), std::out_of_range);

        tree.Add(5, 50);
        tree.Add(3, 30);
        tree.Add(8, 80);
        CHECK(tree.GetCount() == 3);
        CHECK(tree.Get(3) == 30);
        CHECK(tree.ContainsKey(8));
        CHECK(!tree.ContainsKey(4));
        CHECK_THROWS(tree.Add(5, 1), std::invalid_argument);
    }

    void TestBinaryTreeRemove() {
        BinaryTree<int, int> tree;
        int keys[] = {50, 30, 70, 20, 40, 60, 80, 65};
        for (int key : keys) {
            tree.Add(key, key * 10);
        }
        tree.Remove(20);
        tree.Remove(60);
        tree.Remove(70);
        tree.Remove(50);
        CHECK(tree.GetCount() == 4);
        CHECK(tree.Get(30) == 300);
        CHECK(tree.Get(40) == 400);
        CHECK(tree.Get(65) == 650);
        CHECK(tree.Get(80) == 800);
    }

    void TestTraverse() {
        BinaryTree<int, int> tree;
        int keys[] = {5, 3, 8, 2, 4, 7, 9};
        for (int k : keys) {
            tree.Add(k, k * 10);
        }

        auto checkOrder = [&tree](TraverseOrder order, const int* expected, size_t n) {
            DynamicArray<std::pair<int, int>> result = tree.Traverse(order);
            CHECK(result.GetSize() == n);
            for (size_t i = 0; i < n; ++i) {
                CHECK(result[i].first == expected[i]);
            }
        };

        int pre[] = {5, 3, 2, 4, 8, 7, 9};
        checkOrder(TraverseOrder::PreOrder, pre, 7);
        int in[] = {2, 3, 4, 5, 7, 8, 9};
        checkOrder(TraverseOrder::InOrder, in, 7);
        int post[] = {2, 4, 3, 7, 9, 8, 5};
        checkOrder(TraverseOrder::PostOrder, post, 7);
        int rpre[] = {5, 8, 9, 7, 3, 4, 2};
        checkOrder(TraverseOrder::ReversePreOrder, rpre, 7);
        int rin[] = {9, 8, 7, 5, 4, 3, 2};
        checkOrder(TraverseOrder::ReverseInOrder, rin, 7);
        int rpost[] = {9, 7, 8, 4, 2, 3, 5};
        checkOrder(TraverseOrder::ReversePostOrder, rpost, 7);
    }

    void TestMap() {
        BinaryTree<int, int> tree;
        tree.Add(1, 10);
        tree.Add(2, 20);
        tree.Add(3, 30);

        BinaryTree<int, int> mapped = Map<int, int>(tree, [](const int& k) { return k * 2; });
        CHECK(mapped.GetCount() == 3);
        CHECK(mapped.ContainsKey(2) && mapped.ContainsKey(4) && mapped.ContainsKey(6));
        CHECK(mapped.Get(2) == 10);
        CHECK(mapped.Get(4) == 20);
        CHECK(mapped.Get(6) == 30);
    }

    void TestMapCollision() {
        BinaryTree<int, int> tree;
        tree.Add(1, 10);
        tree.Add(2, 20);
        CHECK_THROWS((Map<int, int>(tree, [](const int&) { return 0; })), std::invalid_argument);
    }

    void TestWhere() {
        BinaryTree<int, int> tree;
        for (int k = 1; k <= 10; ++k) {
            tree.Add(k, k * 10);
        }
        BinaryTree<int, int> filtered = Where<int, int>(tree,
                                                        [](const int& k, const int&) {
                                                            return k % 2 == 0;
                                                        });
        CHECK(filtered.GetCount() == 5);
        CHECK(!filtered.ContainsKey(1));
        CHECK(filtered.ContainsKey(2));
        CHECK(filtered.ContainsKey(10));
    }

    void TestReduce() {
        BinaryTree<int, int> tree;
        tree.Add(1, 0);
        tree.Add(2, 0);
        tree.Add(3, 0);
        int sum = Reduce<int, int, int>(
                tree,
                [](const int& acc, const int& k, const int&) { return acc + k; },
                0);
        CHECK(sum == 6);
    }

    void TestToStringFromString() {
        BinaryTree<int, int> tree;
        tree.Add(1, 10);
        tree.Add(2, 20);
        tree.Add(3, 30);

        std::string s = ToString<int, int>(tree);
        CHECK(s == "1:10|2:20|3:30");

        BinaryTree<int, int> back = FromString<int, int>(s);
        CHECK(back.GetCount() == 3);
        CHECK(back.Get(1) == 10);
        CHECK(back.Get(2) == 20);
        CHECK(back.Get(3) == 30);
    }

    void TestFromStringBadFormat() {
        CHECK_THROWS((FromString<int, int>("abc")), std::invalid_argument);
        CHECK_THROWS((FromString<int, int>("1")), std::invalid_argument);
        CHECK_THROWS((FromString<int, int>("1:10|1:20")), std::invalid_argument);
    }

    void TestExtractSubtree() {
        BinaryTree<int, int> tree;
        int keys[] = {5, 3, 8, 2, 4, 7, 9};
        for (int k : keys) {
            tree.Add(k, k * 10);
        }
        BinaryTree<int, int> sub = ExtractSubtree<int, int>(tree, 3);
        CHECK(sub.GetCount() == 3);
        CHECK(sub.ContainsKey(3));
        CHECK(sub.ContainsKey(2));
        CHECK(sub.ContainsKey(4));
        CHECK(!sub.ContainsKey(5));

        BinaryTree<int, int> empty = ExtractSubtree<int, int>(tree, 100);
        CHECK(empty.IsEmpty());
    }

    void TestContainsSubtree() {
        BinaryTree<int, int> tree;
        int keys[] = {5, 3, 8, 2, 4, 7, 9};
        for (int k : keys) {
            tree.Add(k, k * 10);
        }

        CHECK((ContainsSubtree<int, int>(tree, tree)));

        BinaryTree<int, int> sub3;
        sub3.Add(3, 30);
        sub3.Add(2, 20);
        sub3.Add(4, 40);
        CHECK((ContainsSubtree<int, int>(tree, sub3)));

        BinaryTree<int, int> fake;
        fake.Add(100, 1000);
        CHECK((!ContainsSubtree<int, int>(tree, fake)));

        BinaryTree<int, int> empty;
        CHECK((ContainsSubtree<int, int>(tree, empty)));
    }

    void TestMerge() {
        BinaryTree<int, int> a;
        a.Add(1, 10);
        a.Add(3, 30);

        BinaryTree<int, int> b;
        b.Add(2, 20);
        b.Add(4, 40);

        BinaryTree<int, int> merged = Merge<int, int>(a, b);
        CHECK(merged.GetCount() == 4);
        CHECK(merged.Get(1) == 10);
        CHECK(merged.Get(2) == 20);
        CHECK(merged.Get(3) == 30);
        CHECK(merged.Get(4) == 40);
    }

    void TestMergeCollision() {
        BinaryTree<int, int> a;
        a.Add(1, 10);
        BinaryTree<int, int> b;
        b.Add(1, 20);
        CHECK_THROWS((Merge<int, int>(a, b)), std::invalid_argument);
    }

    // проверка сохранения и чтения по формату
    void TestFormatSerialization() {
        BinaryTree<int, int> tree;
        tree.Add(5, 50);
        tree.Add(3, 30);
        tree.Add(8, 80);
        tree.Add(2, 20);

        // сохраняем по формату
        std::string s = tree.ToStringByFormat("{К}(Л)[П]");

        // проверяем что строка не пустая и содержит ключи
        CHECK(!s.empty());
        CHECK(s.find('5') != std::string::npos);
        CHECK(s.find('3') != std::string::npos);

        // читаем обратно
        BinaryTree<int, int> restored = FromStringByFormat<int, int>(s, "{К}(Л)[П]");

        // проверяем что размеры и ключи совпали
        CHECK(restored.GetCount() == tree.GetCount());
        CHECK(restored.ContainsKey(5));
        CHECK(restored.ContainsKey(3));
        CHECK(restored.ContainsKey(8));
        CHECK(restored.ContainsKey(2));
        CHECK(!restored.ContainsKey(100));
    }

    // проверка сохранения в виде списка пар
    void TestParentList() {
        BinaryTree<int, int> tree;
        tree.Add(5, 50);
        tree.Add(3, 30);
        tree.Add(8, 80);

        std::string s = tree.ToStringParentList();

        // проверяем что строка не пустая
        CHECK(!s.empty());

        // проверяем наличие связей (формат "родитель->ребенок")
        CHECK(s.find("5->3") != std::string::npos);
        CHECK(s.find("5->8") != std::string::npos);

        // у листьев нет исходящих связей
        CHECK(s.find("3->") == std::string::npos);
        CHECK(s.find("8->") == std::string::npos);
    }

    // проверка граничных случаев для форматов
    void TestFormatEdgeCases() {
        // пустое дерево
        BinaryTree<int, int> emptyTree;
        std::string s = emptyTree.ToStringByFormat("{К}(Л)[П]");
        CHECK(s.empty());

        BinaryTree<int, int> restoredEmpty = FromStringByFormat<int, int>("", "{К}(Л)[П]");
        CHECK(restoredEmpty.IsEmpty());

        // дерево только с корнем
        BinaryTree<int, int> single;
        single.Add(10, 100);
        std::string s2 = single.ToStringByFormat("{К}(Л)[П]");
        BinaryTree<int, int> restoredSingle = FromStringByFormat<int, int>(s2, "{К}(Л)[П]");
        CHECK(restoredSingle.GetCount() == 1);
        CHECK(restoredSingle.Get(10) == 0); // значение по умолчанию
    }

    using TestFunction = void (*)();

    void RunTest(const char* name, TestFunction test) {
        int before = g_testFailures;
        std::cout << name << " ... ";
        try {
            test();
        } catch (const std::exception& e) {
            ++g_testFailures;
            std::cout << "\n  неожиданное исключение " << e.what();
        } catch (...) {
            ++g_testFailures;
            std::cout << "\n  неожиданное исключение";
        }
        std::cout << (g_testFailures == before ? "ok" : "\n  провал") << "\n";
    }

}  // namespace

int RunAllTests() {
    g_testFailures = 0;
    g_testChecks = 0;

    RunTest("базовые операции", TestBinaryTreeBasic);
    RunTest("удаление", TestBinaryTreeRemove);
    RunTest("обходы", TestTraverse);
    RunTest("map", TestMap);
    RunTest("map коллизия", TestMapCollision);
    RunTest("where", TestWhere);
    RunTest("reduce", TestReduce);
    RunTest("ToString FromString", TestToStringFromString);
    RunTest("FromString плохой формат", TestFromStringBadFormat);
    RunTest("ExtractSubtree", TestExtractSubtree);
    RunTest("ContainsSubtree", TestContainsSubtree);
    RunTest("Merge", TestMerge);
    RunTest("Merge коллизия", TestMergeCollision);
    RunTest("формат строки", TestFormatSerialization);
    RunTest("список пар узел-родитель", TestParentList);
    RunTest("граничные случаи формата", TestFormatEdgeCases);

    std::cout << "проверок " << g_testChecks << " провалов " << g_testFailures << "\n";
    return g_testFailures;
}