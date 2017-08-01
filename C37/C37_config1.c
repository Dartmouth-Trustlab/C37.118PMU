#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>
#include "C37_tools.h"

const HParser *Well_formed;
const HParser *Prefix;
int matrix[][6];
int used_bytes = 0;
int currPMU = 0;
int currDOPT = 1;
int currAOPT = 2;
int currPOPT = 2;

int main(){
  int first = 0;
  FILE *file = fopen("configtest.txt", "r");
  int c = fgetc(file);
  uint8_t input[102400];
  size_t inputsize = 0;
  while (c != EOF){
      if(10 != c){
        input[inputsize] = (char) c;
        inputsize++;
      } else {
        if(first == 0){
          printf("configing\n");
          config(input, inputsize);
        } else {
          printf("dataing\n");
          data(input, inputsize);
        }
        inputsize = 0;
        first++;
      }
      c = fgetc(file);
    }
  printf("Read %d packets\n", first);
  return 0;
}
