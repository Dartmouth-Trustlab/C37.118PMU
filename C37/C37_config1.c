#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "C37_tools.h"

#define MAX_PMUS 40
#define MAX_STREAMS 100
#define DEBUG true

const HParser *Well_formed;
const HParser *Prefix;
//Formats for any number of PMUs. In order:
//IDcode|DataFormat|#ofPhasors|#ofAnalogs|#ofDigital|#ofPMUs(1st PMU only)
//   0  |    1     |     2    |     3    |     4    |    5
int matrix[MAX_PMUS][6];
int currPMU = 0;
int currDOPT = 0;
int currAOPT = 0;
int currPOPT = 0;
int currTYPE = 0;
int fsize = 0;
int getCON = 0;
int getDATA = 0;
int getHEAD = 0;
int desiredID = 0; //What the stream ID claims

//All global data except current type, since this is updated after type is
// determined.
typedef struct AllDatas {
  int currPMU;
  int currDOPT;
  int currAOPT;
  int currPOPT;
  int fsize;
  int getCON;
  int getDATA;
  int getHEAD;
  int matrix[MAX_PMUS][6];
} AllData;

//Local variables
int streamID = 0; //Last used stream ID
int streamPos = 0; //Last used stream index
//Stream ID|# of known streams
//    0    |        1       
int streams[MAX_STREAMS][2];
//Data addresses
AllData *settings[MAX_STREAMS];
//Is this a previously unknown stream?
bool newStream = false;



void updateGlobals(AllData *in);
void updateLocals(AllData *in);
void printGlobals(void);

int main(){
  //Set # of concurrent streams to 0
  streams[0][1] = 0;
  //Set default success to failure
  int result = -1;
  //Set total packets read
  int total = 0;
  //Set valid packets read
  int validTotal = 0;
  //Open the source file
  FILE *file = fopen("streamtest.txt", "r");
  int c = fgetc(file);
  uint8_t input[102400];
  size_t inputsize = 0;
  //Until the end of the file...
  while (c != EOF){
      //If the current character isn't a newline, add it to the
      // list of read characters
      if(10 != c){
        input[inputsize] = (char) c;
        inputsize++;
      } else {
        //If there is a newline, we've reached the end of a packet. 
        //Convert the ascii to hex
        inputsize = process(input, inputsize);
        //Use choose to determine the type and stream id of the packet. If it
        // fails, the packet is invalid
        if(choose(input, inputsize)){
          //If you aren't still on the last stream
          if(desiredID != streamID){
            if(DEBUG)printf("*--------------------*\n");
            if(DEBUG)printf("desiredID = %d, streamID = %d\n", desiredID, streamID);
            //Set newStream default to true
            newStream = true;
            //Look through the streams you've encountered
            for(int i = 0; i < streams[0][1]; i++){
              if(DEBUG)printf("looking at id #%d\n", streams[i][0]);
              //If you find the current ID...
              if(desiredID == streams[i][0]){
                //Set the globals to its saved values
                updateGlobals(settings[i]);
                //It's not a new stream
                newStream = false;
                //Update the id and last used stream
                streamID = desiredID;
                streamPos = i;
                if(DEBUG)printf("Selecting stream %d, pos=%d\n", i, streamPos);
                //Stop looking
                i=streams[0][1];
              }
            }
          }
          //Switch based on type (note that type isn't stored in alldata, if
          // it where it would overwrite the choose function's selection)
          if(1 == currTYPE){
            //If config was requested and this isn't a new stream, run the
            // parser and update the settings.
            if(1 == getCON && !newStream){
              result = config(input, inputsize);
              if(DEBUG)printf("pos=%d\n", streamPos);
            } else {
              result = -1;
              printf("Warning: Config recieved without being requested. Ignoring.\n");
            }
            //Same for data
          } else if (2 == currTYPE && !newStream){
            if(1 == getDATA){
              result = data(input, inputsize);
              updateLocals(settings[streamPos]);
            } else {
              result = -1;
              printf("Warning: Data recieved without being requested. Ignoring.\n");
            }
          } else if (3 == currTYPE){
            if(newStream){
              int size = streams[0][1]; //For convenience
              //Initialize a new alldata object
              AllData *new = malloc(3000);
              new->getCON = new->getHEAD = new->getDATA = 0;
              new->matrix[0][5] = 0;
              updateGlobals(new);
              if(DEBUG)printf("desiredID = %d\n", desiredID);
              streams[size][0] = desiredID;
              streams[0][1] = size + 1;
              settings[size] = new;
              streamID = desiredID;
              newStream = false;
              streamPos = size;
              if(DEBUG)printf("******************\n");
              for(int i = 0; i <= size; i++){
                if(DEBUG)printf("id = %d, address = %d, size = %d, i=%d\n", streams[i][0], &settings[i], streams[0][1], i);
                if(DEBUG)printf("\t matrix[0][0] = %d\n", settings[i]->matrix[0][0]);
              }
              result = command(input, inputsize);
              updateLocals(settings[streamPos]);
            } else {
              result = command(input, inputsize);
              updateLocals(settings[streamPos]);
            }
            //Same for header
          } else if (4 == currTYPE && !newStream){
            if(1 == getHEAD){
              result = header(input, inputsize);
              updateLocals(settings[streamPos]);
            } else {
              result = -1;
              printf("Warning: Header recieved without being requested. Ignoring.\n");
            }
            //Default error, should never occur.
          } else {
            result = -2;
            printf("Error: Invalid type %d ???\n", currTYPE);
          }
        }
        //Reset input size, increment total and valid total
        inputsize = 0;
        total++;
        if(0 == result){
          validTotal++;
        }
      }
      //Get the next character
      c = fgetc(file);
    }
  //clean up
  for(int i = 0; i<streams[0][1]; i++){
    free(settings[i]);
  }
  printf("Read %d packets, %d valid\n", total, validTotal);
  return 0;
}

void updateGlobals(AllData *in){
  if(DEBUG)printGlobals();
  currPMU = in->currPMU;
  currDOPT = in->currDOPT;
  currAOPT = in->currAOPT;
  currPOPT = in->currPOPT;
  fsize = in->fsize;
  getCON = in->getCON;
  getDATA = in->getDATA;
  getHEAD = in->getHEAD;
  for(int i = 0; i<in->matrix[0][5];i++){
    for(int j = 0; j<6; j++){
      matrix[i][j] = in->matrix[i][j];
    }
  }
  if(DEBUG)printGlobals();
}

void updateLocals(AllData *in){
  if(DEBUG)printf("Current matrix[i][0]=%d, global = %d\n", in->matrix[0][0], matrix[0][0]);
  in->currPMU = currPMU;
  in->currDOPT = currDOPT;
  in->currAOPT = currAOPT;
  in->currPOPT = currPOPT;
  in->fsize = fsize;
  in->getCON = getCON;
  in->getDATA = getDATA;
  in->getHEAD = getHEAD;
  for(int i = 0; i<matrix[0][5];i++){
    for(int j = 0; j<6; j++){
      in->matrix[i][j] = matrix[i][j];
    }
  }
  if(DEBUG)printf("Now matrix[i][0]=%d, global =%d\n", in->matrix[0][0], matrix[0][0]);
}

void printGlobals(void){
  printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
  for(int i = 0; i < matrix[0][5]; i++){
    printf("%d %d %d %d %d %d\n", matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3], matrix[i][4], matrix[i][5]);
  }
}