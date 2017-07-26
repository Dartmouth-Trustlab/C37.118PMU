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

bool validate_REP(HParseResult *p, void *user_data){
  h_pprint(stdout, h_act_flatten(p, NULL), 0, 1);
  return true;
}

bool validate_SYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43521 == sync){
    printf("%d\n", matrix[1][1]);
    return true;
  } else {
    printf("Error: invalid sync bytes. Only 0xaa01 is acceptable.\n");
    return false;
  }
}

bool validate_IDCODE_val(HParseResult *p, void *user_data){
  uint16_t id = p->ast->uint;
  if(idcode == id){
    return true;
  } else {
    printf("Error: expected idcode %d\n", idcode);
    return false;
  }
}

bool validate_Final_check(HParseResult *p, void *user_data){
  uint16_t framesize = h_act_index(1, p, NULL)->uint;
  printf("framesize = %hu\n", framesize);
  uint16_t idcode = h_act_index(2, p, NULL)->uint;
  printf("idcode = %hu\n", idcode);
  uint16_t soc = h_act_index(3, p, NULL)->uint;
  printf("soc = %hu\n", soc);
  uint16_t fracsec = h_act_index(4, p, NULL)->uint;
  printf("fracsec = %hu\n", fracsec);
  uint16_t chk = h_act_last(p, NULL)->uint;
  printf("chk = %hu\n", chk);
  return true;
}

HParsedToken *act_FRAMESIZE(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  //h_pprint(stdout, ret, 0, 1);
  printf("%d\n", val);
  return ret;
}

void init_parser(){
  // SYNC
  // Use "h_bits(len, sign)" to recognize the two SYNC bytes

  H_RULE(SYNC_check, h_bits(16, false));
  H_VRULE(SYNC_val, h_bits(16, false));
  // This will parse the first 9 bits of SYNC, then the frame-identifier, then the last 4 bits that we don't care about
  
  H_RULE(FRAMESIZE_check, h_bits(16, false));
  H_RULE(FRAMESIZE_val, h_bits(16, false));
  // This will be FRAMESIZE
  // FRAMESIZE is the number of BYTES in the frame
  // We will need to check this against PHNMR, ANNMR, and DGNMR
  // FRAMESIZE = 20 + NUM_PMU * (30 + 16 * (PH + AN + (16 * DG))) + 4 * (PH + AN + DG) + 4
  

  H_RULE(IDCODE_check, h_bits(16, false));
  H_VRULE(IDCODE_val, h_bits(16, false));
  // IDCODE
  // Just identifies the data stream
  // I don't think that we'll actually have to do anything with this one


  H_RULE(SOC_check, h_bits(32, false));
  H_RULE(SOC_val, h_bits(32, false));
  // SOC
  // Time-stamp
  // I think it's the number of seconds??
  // We shouldn't have to check it against anything


  H_RULE(FRACSEC_check, h_bits(32, false));
  H_RULE(FRACSEC_val, h_bits(32, false));
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

  H_RULE(CHK, h_sequence(h_bits(16, false), h_end_p(), NULL));

  H_RULE(LAZY_REP, h_sequence(h_repeat_n(h_bits(16, false), 3), h_many(h_bits(16, false)), h_end_p(), NULL));

  H_VRULE(Final_check, h_sequence(SYNC_check, FRAMESIZE_check, IDCODE_check, SOC_check, FRACSEC_check, h_xor(LAZY_REP, CHK), NULL));

  H_RULE(Prefix_validate, h_sequence(SYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, NULL));

  Well_formed = Final_check;
  Prefix = Prefix_validate;
  
}

void updateArgs(int i){
  int s = matrix[i][1];
  currDOPT = currPOPT = currAOPT = 1;
  if(s > 7){
    currDOPT = 2;
    s -= 8;
  }
  if(s > 3){
    currAOPT = 2;
    s -= 4;
  }
  if(s > 1){
    currPOPT = 2;
  }
}

int main(int argc, char *argv[]) {
    init_parser();


    uint8_t input[102400];
    size_t inputsize;

    inputsize = fread(input, 1, sizeof(input), stdin);
    printf("%lu\n", inputsize);
    HParseResult *result = h_parse(Well_formed, input, inputsize);
    if(result) {
        printf("yay!\n");
        HParseResult *ongoing = h_parse(Prefix, input, inputsize);
        if(ongoing){
          used_bytes = 14;
          
          for(int i = 0; i < matrix[0][5]; i++){
            updateArgs(i);

            H_RULE(junk, h_bits(8, false));

            H_RULE(STAT, h_bits(16, false));
            H_RULE(PHASOR, h_bits(32*currPOPT, false));
            H_RULE(FREQ, h_bits(16*currDOPT, false));
            H_RULE(DFREQ, h_bits(16*currDOPT, false));
            H_RULE(ANALOG, h_bits(16*currAOPT, false));
            H_RULE(DIGITAL, h_bits(16, false));

            H_VRULE(REP, h_sequence(STAT, h_repeat_n(PHASOR, matrix[i][2]), FREQ, DFREQ, h_repeat_n(ANALOG, matrix[i][3]), h_repeat_n(DIGITAL, matrix[i][4]), NULL));
            H_RULE(To_Date, h_sequence(h_repeat_n(junk, used_bytes), REP, NULL));
            used_bytes += (2 + 4*currPOPT*matrix[i][2] + 2*2*currDOPT + 2*currAOPT*matrix[i][3] + 2*matrix[i][4]);
            HParseResult *ongoing = h_parse(To_Date, input, inputsize);
            if(!ongoing){
              return 2;
            }
            printf("%d\n", used_bytes);
          }
          H_RULE(whole, h_bits(8, false));
          H_RULE(CHK, h_bits(16, false));
          H_RULE(final, h_sequence(h_repeat_n(whole, used_bytes), CHK, h_end_p(), NULL));
          HParseResult *last = h_parse(final, input, inputsize);
          if(last){
            printf("Success!\n");
            return 0;
          }
        } else {
          return 1;
        }
    } else {
        printf("boo!\n");
    }
}
