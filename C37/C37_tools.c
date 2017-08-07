#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <math.h>
#include "C37_tools.h"

extern const HParser *Well_formed;
extern const HParser *Prefix;
extern int matrix[][6];
extern int currPMU;
extern int currDOPT;
extern int currAOPT;
extern int currPOPT;
extern int currTYPE;
extern int fsize;
extern int getCON;
extern int getDATA;
extern int getHEAD;

size_t process(uint8_t *input, size_t inputsize){
  if(inputsize%2 != 0){
    return 0;
  }
  for(int i=0; i < inputsize; i++){
      if(input[i] < 58 & input[i] > 47){
        input[i] -= 48;
      } else if(input[i] > 96 & input[i] < 104){
        input[i] -= 87;
      } else {
        printf("Process failed on char: %d\n", input[i]);
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

//Confirms that sync bytes are aa21 or aa31. Fails if they
// are not.
bool validate_conSYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43569 == sync | 43553 == sync){
    printf("Sync = %hu\n", sync);
    return true;
  } else {
    printf("Sync = %hu\n", sync);
    printf("Error: invalid sync bytes. Only 0xaa21 or 0xaa31 are acceptable.\n");
    return false;
  }
}

//Sets numpmu. Never fails.
bool validate_NUM_PMU_val(HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  matrix[0][5] = val;
  return true;
}

bool validate_LAZY_REP(HParseResult *p, void *user_data){
  uint16_t val = h_act_flatten(p, NULL)->seq->used;
  //printf("# of bytes = %hu\n", val);
  return true;
}

//For testing. Prints relevant values. Never fails.
bool validate_Final_check(HParseResult *p, void *user_data){
  //h_pprint(stdout, h_act_flatten(p, NULL), 0 , 1);
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

//Stores the idcode of the current PMU.
HParsedToken *act_IDCODE(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][0] = val;
  return ret;
}

//Stores the format of the current PMU.
HParsedToken *act_FORMAT(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][1] = val;
  return ret;
}

//Stores the number of phasors of the current PMU.
HParsedToken *act_PHNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][2] = val;
  return ret;
}

//Stores analog number of the current PMU.
HParsedToken *act_ANNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][3] = val;
  return ret;
}

//Stores digital number of the current PMU.
HParsedToken *act_DGNMR(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  matrix[currPMU][4] = val;
  return ret;
}

//ELEPHANT. Stores framesize and confirms that recieved packet
// matches framesize.
HParsedToken *act_FRAMESIZE_val(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  fsize = val;
  HParsedToken *ret = H_MAKE_UINT(val);
  return ret;
}

void init_parser(){
  H_RULE(SYNC_check, h_bits(16, false));
  H_VRULE(conSYNC_val, h_bits(16, false));
  // Checks sync bytes, first 2 bytes in the message.
  
  H_RULE(FRAMESIZE_check, h_bits(16, false));
  H_ARULE(FRAMESIZE_val, h_bits(16, false));
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
  // More time info. Again, irrelevant for parsing.

  H_RULE(NUM_PMU_check, h_bits(16, false));
  H_VRULE(NUM_PMU_val, h_bits(16, false));
  // Number of PMUs. Needs to be used to validate the 
  // repeated frames.

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
  // Expected data rate. Not sure if we need
  // to match this to be valid.

  H_RULE(CHK, h_bits(16, false));
  // Validation bytes.

  H_RULE(Postfix, h_sequence(DATA_RATE, CHK, h_end_p(), NULL));
  // The concluding, non-repeated info for config frames

  //H_RULE(LAZY_REP, h_sequence(h_many(h_bits(8, false)), h_end_p(), NULL));
  H_VRULE(LAZY_REP, h_sequence(h_repeat_n(h_bits(8, false), 34), h_optional(h_sequence(h_repeat_n(h_bits(8, false), 20), h_many(h_repeat_n(h_bits(8, false), 10)), NULL)), h_end_p(), NULL));
  // Tests well-formedness of repeated frames. Any well-formed frame will
  // have either 0 or 1+ repetition of the repeated fields, and the shortest
  // possible repeated field is 30 bytes + 4 postfix bytes. Because additional
  // bytes are always added in multiples of 20, any well formed repeated field
  // will have 34+20x bytes.

  H_VRULE(Final_check, h_sequence(SYNC_check, FRAMESIZE_check, IDCODE_check, SOC_check, FRACSEC_check, TIME_BASE_check, NUM_PMU_check, h_xor(LAZY_REP, Postfix), NULL));
  // Final well-formedness check. Confirms that the prefix is the correct
  // length and followed by either the postfix XOR some number of repeated
  // frames and then the postfix.


  H_RULE(Prefix_validate, h_sequence(conSYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, TIME_BASE_val, NUM_PMU_val, NULL));
  // Validates the prefix after it has been determined to be well formed.

  Well_formed = Final_check;
  Prefix = Prefix_validate;
  
}

int config(uint8_t input[], size_t inputsize){
    init_parser();

    printf("Inputsize = %lu\n", inputsize);

    //Convert ascii file to hex.
    //inputsize = process(input, inputsize);

    //Check that it's well formed.
    HParseResult *result = h_parse(Well_formed, input, inputsize);
    if(result) {
        printf("Config well formed...\n");
        HParseResult *ongoing = h_parse(Prefix, input, inputsize);
        if(ongoing){
          int used_bytes = 20;
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
              printf("Error: Config body part 1 invalid\n");
              return 3;
            }
            H_RULE(NAMES, h_sequence(h_repeat_n(CHNAM, matrix[i][2]), h_repeat_n(CHNAM, matrix[i][3]), h_repeat_n(CHNAM, 16*matrix[i][4]), NULL));
            H_RULE(To_Date1, h_sequence(h_repeat_n(junk, used_bytes), NAMES, h_repeat_n(PHUNIT, matrix[i][2]), h_repeat_n(ANUNIT, matrix[i][3]), h_repeat_n(DIGUNIT, matrix[i][4]), FNOM, CFGCNT, NULL));
            used_bytes += 16*(matrix[i][2]+matrix[i][3]+(16*matrix[i][4]));
            used_bytes += (4*matrix[i][2])+(4*matrix[i][3])+(4*matrix[i][4])+2+2;
            HParseResult *ongoing1 = h_parse(To_Date1, input, inputsize);
            if(!ongoing1){
              printf("Error: Config body part 2 invalid\n");
              return 4;
            }
          }
          H_RULE(whole, h_bits(8, false));
          H_RULE(DATA_RATE, h_bits(16, false));
          H_RULE(CHK, h_bits(16, false));
          H_RULE(final, h_sequence(h_repeat_n(whole, used_bytes), DATA_RATE, CHK, h_end_p(), NULL));
          HParseResult *last = h_parse(final, input, inputsize);
          if(last){
            printf("Success! Config frame valid, settings are:\n");
            for(int j = 0; j < matrix[0][5]; j++){
              printf("%d %d %d %d %d %d\n", matrix[j][0], matrix[j][1], matrix[j][2], matrix[j][3], matrix[j][4], matrix[j][5]);
            }
            getCON = 0; //Reset config request
            return 0;
          }
        } else {
          return 5;
        }
    } else {
        printf("Config malformed!\n");
        return 2;
    }
    return 1;
}

/***********************/
bool validate_CHK(HParseResult *p, void *user_data){
  uint16_t id = p->ast->uint;
  printf("CHK = %hu\n", id);
  return true;
}

bool validate_REP(HParseResult *p, void *user_data){
  //h_pprint(stdout, h_act_flatten(p, NULL), 0, 1);
  return true;
}

bool validate_dSYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43521 == sync){
    return true;
  } else {
    printf("Error: invalid sync bytes. Only 0xaa01 is acceptable.\n");
    return false;
  }
}

bool validate_IDCODE_val(HParseResult *p, void *user_data){
  uint16_t id = p->ast->uint;
  /*if(idcode == id){
    return true;
  } else {
    printf("Error: expected idcode %d\n", idcode);
    return false;
  }*/
  return true;
}

void init_parser2(){
  // SYNC
  // Use "h_bits(len, sign)" to recognize the two SYNC bytes

  H_RULE(SYNC_check, h_bits(16, false));
  H_VRULE(dSYNC_val, h_bits(16, false));
  // This will parse the first 9 bits of SYNC, then the frame-identifier, then the last 4 bits that we don't care about
  
  H_RULE(FRAMESIZE_check, h_bits(16, false));
  H_ARULE(FRAMESIZE_val, h_bits(16, false));
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

  H_RULE(Prefix_validate, h_sequence(dSYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, NULL));

  Well_formed = Final_check;
  Prefix = Prefix_validate;
  
}

int data(uint8_t input[], size_t inputsize){
  if(0 == matrix[0][5]){
    printf("Uninitialized or nonsensical confguration. Ignoring data.\n");
    return 6;
  }
  init_parser2();
  //inputsize = process(input, inputsize);
  HParseResult *result = h_parse(Well_formed, input, inputsize);
  if(result) {
    printf("Data well formed...\n");
    HParseResult *ongoing = h_parse(Prefix, input, inputsize);
    if(ongoing){
      int used_bytes = 14;
          
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
          printf("Data invalid!\n");
          return 2;
        }
      }
      H_RULE(whole, h_bits(8, false));
      H_VRULE(CHK, h_bits(16, false));
      H_RULE(final, h_sequence(h_repeat_n(whole, used_bytes), CHK, h_end_p(), NULL));
      HParseResult *last = h_parse(final, input, inputsize);
      if(last){
        printf("Data valid!\n");
        return 0;
      } else {
        printf("Data invalid!\n");
        return 1;
      }
    } else{
      printf("Data invalid! Bad prefix.\n");
      return 4;
    }
  } else {
    printf("Data malformed!\n");
    return 2;
  }
  printf("ERROR\n");
  return 3;
}

bool validate_SYNC_TYPE(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43569 == sync | 43553 == sync){
    printf("Sync = %hu, config\n", sync);
    currTYPE = 1;
    return true;
  } else if(43521 == sync){
    printf("Sync = %hu, data\n", sync);
    currTYPE = 2;
    return true;
  } else if(43585 == sync){
    printf("Sync = %hu, command\n", sync);
    currTYPE = 3;
    return true;
  } else if(43537 == sync){
    printf("Sync = %hu, header\n", sync);
    currTYPE = 4;
    return true;
  } else {
    printf("Sync = %hu, ???\n", sync);
    return false;
  }
}

bool validate_comSYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43585 == sync){
    return true;
  } else {
    printf("Error: invalid sync bytes. Only 0xaa41 is acceptable.\n");
    return false;
  }
}

HParsedToken *act_CMD(const HParseResult *p, void *user_data){
  uint16_t val = p->ast->uint;
  HParsedToken *ret = H_MAKE_UINT(val);
  switch(val){
    case 1:
      getDATA = 0;
      break;
    case 2:
      getDATA = 1;
      break;
    case 3:
      getHEAD = 1;
      break;
    case 4:
      getCON = 1;
      break;
    case 5:
      getCON = 1;
      break;
    case 6:
      getCON = 1;
      break;
    default:
      printf("Unknown command: please update user-designated commands.\n");
  }
  return ret;
}

void init_parser3(){
  // SYNC
  // Use "h_bits(len, sign)" to recognize the two SYNC bytes

  H_RULE(SYNC_check, h_bits(16, false));
  H_VRULE(comSYNC_val, h_bits(16, false));
  // This will parse the first 9 bits of SYNC, then the frame-identifier, then the last 4 bits that we don't care about
  
  H_RULE(FRAMESIZE_check, h_bits(16, false));
  H_ARULE(FRAMESIZE_val, h_bits(16, false));
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

  //Command frame format:
  //CMD 2 bytes
  //EXTFRAME 0-65518 bytes
  //CHK 2 bytes

  H_RULE(CMD, h_bits(16, false));

  H_RULE(CHK, h_sequence(h_bits(16, false), h_end_p(), NULL));

  H_RULE(LAZY_REP, h_sequence(h_repeat_n(h_bits(16, false), 2), h_many(h_bits(16, false)), h_end_p(), NULL));

  H_VRULE(Final_check, h_sequence(SYNC_check, FRAMESIZE_check, IDCODE_check, SOC_check, FRACSEC_check, h_xor(LAZY_REP, CHK), NULL));

  H_RULE(Prefix_validate, h_sequence(comSYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, NULL));

  Well_formed = Final_check;
  Prefix = Prefix_validate;
}

int command(uint8_t input[], size_t inputsize){
  init_parser3();
  HParseResult *result = h_parse(Well_formed, input, inputsize);
  if(result) {
      printf("Command well formed...\n");
      HParseResult *ongoing = h_parse(Prefix, input, inputsize);
      if(ongoing){
        H_RULE(junk, h_bits(8, false));
        H_ARULE(CMD, h_bits(16, false));
        H_RULE(EXTFRAME, h_bits(8, false));
        H_VRULE(CHK, h_bits(16, false));
        H_RULE(final, h_sequence(h_repeat_n(junk, 14), CMD, h_repeat_n(EXTFRAME, fsize-18), CHK, h_end_p(), NULL));
        HParseResult *last = h_parse(final, input, inputsize);
        if(last){
          printf("Command valid!\n");
          return 0;
        } else {
          printf("Command invalid!\n");
          return 1;
        }
      } else {
        printf("Command invalid\n");
        return 1;
      }
  } else {
      printf("Command malformed!\n");
      return 2;
  }
  return 3;
}

bool validate_hSYNC_val(HParseResult *p, void *user_data){
  uint16_t sync = p->ast->uint;
  if(43537 == sync){
    return true;
  } else {
    printf("Error: invalid sync bytes. Only 0xaa11 is acceptable.\n");
    return false;
  }
}

void init_parser4(){
  // SYNC
  // Use "h_bits(len, sign)" to recognize the two SYNC bytes

  H_RULE(SYNC_check, h_bits(16, false));
  H_VRULE(hSYNC_val, h_bits(16, false));
  // This will parse the first 9 bits of SYNC, then the frame-identifier, then the last 4 bits that we don't care about
  
  H_RULE(FRAMESIZE_check, h_bits(16, false));
  H_ARULE(FRAMESIZE_val, h_bits(16, false));
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

  //Header frame format:
  //Data 1*k bytes
  //CHK 2 bytes

  H_RULE(LAZY_REP, h_sequence(h_bits(24, false), h_many(h_bits(8, false)), h_end_p(), NULL));

  H_VRULE(Final_check, h_sequence(SYNC_check, FRAMESIZE_check, IDCODE_check, SOC_check, FRACSEC_check, LAZY_REP, NULL));

  H_RULE(Prefix_validate, h_sequence(hSYNC_val, FRAMESIZE_val, IDCODE_val, SOC_val, FRACSEC_val, NULL));

  Well_formed = Final_check;
  Prefix = Prefix_validate;
}

int header(uint8_t input[], size_t inputsize){
  init_parser4();
  HParseResult *result = h_parse(Well_formed, input, inputsize);
  if(result) {
      printf("Header well formed...\n");
      HParseResult *ongoing = h_parse(Prefix, input, inputsize);
      if(ongoing){
        int used_bytes = 14;
        H_RULE(junk, h_bits(8, false));
        H_RULE(DATACHAR, h_bits(8, false));
        H_VRULE(CHK, h_bits(16, false));
        H_RULE(final, h_sequence(h_repeat_n(junk, 14), h_repeat_n(DATACHAR, fsize-16), CHK, h_end_p(), NULL));
        HParseResult *last = h_parse(final, input, inputsize);
        if(last){
          printf("Header valid!\n");
          getHEAD = 0;
          return 0;
        } else {
          printf("Header invalid!\n");
          return 1;
        }
      } else {
        printf("Header invalid\n");
        return 1;
      }
  } else {
      printf("Header malformed!\n");
      return 2;
  }
  return 3;
}

int choose(uint8_t input[], size_t inputsize){
  //inputsize = process(input, inputsize);
  H_VRULE(SYNC_TYPE, h_bits(16, false));
  HParseResult *temp = h_parse(SYNC_TYPE, input, inputsize);
  if(temp){
    return 1;
  } else {
    printf("Error: The sync byte matches no known standard/isn't long enough.\n");
    return 0;
  }
}