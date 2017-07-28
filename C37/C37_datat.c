#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>

bool validate_C37_parser(HParseResult *p, void *u){
  h_pprint(stdout, h_act_flatten(p, NULL), 0, 1);
  return true;
}

size_t process(uint8_t *input, size_t inputsize){
  if(inputsize%2 != 0){
    return 0;
  }
  for(int i=0; i < inputsize; i++){
      printf("%d\n", input[i]);
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

int main(int argc, char *argv[]) {
    uint8_t input[102400];
    size_t inputsize;
    H_RULE(rep1, h_uint16());
    H_RULE(rep2, h_uint8());
    H_RULE(fix, h_uint8());
    H_VRULE(C37_parser, h_sequence(fix, rep1, rep2, fix, h_end_p(), NULL));
    inputsize = fread(input, 1, sizeof(input), stdin);
    inputsize = process(input, inputsize);
    printf("%lu\n", inputsize);
    HParseResult *result = h_parse(C37_parser, input, inputsize);
    if(result) {
        printf("yay!\n");
    } else {
        printf("boo!\n");
    }

    H_RULE(start, fix);
    HParseResult *r2 = h_parse(start, input, inputsize);
    int bytes = 0;

    if(r2){
      for(int i=1; i<3; i++){
        H_RULE(junk, h_bits(bytes*8, false));
        if(i==1){
          H_RULE(rep21, h_uint16());
          HParseResult *r3 = h_parse(h_sequence(start, junk, rep21, NULL), input, inputsize);
          bytes +=2;
          if(r3){
            printf("yay3!\n");
          }
        } else {
          H_RULE(rep21, h_uint8());
          HParseResult *r4 = h_parse(h_sequence(start, junk, rep21, NULL), input, inputsize);
          bytes +=1;
          if(r4){
            printf("yay4!\n");
          }
        }
        printf("%d\n", i);
      }
      H_RULE(junk1, h_bits(bytes*8, false));
      HParseResult *r5 = h_parse(h_sequence(start, junk1, fix, h_end_p(), NULL), input, inputsize);
      if(r5){
        printf("yay5!\n");
      }
    }
}