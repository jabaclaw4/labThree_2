#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "BinaryTree.h"
#include "Tests.h"
#include "TraverseOrder.h"
#include "TreeOperations.h"

namespace {

    using IntTree = BinaryTree<int, int>;

    std::string ReadLine(const std::string& prompt) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::exit(0);
        }
        return line;
    }

    int ReadInt(const std::string& prompt) {
        while (true) {
            std::string line = ReadLine(prompt);
            try {
                size_t used = 0;
                int value = std::stoi(line, &used);
                if (used == line.size()) {
                    return value;
                }
            } catch (const std::exception&) {
            }
            std::cout << "нужно целое число\n";
        }
    }

    TraverseOrder ReadOrder() {
        std::cout << "порядок обхода\n";
        std::cout << "1 КЛП\n";
        std::cout << "2 КПЛ\n";
        std::cout << "3 ЛКП\n";
        std::cout << "4 ПКЛ\n";
        std::cout << "5 ЛПК\n";
        std::cout << "6 ПЛК\n";
        int choice = ReadInt("выбор: ");
        switch (choice) {
            case 1: return TraverseOrder::PreOrder;
            case 2: return TraverseOrder::ReversePreOrder;
            case 3: return TraverseOrder::InOrder;
            case 4: return TraverseOrder::ReverseInOrder;
            case 5: return TraverseOrder::PostOrder;
            case 6: return TraverseOrder::ReversePostOrder;
            default: return TraverseOrder::InOrder;
        }
    }

    struct AppState {
        IntTree tree;
    };

    void AddNode(AppState& state) {
        int key = ReadInt("ключ: ");
        int value = ReadInt("значение: ");
        try {
            state.tree.Add(key, value);
            std::cout << "добавлено\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка " << e.what() << "\n";
        }
    }

    void RemoveNode(AppState& state) {
        int key = ReadInt("ключ для удаления: ");
        try {
            state.tree.Remove(key);
            std::cout << "удалено\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка " << e.what() << "\n";
        }
    }

    void FindNode(AppState& state) {
        int key = ReadInt("ключ для поиска: ");
        if (state.tree.ContainsKey(key)) {
            std::cout << "найдено " << key << " -> " << state.tree.Get(key) << "\n";
        } else {
            std::cout << "не найдено\n";
        }
    }

    void PrintTraversal(const AppState& state) {
        if (state.tree.IsEmpty()) {
            std::cout << "дерево пустое\n";
            return;
        }
        TraverseOrder order = ReadOrder();
        std::cout << "обход: ";
        state.tree.Traverse(order, [](const int& key, const int& element) {
            std::cout << key << ":" << element << " ";
        });
        std::cout << "\n";
    }

    void DoMap(AppState& state) {
        try {
            IntTree mapped = Map<int, int>(state.tree, [](const int& k) {
                return 2 * k + 1;
            });
            state.tree = std::move(mapped);
            std::cout << "применено размер " << state.tree.GetCount() << "\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка map " << e.what() << "\n";
        }
    }

    void DoWhere(AppState& state) {
        int threshold = ReadInt("оставить ключи >= ");
        IntTree filtered = Where<int, int>(state.tree,
                                           [threshold](const int& k, const int&) {
                                               return k >= threshold;
                                           });
        state.tree = std::move(filtered);
        std::cout << "после where размер " << state.tree.GetCount() << "\n";
    }

    void DoReduce(const AppState& state) {
        int sum = Reduce<int, int, int>(
                state.tree,
                [](const int& acc, const int& k, const int&) {
                    return acc + k;
                },
                0);
        std::cout << "reduce сумма ключей " << sum << "\n";
    }

    void DoToString(const AppState& state) {
        TraverseOrder order = ReadOrder();
        std::string s = ToString<int, int>(state.tree, order);
        std::cout << "строка " << s << "\n";
    }

    void DoFromString(AppState& state) {
        std::cout << "формат key:element|key:element\n";
        std::string s = ReadLine("строка: ");
        try {
            state.tree = FromString<int, int>(s);
            std::cout << "прочитано размер " << state.tree.GetCount() << "\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка " << e.what() << "\n";
        }
    }

    void DoExtract(const AppState& state) {
        if (state.tree.IsEmpty()) {
            std::cout << "дерево пустое\n";
            return;
        }
        int rootKey = ReadInt("ключ корень поддерева: ");
        if (!state.tree.ContainsKey(rootKey)) {
            std::cout << "ключа нет\n";
            return;
        }
        IntTree sub = ExtractSubtree<int, int>(state.tree, rootKey);
        std::cout << "поддерево размер " << sub.GetCount() << ": ";
        sub.Traverse(TraverseOrder::PreOrder, [](const int& k, const int& v) {
            std::cout << k << ":" << v << " ";
        });
        std::cout << "\n";
    }

    void DoContains(const AppState& state) {
        std::cout << "формат key:element|key:element\n";
        std::string s = ReadLine("строка: ");
        try {
            IntTree sub = FromString<int, int>(s);
            bool result = ContainsSubtree<int, int>(state.tree, sub);
            std::cout << "содержится " << (result ? "да" : "нет") << "\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка " << e.what() << "\n";
        }
    }

    void ShowInfo(const AppState& state) {
        std::cout << "размер " << state.tree.GetCount() << "\n";
        std::cout << "высота " << state.tree.GetHeight() << "\n";
    }

    // обертка для сохранения по формату
    void DoToStringFormat(const AppState& state) {
        std::string s = state.tree.ToStringByFormat("{К}(Л)[П]");
        std::cout << "строка " << s << "\n";
    }

    // обертка для чтения по формату
    void DoFromStringFormat(AppState& state) {
        std::string s = ReadLine("строка: ");
        try {
            state.tree = FromStringByFormat<int, int>(s, "{К}(Л)[П]");
            std::cout << "прочитано размер " << state.tree.GetCount() << "\n";
        } catch (const std::exception& e) {
            std::cout << "ошибка " << e.what() << "\n";
        }
    }

    // обертка для сохранения списка пар
    void DoToStringParent(const AppState& state) {
        std::string s = state.tree.ToStringParentList();
        std::cout << "список пар " << s << "\n";
    }

    void PrintMenu() {
        std::cout << "\n1 добавить\n";
        std::cout << "2 удалить\n";
        std::cout << "3 найти\n";
        std::cout << "4 показать обход\n";
        std::cout << "5 map\n";
        std::cout << "6 where\n";
        std::cout << "7 reduce\n";
        std::cout << "8 сохранить в строку\n";
        std::cout << "9 прочитать из строки\n";
        std::cout << "10 извлечь поддерево\n";
        std::cout << "11 проверить вхождение поддерева\n";
        std::cout << "12 информация\n";
        std::cout << "13 запустить тесты\n";
        std::cout << "14 сохранить по формату\n";
        std::cout << "15 прочитать по формату\n";
        std::cout << "16 сохранить список пар\n";
        std::cout << "0 выход\n";
    }

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << std::fixed << std::setprecision(3);

    AppState state;
    while (true) {
        PrintMenu();
        int choice = ReadInt("выбор: ");
        if (choice == 0) break;
        try {
            switch (choice) {
                case 1: AddNode(state); break;
                case 2: RemoveNode(state); break;
                case 3: FindNode(state); break;
                case 4: PrintTraversal(state); break;
                case 5: DoMap(state); break;
                case 6: DoWhere(state); break;
                case 7: DoReduce(state); break;
                case 8: DoToString(state); break;
                case 9: DoFromString(state); break;
                case 10: DoExtract(state); break;
                case 11: DoContains(state); break;
                case 12: ShowInfo(state); break;

                case 13: {
                    int failures = RunAllTests();
                    std::cout << (failures == 0 ? "все тесты прошли\n" : "есть провалы\n");
                    break;
                }

                case 14: DoToStringFormat(state); break;
                case 15: DoFromStringFormat(state); break;
                case 16: DoToStringParent(state); break;
                default: std::cout << "нет такого пункта\n";
            }
        } catch (const std::exception& e) {
            std::cout << "неожиданная ошибка " << e.what() << "\n";
        }
    }
    return 0;
}