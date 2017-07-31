#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>

const HParser *Well_formed;
const HParser *Prefix;
int matrix[][6];
int used_bytes = 0;
int currPMU = 0;

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

bool validate_SYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43569 == sync | 43553 == sync){
    printf("%d\n", matrix[1][1]);
    return true;
  } else {
    printf("%hu\n", sync);
    printf("Error: invalid sync bytes. Only 0xaa21 or 0xaa31 are acceptable.\n");
    return false;
  }
}

bool validate_NUM_PMU_val(HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  matrix[0][5] = val;
  return true;
}

bool validate_Final_check(HParseResult *p, void *user_data){
  h_pprint(stdout, h_act_flatten(p, NULL), 0 , 1);
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

HParsedToken *act_IDCODE(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][0] = val;
  return ret;
}

HParsedToken *act_FORMAT(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][1] = val;
  return ret;
}

HParsedToken *act_PHNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][2] = val;
  return ret;
}

HParsedToken *act_ANNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][3] = val;
  return ret;
}

HParsedToken *act_DGNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][4] = val;
  return ret;
}

HParsedToken *act_FRAMESIZE(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
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
  H_RULE(IDCODE_val, h_bits(16, false));
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

  H_RULE(TIME_BASE_check, h_bits(32, false));
  H_RULE(TIME_BASE_val, h_bits(32, false));

  H_RULE(NUM_PMU_check, h_bits(16, false));
  H_VRULE(NUM_PMU_val, h_bits(16, false));

  // THE FRAMES BECOMES DISTINCT AFTER THIS POINT

  //Config frame format:
  //*Repeated:
  //|Stn 16 bytes
  //|Idcode 2 bytes
  //|Format 2 bytes
  //|Phnmr 2 bytes
  //|Annmr 2 bytes
  //|Dgnmr 2 bytes
  //|Chnam 16*(Phnmr+Annmr+(16*Dgnmr)) bytes
  //|Phunit 4*Phnmr bytes
  //|Anunit 4*Annmr bytes
  //|Digunit 4*Dgnmr bytes
  //|Fnom 2 bytes
  //|Cfgcnt 2 bytes
  //*End Repeated
  //Data_rate 2 bytes
  //CHK 2 bytes

  H_RULE(DATA_RATE, h_bits(16, false));

  H_RULE(CHK, h_bits(16, false));

  H_RULE(Postfix, h_sequence(DATA_RATE, CHK, h_end_p(), NULL));

  H_RULE(LAZY_REP, h_sequence(h_repeat_n(h_bits(16, false), 34), h_many(h_bits(16, false)), h_end_p(), NULL));

  H_VRULE(Final_check, h_sequence(SYNC_check, FRAMESIZE_check, IDCODE_check, SOC_check, FRACSEC_check, TIME_BASE_check, NUM_PMU_check, h_xor(LAZY_REP, Postfix), NULL));

  H_RULE(Prefix_validate, h_sequence(SYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, TIME_BASE_val, NUM_PMU_val, NULL));

  Well_formed = Final_check;
  Prefix = Prefix_validate;
  
}

int config() {
    init_parser();

    uint8_t input[102400];
    size_t inputsize;

    inputsize = fread(input, 1, sizeof(input), stdin);
    printf("%lu\n", inputsize);
    inputsize = process(input, inputsize);
    HParseResult *result = h_parse(Well_formed, input, inputsize);
    if(result) {
        printf("yay!\n");
        HParseResult *ongoing = h_parse(Prefix, input, inputsize);
        if(ongoing){
          used_bytes = 20;
          for(int i = 0; i < matrix[0][5]; i++){
            currPMU = i;
            H_RULE(junk, h_bits(8, false));

            H_RULE(STN, h_sequence(h_uint64(), h_uint64(), NULL));
            H_ARULE(IDCODE, h_bits(16, false));
            H_ARULE(FORMAT, h_bits(16, false));
            H_ARULE(PHNMR, h_bits(16, false));
            H_ARULE(ANNMR, h_bits(16, false));
            H_ARULE(DGNMR, h_bits(16, false));
            H_RULE(CHNAM, h_sequence(h_uint64(), h_uint64(), NULL));
            H_RULE(PHUNIT, h_bits(32, false));
            H_RULE(ANUNIT, h_bits(32, false));
            H_RULE(DIGUNIT, h_bits(32, false));
            H_RULE(FNOM, h_bits(16, false));
            H_RULE(CFGCNT, h_bits(16, false));

            H_RULE(REP1, h_sequence(STN, IDCODE, FORMAT, PHNMR, ANNMR, DGNMR, NULL));
            H_RULE(To_Date, h_sequence(h_repeat_n(junk, used_bytes), REP1, NULL));
            used_bytes += (16 + 2 + 2 + 2 + 2 + 2);
            HParseResult *ongoing = h_parse(To_Date, input, inputsize);
            if(!ongoing){
              printf("Error\n");
              return 3;
            }
            H_RULE(NAMES, h_sequence(h_repeat_n(CHNAM, matrix[i][2]), h_repeat_n(CHNAM, matrix[i][3]), h_repeat_n(CHNAM, 16*matrix[i][4]), NULL));
            H_RULE(To_Date1, h_sequence(h_repeat_n(junk, used_bytes), NAMES, h_repeat_n(PHUNIT, matrix[i][2]), h_repeat_n(ANUNIT, matrix[i][3]), h_repeat_n(DIGUNIT, matrix[i][4]), FNOM, CFGCNT, NULL));
            used_bytes += 16*(matrix[i][2]+matrix[i][3]+(16*matrix[i][4]));
            used_bytes += (4*matrix[i][2])+(4*matrix[i][3])+(4*matrix[i][4])+2+2;
            HParseResult *ongoing1 = h_parse(To_Date1, input, inputsize);
            if(!ongoing1){
              printf("Error\n");
              return 4;
            }
          }
          H_RULE(whole, h_bits(8, false));
          H_RULE(DATA_RATE, h_bits(16, false));
          H_RULE(CHK, h_bits(16, false));
          H_RULE(final, h_sequence(h_repeat_n(whole, used_bytes), DATA_RATE, CHK, h_end_p(), NULL));
          HParseResult *last = h_parse(final, input, inputsize);
          if(last){
            printf("Success!\n");
            for(int j = 0; j < matrix[0][5]; j++){
              printf("%d %d %d %d %d %d\n", matrix[j][0], matrix[j][1], matrix[j][2], matrix[j][3], matrix[j][4], matrix[j][5]);
            }
            return 0;
          }
        } else {
          return 5;
        }
    } else {
        printf("boo!\n");
        return 2;
    }
    return 1;
}

int main(){
  if(!config()){
    printf("yay!\n");
  }
  return 0;
}