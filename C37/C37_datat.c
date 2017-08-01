#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>
#include "./C37_tools.h"

int g[2];

bool validate_C37_parser(HParseResult *p, void *u){
  h_pprint(stdout, h_act_flatten(p, NULL), 0, 1);
  return true;
}

int main(int argc, char *argv[]) {
    g[0] = 1;
    g[1] = 2;
    printf("%d %d\n", g[0], g[1]);
    int c;
    uint8_t input[102400];
    size_t inputsize = 0;
    H_RULE(rep1, h_uint16());
    H_RULE(rep2, h_uint8());
    H_RULE(fix, h_uint8());
    H_VRULE(C37_parser, h_sequence(fix, rep1, rep2, fix, h_end_p(), NULL));
    while ((c = fgetc(stdin)) != EOF)
    {
        input[inputsize++] = (char) c;
    }
    printf("%lu\n", inputsize);
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
