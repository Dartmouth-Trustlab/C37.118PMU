#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>

const HParser *Well_formed;
const HParser *Prefix;
const int matrix[4][6] = {{61,7,3,0,1,4},{62,7,14,8,1,0},{63,7,14,4,1,0},{64,7,14,0,1,0}};
const int idcode = 60;
int used_bytes = 0;
int currPMU = 0;
int currDOPT = 1;
int currAOPT = 2;
int currPOPT = 2;


int main(int argc, char *argv[]) {
  FILE *file = fopen("datatest.txt", "r");
  int c = fgetc(file);
  uint8_t input[102400];
  size_t inputsize = 0;
  printf("test\n");
  while (c != EOF){
      if(10 != c){
        input[inputsize] = (char) c;
        inputsize++;
      } else {
        printf("dataing\n");
        data(input, inputsize);
        inputsize = 0;
      }
      c = fgetc(file);
    }
  printf("%d\n", matrix[0][0]);
  return 0;
}
