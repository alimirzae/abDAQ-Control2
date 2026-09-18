#ifndef LABDAQ_DISPLAY_H
#define LABDAQ_DISPLAY_H
#include <stdint.h>
#include <stdbool.h>
#include "labdaq_sampler.h"

#define LABDAQ_DISPLAY_BRAND "iMonitor"
#define LABDAQ_DISPLAY_COMPANY "Azerbaijan Industrial Processing Co."
#define LABDAQ_DISPLAY_SITE "iMonitor.ir"
#define LABDAQ_DISPLAY_HISTORY 120

typedef enum {
 LABDAQ_DISPLAY_STATUS=0,
 LABDAQ_DISPLAY_SENSORS,
 LABDAQ_DISPLAY_LIVE_GRAPH,
 LABDAQ_DISPLAY_TEST_GRAPH,
 LABDAQ_DISPLAY_EXPERIMENT,
 LABDAQ_DISPLAY_CONTROL,
 LABDAQ_DISPLAY_ALARMS,
 LABDAQ_DISPLAY_SYSTEM,
 LABDAQ_DISPLAY_PAGE_COUNT
} labdaq_display_page_t;

typedef struct {
 labdaq_display_page_t page;
 uint8_t selected_channel;
 uint32_t last_refresh_ms;
 uint32_t last_button_ms;
 float history[LABDAQ_DISPLAY_HISTORY];
 uint16_t history_head;
 bool dirty;
} labdaq_display_t;

extern labdaq_display_t g_labdaq_display;
void LABDAQ_Display_Init(void);
void LABDAQ_Display_Task(labdaq_system_t *sys);
void LABDAQ_Display_NextPage(void);
void LABDAQ_Display_ButtonEvent(void);
void LABDAQ_Display_SelectChannel(uint8_t channel);
const char *LABDAQ_Display_PageName(labdaq_display_page_t page);

/* Hardware adapter hooks. Override these weak functions when LCD controller is selected. */
void LABDAQ_DisplayHW_Init(void);
void LABDAQ_DisplayHW_BeginFrame(void);
void LABDAQ_DisplayHW_Header(const char *brand,const char *company,const char *site,const char *page);
void LABDAQ_DisplayHW_Text(uint16_t row,const char *text);
void LABDAQ_DisplayHW_Graph(const float *samples,uint16_t count,uint16_t head,const char *x_label,const char *y_label);
void LABDAQ_DisplayHW_EndFrame(void);
#endif
