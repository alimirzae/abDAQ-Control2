#ifndef LABDAQ_HEARTBEAT_H
#define LABDAQ_HEARTBEAT_H
#include <stdint.h>
typedef enum {
 LABDAQ_HB_BOOT=0,
 LABDAQ_HB_INIT,
 LABDAQ_HB_READY,
 LABDAQ_HB_ACQUIRING,
 LABDAQ_HB_TEST_RUNNING,
 LABDAQ_HB_FAULT
} labdaq_heartbeat_state_t;
void LABDAQ_Heartbeat_Init(void);
void LABDAQ_Heartbeat_SetState(labdaq_heartbeat_state_t state);
labdaq_heartbeat_state_t LABDAQ_Heartbeat_GetState(void);
void LABDAQ_Heartbeat_Task(uint32_t now_ms);
void LABDAQ_Heartbeat_FaultBlocking(void);
#endif
