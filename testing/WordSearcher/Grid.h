#include <vector>
#include <iostream>

#ifndef GRID_H
#define GRID_H

class Grid
{
public:
  Grid(std::string filename);
  int getWidth(void);
  int getHeight(void);
  char getLetter(int col, int row);
  void printGrid(void);
private:
  std::vector<std::vector<char>> letters;
};

#endif //GRID_H
