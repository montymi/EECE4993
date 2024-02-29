#include "Dictionary.h"
#include "Heap.h"
#include <iostream>
#include <vector>
#include <fstream>

/******* PRIVATE FUNCTION DECLARATIONS *********/

void convertToLower(char* str, int len);

Dictionary::Dictionary(void)
{
  wordlist = std::vector<std::string>();
  longestWord = 0;
}

void Dictionary::readWords(std::string filename)
{
  std::ifstream infile(filename);
  std::string next;
  while(getline(infile, next))
  {
    wordlist.push_back(next);
    convertToLower(&wordlist[wordlist.size()-1][0], wordlist[wordlist.size()-1].length());
    if (wordlist[wordlist.size()-1].length() > longestWord)
    {
      longestWord = wordlist[wordlist.size()-1].length();
    }
  }
}

void Dictionary::printWords(void)
{
  for(int i = 0; i < wordlist.size(); i++)
  {
    std::cout << wordlist[i] << std::endl;
  }
}

void Dictionary::selectionSort(void)
{
  if (wordlist.size() == 0)
  {
    return;
  }
  int current_word = 0;
  std::string first = wordlist[0], temp;
  for (int i = 0; i < wordlist.size(); i++)
  {
    for (int j = i; j < wordlist.size(); j++)
    {
      if (wordlist[j] < first)
      {
        first = wordlist[j];
        current_word = j;
      }
    }
    wordlist[current_word] = wordlist[i];
    wordlist[i] = first;
    first = wordlist[i + 1];
    current_word = i + 1;
  }
}



void Dictionary::quickSort(void)
{
  if (wordlist.size() == 0)
  {
    return;
  }
  this->quickSort(0,wordlist.size()-1);
}

int Dictionary::lookupWord(std::string word)
{
  int top = wordlist.size();
  int bottom = 0;
  int i = top / 2;
  while (wordlist[i] != word)
  {
    if(wordlist[i] > word)
    {
      top = i;
    }
    if (wordlist[i] < word)
    {
      bottom = i;
    }
    if (top - bottom <= 1)
    {
      return -1;
    }
    i = ((top - bottom) / 2) + bottom;
  }
  return i;
}

void Dictionary::heapSort(void) {
  Heap<std::string> maxheap;
  for (const auto& word: wordlist) {
    maxheap.insert(word);
  }
  maxheap.initializeMaxHeapSort();
  for (int i = wordlist.size() - 1; i > 0; i--) {
    wordlist[i] = maxheap.getItem(i);
  }
}

int Dictionary::getMax(void)
{
  return longestWord;
}

void Dictionary::quickSort(int low, int high)
{
  if (high <= low)
  {
    return;
  }
  int pivot = this->partition(low,high);
  
  this->quickSort(low, pivot-1);
  this->quickSort(pivot+1, high);
}

int Dictionary::partition(int low, int high)
{
  int i = low;
  std::string temp;
  std::string pivotStr = wordlist[high];
  for(int j = low; j < high; j++)
  {
    if(wordlist[j] < pivotStr)
    {
      temp = wordlist[i];
      wordlist[i] = wordlist[j];
      wordlist[j] = temp;
      i++;
    }
  }
  temp = wordlist[i];
  wordlist[i] = wordlist[high];
  wordlist[high] = temp;
  return i;
}

void convertToLower(char* str, int len)
{
  for (int i = 0; i < len; i++)
  {
    if (str[i] >= 'A' && str[i] <= 'Z')
    {
      str[i] += 32;
    }
  }
}
