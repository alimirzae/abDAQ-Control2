#include "labdaq_heartbeat.h"
#include "labdaq_config.h"
#include "stm32f4xx_hal.h"

static labdaq_heartbeat_state_t g_state=LABDAQ_HB_BOOT;
static uint32_t g_last_toggle=0;

static uint32_t interval_ms(labdaq_heartbeat_state_t s){
 switch(s){
  case LABDAQ_HB_BOOT: return 100;       /* very fast: reset/boot */
  case LABDAQ_HB_INIT: return 250;       /* fast: peripheral initialization */
  case LABDAQ_HB_READY: return 1000;     /* slow: healthy and idle */
  case LABDAQ_HB_ACQUIRING: return 500;  /* normal: DAQ active */
  case LABDAQ_HB_TEST_RUNNING: return 125;/* very fast: experiment active */
  case LABDAQ_HB_FAULT: return 75;       /* rapid fault indication */
  default:return 1000;
 }
}
void LABDAQ_Heartbeat_Init(void){g_state=LABDAQ_HB_BOOT;g_last_toggle=HAL_GetTick();HAL_GPIO_WritePin(LABDAQ_LED_PORT,LABDAQ_LED_PIN,GPIO_PIN_RESET);}
void LABDAQ_Heartbeat_SetState(labdaq_heartbeat_state_t s){if(g_state!=s){g_state=s;g_last_toggle=HAL_GetTick();HAL_GPIO_WritePin(LABDAQ_LED_PORT,LABDAQ_LED_PIN,GPIO_PIN_RESET);}}
labdaq_heartbeat_state_t LABDAQ_Heartbeat_GetState(void){return g_state;}
void LABDAQ_Heartbeat_Task(uint32_t now){uint32_t d=interval_ms(g_state);if(now-g_last_toggle>=d){g_last_toggle=now;HAL_GPIO_TogglePin(LABDAQ_LED_PORT,LABDAQ_LED_PIN);}}
void LABDAQ_Heartbeat_FaultBlocking(void){g_state=LABDAQ_HB_FAULT;while(1){HAL_GPIO_TogglePin(LABDAQ_LED_PORT,LABDAQ_LED_PIN);HAL_Delay(interval_ms(LABDAQ_HB_FAULT));}}
