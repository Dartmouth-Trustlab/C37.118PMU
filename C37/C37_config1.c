#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>
#include "C37_tools.h"

const HParser *Well_formed;
const HParser *Prefix;
int matrix[][6];
int currPMU = 0;
int currDOPT = 0;
int currAOPT = 0;
int currPOPT = 0;
int currTYPE = 0;
int fsize = 0;
int getCON = 1;
int getDATA = 1;
int getHEAD = 0;

int main(){
  int result = 0;
  int total = 0;
  int validTotal = 0;
  FILE *file = fopen("fulltest.txt", "r");
  int c = fgetc(file);
  uint8_t input[102400];
  size_t inputsize = 0;
  while (c != EOF){
      if(10 != c){
        input[inputsize] = (char) c;
        inputsize++;
      } else {
        inputsize = process(input, inputsize);
        if(choose(input, inputsize)){
          if(1 == currTYPE){
            if(1 == getCON){
              result = config(input, inputsize);
            } else {
              result = -1;
              printf("Warning: Config recieved without being requested. Ignoring.\n");
            }
          } else if (2 == currTYPE){
            if(1 == getDATA){
              result = data(input, inputsize);
            } else {
              result = -1;
              printf("Warning: Data recieved without being requested. Ignoring.\n");
            }
          } else if (3 == currTYPE){
            result = command(input, inputsize);
          } else if (4 == currTYPE){
            if(1 == getHEAD){
              result = header(input, inputsize);
            } else {
              result = -1;
              printf("Warning: Header recieved without being requested. Ignoring.\n");
            }
          } else {
            result = -2;
            printf("Error: Invalid type %d ???\n", currTYPE);
          }
        }
        inputsize = 0;
        total++;
        if(0 == result){
          validTotal++;
        }
      }
      c = fgetc(file);
    }
  printf("Read %d packets, %d valid\n", total, validTotal);
  return 0;
}
