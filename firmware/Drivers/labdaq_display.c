#include "labdaq_display.h"
#include "labdaq_control.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

labdaq_display_t g_labdaq_display;

__attribute__((weak)) void LABDAQ_DisplayHW_Init(void){}
__attribute__((weak)) void LABDAQ_DisplayHW_BeginFrame(void){}
__attribute__((weak)) void LABDAQ_DisplayHW_Header(const char*a,const char*b,const char*c,const char*d){(void)a;(void)b;(void)c;(void)d;}
__attribute__((weak)) void LABDAQ_DisplayHW_Text(uint16_t r,const char*t){(void)r;(void)t;}
__attribute__((weak)) void LABDAQ_DisplayHW_Graph(const float*s,uint16_t n,uint16_t h,const char*x,const char*y){(void)s;(void)n;(void)h;(void)x;(void)y;}
__attribute__((weak)) void LABDAQ_DisplayHW_EndFrame(void){}

const char *LABDAQ_Display_PageName(labdaq_display_page_t p){
 static const char *n[]={"Status","Sensors","Live Graph","Test Graph","Experiment","PID / EP","Alarms","System"};
 return ((unsigned)p<LABDAQ_DISPLAY_PAGE_COUNT)?n[p]:"Status";
}
void LABDAQ_Display_Init(void){memset(&g_labdaq_display,0,sizeof(g_labdaq_display));g_labdaq_display.dirty=true;LABDAQ_DisplayHW_Init();}
void LABDAQ_Display_NextPage(void){g_labdaq_display.page=(labdaq_display_page_t)((g_labdaq_display.page+1U)%LABDAQ_DISPLAY_PAGE_COUNT);g_labdaq_display.dirty=true;}
void LABDAQ_Display_ButtonEvent(void){uint32_t now=HAL_GetTick();if(now-g_labdaq_display.last_button_ms>=200U){g_labdaq_display.last_button_ms=now;LABDAQ_Display_NextPage();}}
void LABDAQ_Display_SelectChannel(uint8_t ch){if(ch<LABDAQ_NUM_CHANNELS){g_labdaq_display.selected_channel=ch;g_labdaq_display.dirty=true;}}

static void render(labdaq_system_t *s){
 char line[96]; uint8_t ch=g_labdaq_display.selected_channel;
 LABDAQ_DisplayHW_BeginFrame();
 LABDAQ_DisplayHW_Header(LABDAQ_DISPLAY_BRAND,LABDAQ_DISPLAY_COMPANY,LABDAQ_DISPLAY_SITE,LABDAQ_Display_PageName(g_labdaq_display.page));
 switch(g_labdaq_display.page){
 case LABDAQ_DISPLAY_STATUS:
  snprintf(line,sizeof(line),"DAQ %s | %lu SPS/ch",s->streaming_active?"RUN":"STOP",(unsigned long)s->sampling_rate_hz);LABDAQ_DisplayHW_Text(0,line);
  snprintf(line,sizeof(line),"Test state %d | Cycle %lu/%lu",(int)s->test_state,(unsigned long)s->cyclic_gen.current_cycle,(unsigned long)s->cyclic_gen.target_cycles);LABDAQ_DisplayHW_Text(1,line);
  snprintf(line,sizeof(line),"Time %lu | Logs %u",(unsigned long)LABDAQ_Time_NowUnix(),g_labdaq_control.log_count);LABDAQ_DisplayHW_Text(2,line); break;
 case LABDAQ_DISPLAY_SENSORS:
  for(uint8_t i=0;i<LABDAQ_NUM_CHANNELS;i++){float v=LABDAQ_Channel_Engineering(i,s->latest_voltage_frame[i]);snprintf(line,sizeof(line),"%02u %-12s %8.2f %s",i+1,g_labdaq_control.channel[i].name,v,g_labdaq_control.channel[i].unit);LABDAQ_DisplayHW_Text(i,line);} break;
 case LABDAQ_DISPLAY_LIVE_GRAPH:
  snprintf(line,sizeof(line),"CH%u %s %.2f %s",ch+1,g_labdaq_control.channel[ch].name,LABDAQ_Channel_Engineering(ch,s->latest_voltage_frame[ch]),g_labdaq_control.channel[ch].unit);LABDAQ_DisplayHW_Text(0,line);
  LABDAQ_DisplayHW_Graph(g_labdaq_display.history,LABDAQ_DISPLAY_HISTORY,g_labdaq_display.history_head,"Time",g_labdaq_control.channel[ch].unit); break;
 case LABDAQ_DISPLAY_TEST_GRAPH:
  LABDAQ_DisplayHW_Text(0,"Experiment curve (X/Y mapping configurable)");
  LABDAQ_DisplayHW_Graph(g_labdaq_display.history,LABDAQ_DISPLAY_HISTORY,g_labdaq_display.history_head,"Displacement / Strain","Load / Stress"); break;
 case LABDAQ_DISPLAY_EXPERIMENT:
  snprintf(line,sizeof(line),"Type %d | F %.2f Hz",(int)g_labdaq_control.experiment_type,g_labdaq_control.frequency_hz);LABDAQ_DisplayHW_Text(0,line);
  snprintf(line,sizeof(line),"Cycles %lu/%lu",(unsigned long)s->cyclic_gen.current_cycle,(unsigned long)g_labdaq_control.target_cycles);LABDAQ_DisplayHW_Text(1,line);
  snprintf(line,sizeof(line),"Confining %.2f | Axial %.2f",g_labdaq_control.confining_pressure,g_labdaq_control.axial_target);LABDAQ_DisplayHW_Text(2,line); break;
 case LABDAQ_DISPLAY_CONTROL:
  for(uint8_t m=0;m<2;m++){labdaq_pid_t*p=&g_labdaq_control.motor_pid[m];snprintf(line,sizeof(line),"EP%u SP %.2f PID %.2f/%.2f/%.2f %s",m+1,p->setpoint,p->kp,p->ki,p->kd,g_labdaq_control.motor_cal[m].calibrated?"CAL":"UNCAL");LABDAQ_DisplayHW_Text(m,line);} break;
 case LABDAQ_DISPLAY_ALARMS:
  LABDAQ_DisplayHW_Text(0,"Recent events / alarms"); for(uint16_t i=0;i<g_labdaq_control.log_count && i<6;i++){uint16_t idx=(g_labdaq_control.log_head+LABDAQ_LOG_CAPACITY-1U-i)%LABDAQ_LOG_CAPACITY;LABDAQ_DisplayHW_Text(i+1,g_labdaq_control.logs[idx].text);} break;
 case LABDAQ_DISPLAY_SYSTEM:
  LABDAQ_DisplayHW_Text(0,"STM32F407 | 16-channel DAQ");snprintf(line,sizeof(line),"Firmware %d.%d.%d",LABDAQ_FW_VERSION_MAJOR,LABDAQ_FW_VERSION_MINOR,LABDAQ_FW_VERSION_PATCH);LABDAQ_DisplayHW_Text(1,line);LABDAQ_DisplayHW_Text(2,"Web / SCPI / Modbus / UDP");LABDAQ_DisplayHW_Text(3,"Local HMI LCD adapter ready"); break;
 default: break;
 }
 LABDAQ_DisplayHW_EndFrame();
}
void LABDAQ_Display_Task(labdaq_system_t *s){
 if (!s) { return; }\n uint32_t now = HAL_GetTick();
 if(now-g_labdaq_display.last_refresh_ms<100U && !g_labdaq_display.dirty)return;
 g_labdaq_display.last_refresh_ms=now;
 uint8_t ch=g_labdaq_display.selected_channel;
 g_labdaq_display.history[g_labdaq_display.history_head]=LABDAQ_Channel_Engineering(ch,s->latest_voltage_frame[ch]);
 g_labdaq_display.history_head=(g_labdaq_display.history_head+1U)%LABDAQ_DISPLAY_HISTORY;
 render(s);g_labdaq_display.dirty=false;
}
