#pragma once

// порядок обхода дерева
// К корень Л левое поддерево П правое поддерево
// КЛП это PreOrder
// КПЛ это ReversePreOrder
// ЛКП это InOrder
// ПКЛ это ReverseInOrder
// ЛПК это PostOrder
// ПЛК это ReversePostOrder
// вынесено в отдельный заголовок чтобы BinaryTree.h и TreeOperations.h
// не зависели друг от друга
enum class TraverseOrder {
    PreOrder,
    InOrder,
    PostOrder,
    ReversePreOrder,
    ReverseInOrder,
    ReversePostOrder
};