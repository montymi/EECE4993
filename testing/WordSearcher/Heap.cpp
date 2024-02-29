#include "Heap.h"
#include <iostream>
#include <algorithm>

template <typename T>
void Heap<T>::initializeMaxHeapSort() {
    maxHeapSort();
}

template <typename T>
T Heap<T>::getItem(int i) {
    return item_list[i];
}

template <typename T>
void Heap<T>::insert(T value) {
    item_list.push_back(value);
}

template <typename T>
int Heap<T>::size() {
    return item_list.size();
}

template <typename T>
int Heap<T>::parent(int i) {
    return (i - 1) / 2;
}

template <typename T>
int Heap<T>::child_left(int i) {
    return 2 * i + 1;
}

template <typename T>
int Heap<T>::child_right(int i) {
    return 2 * i + 2;
}

template <typename T>
void Heap<T>::maxHeapify(int size, int i) {
    int largest = i;
    int l = child_left(i);
    int r = child_right(i);
    if (l < size && item_list[l] > item_list[largest]) { largest = l; }
    if (r < size && item_list[r] > item_list[largest]) { largest = r; }
    if (largest != i) {
        std::swap(item_list[i], item_list[largest]);
        maxHeapify(size, largest);
    }
}

template <typename T>
void Heap<T>::maxHeapSort() {
    int N = this->size();
    for (int i = N / 2 - 1; i >= 0; i--)
        maxHeapify(N, i);
    for (int i = N - 1; i > 0; i--) {
        std::swap(item_list[0], item_list[i]);
        maxHeapify(i, 0);
    }
}

// Explicit instantiation of the template class for the types you plan to use
template class Heap<std::string>;  // Example instantiation for int
template class Heap<double>;  // Example instantiation for double
// Add more instantiations as needed for other types

