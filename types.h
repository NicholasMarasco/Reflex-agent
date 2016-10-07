// structure to contain x and y coordinates
typedef struct Point
{
  int x;
  int y;
} Point;

// structure to hold input file data
typedef struct Finput
{
  int n;
  int fNum;
  int dNum;
  Point goal;
  Point **furn;
  Point **dirt;
} Finput;
