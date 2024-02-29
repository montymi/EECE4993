#include <vector>
#include <iostream>

#ifndef DICTIONARY_H
#define DICTIONARY_H

class Dictionary
{
public:
  Dictionary(void);
  void readWords(std::string filename);
  void printWords(void);
  void selectionSort(void);
  void quickSort(void);
  int lookupWord(std::string word);
  int getMax(void);
  void heapSort(void);
private:
  void quickSort(int low, int high);
  int partition(int low, int high);
  std::vector<std::string> wordlist;
  int longestWord;
};

#endif //DICTIONARY_H
