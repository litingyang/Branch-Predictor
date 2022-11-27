//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <string.h>
#define MAX_ADDRESS 1048576
#define MAX_HISTORY 1048576

//
// TODO:Student Information
//
const char *studentName1 = "Chuling Ko";
const char *studentID1 = "A53310035";
const char *email1 = "c4ko@ucsd.edu";
const char *studentName2 = "Liting Yang";
const char *studentID2 = "A53312875";
const char *email2 = "ltyang@ucsd.edu";
//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

uint32_t globalHistoryMask;
uint32_t localHistoryMask;
uint32_t pcIndexMask;

uint32_t globalHistory;
uint32_t localHistory[MAX_ADDRESS];

uint8_t globalPredictorState[MAX_HISTORY];
uint8_t localPredictorState[MAX_HISTORY];

uint32_t tournamentSelectorState[MAX_HISTORY];

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
uint32_t c_globalHistory;
uint8_t c_globalPredictorState[MAX_HISTORY];


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_predictor()
{
  globalHistoryMask = (1ULL << ghistoryBits) - 1;
  localHistoryMask = (1ULL << lhistoryBits) - 1;
  pcIndexMask = (1ULL << pcIndexBits) - 1;
  globalHistory = 0;
  c_globalHistory = 0;
  for (int i = 0; i < MAX_ADDRESS; i++)
    localHistory[i] = 0;
  for (int i = 0; i < MAX_HISTORY; i++)
  {
    c_globalPredictorState[i] = WN;
    globalPredictorState[i] = WN;
    localPredictorState[i] = WN;
    tournamentSelectorState[i] = WG;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

uint8_t make_prediction_gshare(uint32_t pc)
{
  uint32_t index = (pc & globalHistoryMask) ^ (globalHistory & globalHistoryMask);
  if (globalPredictorState[index] == SN || globalPredictorState[index] == WN)
    return NOTTAKEN;
  else
    return TAKEN;
}
uint8_t make_prediction_cl(uint32_t pc)
{
  uint32_t index = (pc & globalHistoryMask) ^ (c_globalHistory & globalHistoryMask);
  if (c_globalPredictorState[index] == SN || c_globalPredictorState[index] == WN)
    return NOTTAKEN;
  else
    return TAKEN;
}
uint8_t make_prediction_global()
{
  uint32_t history = globalHistory & globalHistoryMask;
  if (globalPredictorState[history] == SN || globalPredictorState[history] == WN)
    return NOTTAKEN;
  else
    return TAKEN;
}

uint8_t make_prediction_local(uint32_t pc)
{
  uint32_t address = pc & pcIndexMask;
  uint32_t history = localHistory[address] & localHistoryMask;
  if (localPredictorState[history] == SN || localPredictorState[history] == WN)
    return NOTTAKEN;
  else
    return TAKEN;
}

uint8_t tournament_perdictor_select()
{
  uint32_t history = globalHistory & globalHistoryMask;
  if (tournamentSelectorState[history] == WL || tournamentSelectorState[history] == SL)
    return LOCAL;
  else // 0 or 1
    return GLOBAL;
}

uint8_t c_perdictor_select()
{
  uint32_t history = globalHistory & globalHistoryMask;
  if (tournamentSelectorState[history] == WL || tournamentSelectorState[history] == SL)
    return LOCAL;
  else // 0 or 1
    return GLOBAL;
}

uint8_t make_prediction_turnament(uint32_t pc)
{
  if (tournament_perdictor_select() == LOCAL)
    return make_prediction_local(pc);
  else
    return make_prediction_global();
}

uint8_t make_prediction_c(uint32_t pc)
{
  if (c_perdictor_select() == LOCAL)
    return make_prediction_local(pc);
  else
    return make_prediction_gshare(pc);
}

uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return make_prediction_gshare(pc);
  case TOURNAMENT:
    return make_prediction_turnament(pc);
  case CUSTOM:
 
    return make_prediction_c(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

uint8_t get_next_state(uint8_t state, uint8_t outcome)
{
  if (outcome == NOTTAKEN && state != SN)
    state--;
  else if (outcome == TAKEN && state != ST)
    state++;
  return state;
}

uint8_t get_tournament_result(uint32_t pc, uint8_t outcome)
{
  uint8_t predictionLocal = make_prediction_local(pc);
  uint8_t predictionGlobal = make_prediction_global();
  if (predictionLocal == outcome && predictionGlobal != outcome)
    return LOCALWIN;
  else if (predictionLocal != outcome && predictionGlobal == outcome)
    return GLOBALWIN;
  else
    return DRAW;
}


uint8_t get_c_result(uint32_t pc, uint8_t outcome)
{
  uint8_t predictionLocal = make_prediction_local(pc);
  uint8_t predictionGlobal = make_prediction_gshare(pc);
  if (predictionLocal == outcome && predictionGlobal != outcome)
    return LOCALWIN;
  else if (predictionLocal != outcome && predictionGlobal == outcome)
    return GLOBALWIN;
  else
    return DRAW;
}


uint32_t get_next_selector_c(uint32_t pc, uint32_t selector, uint8_t outcome)
{
  uint8_t result = get_c_result(pc, outcome);
  if (result == LOCALWIN && selector != SL)
    selector++;
  else if (result == GLOBALWIN && selector != SG)
    selector--;
  return selector;
}

uint32_t get_next_selector(uint32_t pc, uint32_t selector, uint8_t outcome)
{
  uint8_t result = get_tournament_result(pc, outcome);
  if (result == LOCALWIN && selector != SL)
    selector++;
  else if (result == GLOBALWIN && selector != SG)
    selector--;
  return selector;
}

void print_history(uint32_t history)
{
  uint32_t index = 1 << (ghistoryBits - 1);
  while (index)
  {
    if (history & index)
      printf("T");
    else
      printf("N");
    index >>= 1;
  }
  puts("");
}

void train_predictor_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t ghistory = globalHistory & globalHistoryMask;
  tournamentSelectorState[ghistory] = get_next_selector(pc, tournamentSelectorState[ghistory], outcome);

  uint32_t address = pc & pcIndexMask;
  uint32_t lhistory = localHistory[address] & localHistoryMask;
  localPredictorState[lhistory] = get_next_state(localPredictorState[lhistory], outcome);
  localHistory[address] <<= 1;
  localHistory[address] |= outcome;

  globalPredictorState[ghistory] = get_next_state(globalPredictorState[ghistory], outcome);
  globalHistory <<= 1;
  globalHistory |= outcome;
}
void train_predictor_c(uint32_t pc, uint8_t outcome)
{
  uint32_t ghistory = globalHistory & globalHistoryMask;
  tournamentSelectorState[ghistory] = get_next_selector_c(pc, tournamentSelectorState[ghistory], outcome);

  uint32_t address = pc & pcIndexMask;
  uint32_t lhistory = localHistory[address] & localHistoryMask;
  localPredictorState[lhistory] = get_next_state(localPredictorState[lhistory], outcome);
  localHistory[address] <<= 1;
  localHistory[address] |= outcome;


  uint32_t index = (pc & globalHistoryMask) ^ (globalHistory & globalHistoryMask);
  globalPredictorState[index] = get_next_state(globalPredictorState[index], outcome);

  globalHistory <<= 1;
  globalHistory |= outcome;
}
void train_predictor_gshare(uint32_t pc, uint8_t outcome)
{
  uint32_t index = (pc & globalHistoryMask) ^ (globalHistory & globalHistoryMask);
  globalPredictorState[index] = get_next_state(globalPredictorState[index], outcome);

  globalHistory <<= 1;
  globalHistory |= outcome;
}

void train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    train_predictor_gshare(pc, outcome);
    break;
  case TOURNAMENT:
    train_predictor_tournament(pc, outcome);
  case CUSTOM:
    train_predictor_c(pc, outcome);
  default:
    break;
  }
}
