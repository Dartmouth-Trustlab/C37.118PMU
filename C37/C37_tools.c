#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>
#include "C37_tools.h"

size_t process(uint8_t *input, size_t inputsize){
  if(inputsize%2 != 0){
    return 0;
  }
  for(int i=0; i < inputsize; i++){
      //printf("%d\n", input[i]);
      if(input[i] < 58 & input[i] > 47){
        input[i] -= 48;
      } else if(input[i] > 96 & input[i] < 104){
        input[i] -= 87;
      } else {
        printf("%d, Aaah!\n", input[i]);
        return 0;
      }
      if(1 == i%2){
        input[i/2] += input[i];
      } else {
        input[i/2] = (input[i]*16);
      }
    }
  return inputsize/2;
}