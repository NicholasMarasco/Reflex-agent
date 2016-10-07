#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "types.h"

#define MAX_LINE_SIZE 512

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

// enum containing the possible actions
typedef enum {FW, TR, TL, VU, TO, NO} action;

// global variables
Point curLoc = {0,0};
int bearing = UP;
int turns = 0;
int maxTurns;
int n;
int score;
FILE *fout;

// update functions
int updatePercept(int*,char**,Finput);
void updateMap(char**,action,int*);
action decide(int*);
char getChar(int);

// get directions
Point getFW();
Point getBW();
Point getLT();
Point getRT();

// building functions
Finput handleFile(char*);
char** buildMap(Finput);

// destroying functions
void freeArray(Point**,int);
void burnMap(char**);

// debug/printing functions
void draw(char**,int*,action);
void printFinput(Finput);

int main(int ARGC,char *ARGV[]){
  srand(time(NULL));
  score = 0;
  action next = NO;
  Finput fileInputs = handleFile(ARGV[1]);
  n = fileInputs.n;
  maxTurns = 10 * n * n;
  char **map = buildMap(fileInputs);
  int percept[11] = {};
  int *perPtr = percept;
  int remove;

  // start the "game loop"
  while(turns < maxTurns){
    remove = updatePercept(perPtr,map,fileInputs);
    draw(map,perPtr,next);
    if(next == 4)
      break;
    next = decide(perPtr);
    if(remove != fileInputs.dNum){
      free(fileInputs.dirt[remove]);
      fileInputs.dirt[remove] = NULL;
    }
    updateMap(map,next,perPtr);
    turns++;
  }
  if(turns == maxTurns){
    score -= 1000;
    draw(map,perPtr,next);
  }

  freeArray(fileInputs.furn,fileInputs.fNum);
  free(fileInputs.dirt);
  burnMap(map);
  return 0;
}

// update the map with the next action
// parameters:
//   map - the 2D character array of the current map
//   next - action chosen by the agent
//   percept - array representation of the percept vector
void updateMap(char** map, action next, int *percept){
  if(next == 0){
    Point fw = getFW();
    if(!(fw.x >= 0 && fw.x < n) || !(fw.y >= 0 && fw.y < n))
      percept[0] = 1;
    else if(map[fw.x][fw.y] == 'X')
      percept[0] = 1;
    else{
      map[fw.x][fw.y] = map[curLoc.x][curLoc.y];
      if(curLoc.x == 0 && curLoc.y == 0)
        map[0][0] = 'H';
      else
        map[curLoc.x][curLoc.y] = ' ';
      curLoc.x = fw.x;
      curLoc.y = fw.y;
    }
    score -= 1;
  }
  else if(next == 1){
    map[curLoc.x][curLoc.y] = getChar(RIGHT);
    bearing = (bearing + 1) % 4;
    score -= 1;
  }
  else if(next == 2){
    map[curLoc.x][curLoc.y] = getChar(LEFT);
    if(bearing)
      bearing -= 1;
    else
      bearing = LEFT;
    score -= 1;
  }
  else if(next == 3)
    score += 99;
  else
    score -= 1;
}

// make a decision
// parameters:
//   percept - percept vector
// returns: action to chosen by the agent
action decide(int *percept){
  action next;
  // dirt actions are next on the list
  if(percept[1])
    return next = VU;
  if(percept[2])
    return next = FW;
  if(percept[4])
    return next = TL;
  if(percept[5])
    return next = TR;
  if(percept[3])
    return next = TR;

  // goal related actions take priority
  if(percept[6])
    return next = TO;
  if(percept[7])
    return next = FW;
  if(percept[9])
    return next = TL;
  if(percept[10])
    return next = TR;
  if(percept[8])
    return next = TR;

  // now we deal with nothing and furniture
  int bump = percept[0];
  percept[0] = 0;
  int r = rand() % 100;

  if(r < 50){
    if(bump){
      r = rand() % 50;
      if(r < 25)
        next = TL;
      else
        next = TR;
    }
    else
      next = FW;
  }
  else if(r < 75)
    next = TL;
  else
    next = TR;

  return next;
}

// update percept vector
// parameters:
//   percept - percept vector
//   map - current state of the world
//   finputs - structure containing the file input values
// returns: array postion of the dirt to remove
int updatePercept(int *percept,char **map,Finput finputs){

  int i;
  for(i = 1; i < 11; i++)
    percept[i] = 0;

  // set up directions for each bearing
  Point fw, bw, lt, rt;
  fw = getFW();
  bw = getBW();
  lt = getLT();
  rt = getRT();

  // check under
  for(i = 0; i < finputs.dNum; i++){
    if(finputs.dirt[i] != NULL){
      if((curLoc.x == finputs.dirt[i]->x) && (curLoc.y == finputs.dirt[i]->y)){
        percept[1] = 1;
        break;
      }
    }
  }
  if(curLoc.x == finputs.goal.x && curLoc.y == finputs.goal.y)
    percept[6] = 1;

  if((0 <= fw.x && fw.x < n) && (0 <= fw.y && fw.y < n)){
    if(map[fw.x][fw.y] == '#')
      percept[2] = 1;
    else if(map[fw.x][fw.y] == 'G')
      percept[7] = 1;
  }

  if((0 <= bw.x && bw.x < n) && (0 <= bw.y && bw.y < n)){
    if(map[bw.x][bw.y] == '#')
      percept[3] = 1;
    else if(map[bw.x][bw.y] == 'G')
      percept[8] = 1;
  }

  if((0 <= lt.x && lt.x < n) && (0 <= lt.y && lt.y < n)){
    if(map[lt.x][lt.y] == '#')
      percept[4] = 1;
    else if(map[lt.x][lt.y] == 'G')
      percept[9] = 1;
  }

  if((0 <= rt.x && rt.x < n) && (0 <= rt.y && rt.y < n)){
    if(map[rt.x][rt.y] == '#')
      percept[5] = 1;
    else if(map[rt.x][rt.y] == 'G')
      percept[10] = 1;
  }
  return i;
}

// take the file inputs and build the initial map
//   parameters:
//     fileInputs - struct containing the file inputs
//   returns:
//     2D array representation of the map
char** buildMap(Finput fileInputs){
  // allocate memory for the map
  int n = fileInputs.n;
  char **arr = malloc(n*sizeof(char*));
  int i;
  for(i = 0; i < n; i++)
    arr[i] = malloc(n);

  // fill grid with empty spaces
  int j;
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++)
      arr[i][j] = ' ';

  // initialize agent, goal, and object locations
  arr[0][0] = '^';
  arr[fileInputs.goal.x][fileInputs.goal.y] = 'G';
  for(i = 0; i < fileInputs.fNum; i++)
    arr[fileInputs.furn[i]->x][fileInputs.furn[i]->y] = 'X';
  for(i = 0; i < fileInputs.dNum; i++)
    arr[fileInputs.dirt[i]->x][fileInputs.dirt[i]->y] = '#';

//   printf("arr[0][0]: %c\n",arr[0][0]);
  return arr;
}

// calculate the forward offset
// returns: Point containing forward offsets
Point getFW(){
  Point fw;
  switch(bearing){
    case UP:
      fw.x = curLoc.x+1;
      fw.y = curLoc.y;
      break;
    case RIGHT:
      fw.x = curLoc.x;
      fw.y = curLoc.y+1;
      break;
    case DOWN:
      fw.x = curLoc.x-1;
      fw.y = curLoc.y;
      break;
    case LEFT:
      fw.x = curLoc.x;
      fw.y = curLoc.y-1;
      break;
  }
  return fw;
}

// calculate the backward offset
// returns: Point containing backward offsets
Point getBW(){
  Point bw;
  switch(bearing){
    case UP:
      bw.x = curLoc.x-1;
      bw.y = curLoc.y;
      break;
    case RIGHT:
      bw.x = curLoc.x;
      bw.y = curLoc.y-1;
      break;
    case DOWN:
      bw.x = curLoc.x+1;
      bw.y = curLoc.y;
      break;
    case LEFT:
      bw.x = curLoc.x;
      bw.y = curLoc.y+1;
      break;
  }
  return bw;
}

// calculate the left offset
// returns: Point containing left offsets
Point getLT(){
  Point lt;
  switch(bearing){
    case UP:
      lt.x = curLoc.x;
      lt.y = curLoc.y-1;
      break;
    case RIGHT:
      lt.x = curLoc.x+1;
      lt.y = curLoc.y;
      break;
    case DOWN:
      lt.x = curLoc.x;
      lt.y = curLoc.y+1;
      break;
    case LEFT:
      lt.x = curLoc.x-1;
      lt.y = curLoc.y;
      break;
  }
   return lt;
}

// calculate the right offset
// returns: Point containing right offsets
Point getRT(){
  Point rt;
  switch(bearing){
    case UP:
      rt.x = curLoc.x;
      rt.y = curLoc.y+1;
      break;
    case RIGHT:
      rt.x = curLoc.x-1;
      rt.y = curLoc.y;
      break;
    case DOWN:
      rt.x = curLoc.x;
      rt.y = curLoc.y-1;
      break;
    case LEFT:
      rt.x = curLoc.x+1;
      rt.y = curLoc.y;
      break;
  }
  return rt;
}

// get the character change from the current bearing
// parameters:
//   target - direction turning (left or right)
// returns: character showing the proper orientation
char getChar(int target){
  switch(target){
    case RIGHT:
      switch(bearing){
        case UP:
          return '>';
        case RIGHT:
          return 'v';
        case DOWN:
          return '<';
        case LEFT:
          return '^';
      }
    case LEFT:
      switch(bearing){
        case UP:
          return '<';
        case RIGHT:
          return '^';
        case DOWN:
          return '>';
        case LEFT:
          return 'v';
      }
  }
}

// read in the file and grab the input information
//   parameter:
//     fileName - name of the input file
//   returns:
//     struct Finput - contains all of the file inputs
Finput handleFile(char *fileName){
  // declare the local variables
  //   i - loop counter
  //   n - map size
  //   fNum - number of furnitures
  //   dNum - number of dirts
  //   pX, pY - temporary storage for point values
  //   goal - point for goal space
  //   furn - array for the furniture locations
  //   dirt - array for the dirt locations
  //   buff - buffer for the file lines
  int i, n, fNum, dNum, pX, pY;
  Point goal;
  Point **furn;
  Point **dirt;
  char buff[MAX_LINE_SIZE];

  // open log file and initialize
  fout = fopen("prog1_log.txt","w");
  fprintf(fout,"Time <B Du Df Db Dr Dl Gu Gf Gb Gr Gl>     Action      Score\n");
  fprintf(fout,"--------------------------------------     ------      -----\n");

  FILE *input = fopen(fileName,"r");

  if (input != NULL){
    char const *place;
    int offset;

    // grab lines and store values in variables
    fgets(buff, MAX_LINE_SIZE, input);
    sscanf(buff, " %d %d %d", &n, &fNum, &dNum);

    furn = calloc(fNum, sizeof(Point*));
    dirt = calloc(dNum, sizeof(Point*));

    fgets(buff, MAX_LINE_SIZE, input);
    sscanf(buff, " %d %d",&pY,&pX);
    goal.x = pX; goal.y = pY;

    fgets(buff, MAX_LINE_SIZE, input);
    i = 0;
    for(place = buff;
        sscanf(place," %d %d%n",&pY,&pX,&offset) == 2; place += offset){
      furn[i] = malloc(sizeof(Point));
      furn[i]->x = pX;
      furn[i]->y = pY;
      i++;
    }
//     printf("furn[0] : %p\n",furn[0]);

    fgets(buff, MAX_LINE_SIZE, input);
    i = 0;
    for(place = buff;
        sscanf(place," %d %d%n",&pY,&pX,&offset) == 2; place += offset){
      dirt[i] = malloc(sizeof(Point));
      dirt[i]->x = pX;
      dirt[i]->y = pY;
      i++;
    }
  }
//   printf("dirt[0]: %p\n",dirt[0]);

  fclose(input);
  Finput f = {n, fNum, dNum, goal, furn, dirt};
//   printFinput(f);
  return f;
}

// draw the current board
void draw(char **map, int *percept, action last){
  int i;

  // top line
  for(i = 0; i < n; i++)
    printf("+-");
  printf("+\n");

  // rows, i flipped to make bottom left 0,0
  for(i = n-1; i >= 0; i--){
    printf("|");
    int j;
    for(j = 0; j < n-1; j++)
      printf("%c ",map[i][j]);
    printf("%c|\n",map[i][n-1]);

    if(i){
      for(j = 0; j < n; j++)
        printf("+ ");
      printf("+\n");
    }
  }

  // bottom line
  for(i = 0; i < n; i++)
    printf("+-");
  printf("+\n");

  // print percept stuff
  printf("T  DU  DF  DB  DL  DR  GU  GF  GB  GL  GR\n");
  printf("%d",percept[0]);
  for(i = 1; i < 11; i++)
    printf("   %d",percept[i]);
  printf("\n");

  printf("Score: %d\n",score);
  printf("Last action: ");
  char *act;
  switch(last){
    case 0:
      printf("Fwd\n");
      act = "MOVE FORWARD";
      break;
    case 1:
      printf("Turn Rt\n");
      act = "TURN RIGHT\t";
      break;
    case 2:
      printf("Turn Lft\n");
      act = "TURN LEFT\t";
      break;
    case 3:
      printf("Vac\n");
      act = "VACUUM\t\t\t";
      break;
    case 4:
      printf("Pwr off\n");
      act = "POWER OFF\t";
      break;
    default:
      printf("Nothing\n");
      act = "NO ACTION\t\t";
  }

  // write to log file
  fprintf(fout,"%d\t\t<%d",turns,percept[0]);
  for(i = 1; i < 11; i++)
    fprintf(fout,", %d",percept[i]);
  fprintf(fout,">  %s",act);
  fprintf(fout,"\t\t%d\n",score);
}

// print out the file input variables
// parameters:
//   fileInputs - structure containing all of the file input values
void printFinput(Finput fileInputs){
  int i;
  printf("n: %d\n",fileInputs.n);
  printf("fNum: %d\n",fileInputs.fNum);
  printf("dNum: %d\n",fileInputs.dNum);
  printf("Goal: (%d %d)\n",fileInputs.goal.x,fileInputs.goal.y);
  for(i = 0; i < fileInputs.fNum; i++)
    printf("furn%d = (%d %d)\n",i+1,fileInputs.furn[i]->x,fileInputs.furn[i]->y);
  for(i = 0; i < fileInputs.dNum; i++)
    printf("dirt%d = (%d %d)\n",i+1,fileInputs.dirt[i]->x,fileInputs.dirt[i]->y);
}

// destroy the map
// parameters:
//   map - current state of the map
void burnMap(char **map){
  int i;
  for(i = 0; i < n; i++)
    free(map[i]);
  free(map);
}

// free all of the memory allocated in the array
// parameters:
//   arr - array of pointers to free
//   size - number of elements in the array
void freeArray(Point **arr,int size){
  int i;
  for(i = 0; i < size; i++)
    free(arr[i]);
  free(arr);
}







