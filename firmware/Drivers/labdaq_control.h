#ifndef LABDAQ_CONTROL_H
#define LABDAQ_CONTROL_H
#include <stdint.h>
#include <stdbool.h>
#include "labdaq_sampler.h"
#define LABDAQ_LOG_CAPACITY 64
#define LABDAQ_NAME_LEN 24
typedef enum { LABDAQ_TEST_CYCLIC_TRIAXIAL=0, LABDAQ_TEST_MONOTONIC, LABDAQ_TEST_STRESS_CONTROLLED, LABDAQ_TEST_STRAIN_CONTROLLED, LABDAQ_TEST_CUSTOM } labdaq_experiment_type_t;
typedef struct { float kp,ki,kd,setpoint,integral,prev_error,output_min,output_max; bool enabled; } labdaq_pid_t;
typedef struct { float command_min_v,command_max_v,feedback_min,feedback_max,gain,offset; bool calibrated; } labdaq_motor_cal_t;
typedef struct { char name[LABDAQ_NAME_LEN]; char unit[12]; float gain,offset,min_value,max_value; bool enabled; } labdaq_channel_cal_t;
typedef struct { uint32_t timestamp_s; uint8_t level; char text[64]; } labdaq_log_entry_t;
typedef struct {
 uint32_t epoch_at_set; uint32_t tick_at_set;
 labdaq_pid_t motor_pid[2]; labdaq_motor_cal_t motor_cal[2];
 labdaq_channel_cal_t channel[LABDAQ_NUM_CHANNELS];
 labdaq_experiment_type_t experiment_type; float confining_pressure; float axial_target; float frequency_hz; uint32_t target_cycles;
 labdaq_log_entry_t logs[LABDAQ_LOG_CAPACITY]; uint16_t log_head,log_count;
} labdaq_control_t;
extern labdaq_control_t g_labdaq_control;
void LABDAQ_Control_Init(void);
void LABDAQ_Control_Task(labdaq_system_t *sys);
void LABDAQ_Time_SetUnix(uint32_t epoch);
uint32_t LABDAQ_Time_NowUnix(void);
void LABDAQ_Log(uint8_t level,const char *text);
void LABDAQ_Channel_Set(uint8_t ch,const char *name,const char *unit,float gain,float offset);
float LABDAQ_Channel_Engineering(uint8_t ch,float input);
void LABDAQ_PID_Set(uint8_t motor,float kp,float ki,float kd,float setpoint);
float LABDAQ_PID_Update(uint8_t motor,float feedback,float dt_s);
void LABDAQ_Motor_SetCalibration(uint8_t motor,float cmd_min,float cmd_max,float fb_min,float fb_max);
#endif
