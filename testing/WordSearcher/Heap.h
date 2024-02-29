#include <vector>

template <typename T>
class Heap {
public:
    void initializeMaxHeapSort();
    T getItem(int i);
    void insert(T value);
    int size();
private:
    std::vector<T> item_list;
    int parent(int i);
    int child_left(int i);
    int child_right(int i);
    void maxHeapify(int size, int i);
    void maxHeapSort();
};

