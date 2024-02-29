#include <iostream>
#include <vector>
#include "Dictionary.h"
#include "Grid.h"

const int MIN_LENGTH = 5;

void search(int algorithm);
void findMatches(Dictionary dict, Grid grid);
void searchDirections(Dictionary dict, Grid grid, int row, int col);

int main(){
  search(1);
  search(2);
}

void findMatches(Dictionary dict, Grid grid)
{
  int width = grid.getWidth(), height = grid.getHeight();
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      searchDirections(dict, grid, row, col);
    }
  }
}

void searchDirections(Dictionary dict, Grid grid, int row, int col)
{
  // directional arrays
  int x[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int y[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  for (int direction = 0; direction < 8; direction++) {
    std::string current = "";
    int rd = row;
    int cd = col;

    while (current.length() <= dict.getMax()) {
      current += grid.getLetter((((cd % grid.getWidth()) + grid.getWidth()) % grid.getWidth()), (((rd % grid.getHeight()) + grid.getHeight()) % grid.getHeight()));
      if (current.length() >= MIN_LENGTH) {
        if (dict.lookupWord(current) != -1) {std::cout << "MATCH: " << current << std::endl;}
      }
      rd += x[direction];
      cd += y[direction];
    }
  }
}

void search(int algorithm)
{
  Dictionary dict = Dictionary();
  dict.readWords("dictionary.txt");
  
  switch(algorithm)
  {
    case 0:
      	dict.selectionSort();
      	break;
    case 1:
 	dict.quickSort();
      	break;
    case 2:
      	dict.heapSort(); 
      	break;
    default:
      	dict.quickSort();
      	break;
  }
  std::string filename;
  std::cout << "Please enter the name of the file containing the grid: ";
  std::cin >> filename;
  Grid grid = Grid(filename);
  findMatches(dict, grid);
}
