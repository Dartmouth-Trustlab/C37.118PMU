#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>

const HParser *C37_parser;
const HParser *C37_parser2;
const int PHNMR = 1;
const int ANNMR = 1;
const int DGNMR = 1;
const int OPT = 1;

bool validate_SYNC_parse(const HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43521 == sync){
    return true;
  } else {
    printf("Error: invalid sync bytes. Only 0xaa01 is acceptable.\n");
    return false;
  }
}

bool validate_Final_parse(const HParseResult *p, void *user_data){
  uint16_t chk = h_act_last(p, NULL)->uint;
  printf("%hu\n", chk);
  h_pprint(stdout, h_act_last(p, NULL), 0, 1);
  return (chk == 32984);
}

HParsedToken *act_FRAMESIZE(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  h_pprint(stdout, ret, 0, 1);
  printf("%d\n", val);
  return ret;
}

HParsedToken *act_Repeated_Fields(const HParseResult *p, void *user_data){
  HCountedArray *t = p->ast->seq;
  HParsedToken *ret = h_act_flatten(p, NULL);
  printf("%zu\n", t->used);
  h_pprint(stdout, ret, 0, 1);
  return ret;
}

HParsedToken *act_TEMP_parse(const HParseResult *p, void *user_data){
  HCountedArray *t = p->ast->seq;
  HParsedToken *ret = h_act_flatten(p, NULL);
  h_pprint(stdout, ret, 0, 1);
  return ret;
}

HParsedToken *act_Final_parse(const HParseResult *p, void *user_data){
  HCountedArray *t = p->ast->seq;
  HParsedToken *ret = h_act_flatten(p, NULL);
  //h_pprint(stdout, ret, 0, 1);
  return ret;
}

void init_parser(){
  //For readability, used for 16 & 32 bit words
  H_RULE(Bytes2, h_bits(16, false));
  H_RULE(Bytes4, h_bits(32, false));

  // SYNC
  // Use "h_bits(len, sign)" to recognize the two SYNC bytes

  H_VRULE(SYNC_parse, Bytes2);
  // This will parse the first 9 bits of SYNC, then the frame-identifier, then the last 4 bits that we don't care about
  
  H_RULE(FRAMESIZE, Bytes2);
  // This will be FRAMESIZE
  // FRAMESIZE is the number of BYTES in the frame
  // We will need to check this against PHNMR, ANNMR, and DGNMR
  // FRAMESIZE = 20 + NUM_PMU * (30 + 16 * (PH + AN + (16 * DG))) + 4 * (PH + AN + DG) + 4
  

  H_RULE(IDCODE_1, Bytes2);
  // IDCODE
  // Just identifies the data stream
  // I don't think that we'll actually have to do anything with this one


  H_RULE(SOC, Bytes4);
  // SOC
  // Time-stamp
  // I think it's the number of seconds??
  // We shouldn't have to check it against anything


  H_RULE(FRACSEC, Bytes4);
  // FRACSEC
  // More time-information


  // THE FRAMES BECOMES DISTINCT AFTER THIS POINT

  //Data frame format:
  //*Repeated:
  //|Stat 2 bytes
  //|Phasors PHNMR*(4/8) bytes
  //|Freq 2/4 bytes
  //|Dfreq 2/4 bytes
  //|Analog ANNMR*(2/4) bytes
  //|Digital DGNMR*2 bytes
  //*End Repeated
  //CHK 2 bytes

  //Commented out rules will be used for correctness checks,
  // active rules are well-formedness checks.
  /*
  H_RULE(STAT, Bytes2);

  H_RULE(PHASORS, h_many(h_xor(Bytes4, h_sequence(Bytes4, Bytes4, NULL))));
  //H_RULE(PHASORS, h_bits(PHNMR*OPT*32, false));

  H_RULE(FREQ, h_xor(Bytes2, Bytes4));
  //H_RULE(FREQ, h_bits(OPT*16, false));

  H_RULE(DFREQ, h_xor(Bytes2, Bytes4));
  //H_RULE(DFREQ, h_bits(OPT*16, false));

  H_RULE(ANALOG, h_many(Bytes2));
  //H_RULE(ANALOG, h_bits(ANNMR*OPT*16, false));

  H_RULE(DIGITAL, h_many(Bytes2));
  //H_RULE(DIGITAL, h_bits(DGNMR*16, false));

  H_ARULE(Repeated_Fields, h_many1(h_sequence(STAT, NULL))); 
  //H_ARULE(Repeated_Fields, h_many1(h_sequence(STAT, PHASORS, FREQ, DFREQ, ANALOG, DIGITAL, NULL))); 
*/
  H_RULE(CHK, h_sequence(Bytes2, h_end_p(), NULL));

  H_RULE(LAZY_REP, h_sequence(h_repeat_n(Bytes2, 3), h_many(Bytes2), h_end_p(), NULL));
  //HParser *Repeat_the_Fields = h_repeat_n(Repeated_Fields, NUM_PMU);
  // YOU NEED TO GET THE ACTUAL VALUE IN NUM_PMU

/*
  int Actual_Size =  20 + NUM_PMU * (30 + 16 * (PH + AN + (16 * DG))) + 4 * (PH + AN + DG) + 4;

  
  HParser *Size_Check = h_int_range(FRAMESIZE, Actual_Size, Actual_Size);
  // This checks FRAMESIZE against the actual size of the input


  HParser *Configuration_Parse = h_sequence(SYNC_parse, FRAMESIZE, IDCODE, SOC, FRACSEC, TIME_BASE, NUM_PMU, Repeat_the_Fields, DATA_RATE, CHK);
*/
  //For testing
  H_ARULE(TEMP_parse, h_sequence(SYNC_parse, FRAMESIZE, IDCODE_1, SOC, FRACSEC, NULL));

  H_VARULE(Final_parse, h_sequence(SYNC_parse, FRAMESIZE, IDCODE_1, SOC, FRACSEC, h_xor(LAZY_REP, CHK), NULL));

  C37_parser = Final_parse;
  
}



int main(int argc, char *argv[]) {
    init_parser();

    uint8_t input[102400];
    size_t inputsize;


    inputsize = fread(input, 1, sizeof(input), stdin);
    printf("%lu\n", inputsize);
    HParseResult *result = h_parse(C37_parser, input, inputsize);
    if(result) {
        printf("yay!\n");
    } else {
        printf("boo!\n");
    }
}
