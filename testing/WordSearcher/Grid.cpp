#include "Grid.h"
#include <iostream>
#include <vector>
#include <fstream>

Grid::Grid(std::string filename)
{
  int width, height;
  std::ifstream infile(filename);
  char inchar;
  letters = {};
  infile >> width;
  infile >> height;
  for(int i = 0; i < width; i++)
  {
    letters.push_back({});
    for(int j = 0; j < height; j++)
    {
      infile >> inchar;
      letters[i].push_back(inchar);
    }
  }
}

int Grid::getWidth(void)
{
  return letters.size();
}

int Grid::getHeight(void)
{
  return letters[0].size();
}

char Grid::getLetter(int col, int row)
{
  if (letters.size() <= col || letters[0].size() <= row)
  {
    return -1;
  }
  return letters[col][row];
}

void Grid::printGrid(void)
{
  for(int i = 0; i < letters.size(); i++)
  {
    for(int j = 0; j < letters[0].size(); j++)
    {
      std::cout << letters[i][j] << " ";
    }
    std::cout << std::endl;
  }
}
