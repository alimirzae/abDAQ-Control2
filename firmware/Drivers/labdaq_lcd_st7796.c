#include "labdaq_display.h"
#include "labdaq_config.h"
#include "stm32f4xx_hal.h"
#include <ctype.h>
#include <string.h>

/*
 * EWB-STM32F407 Rev2.0 4-inch 40-pin LCD adapter.
 * Board schematic: SPI1 SCK=PA5, MISO=PA6, MOSI=PB5,
 * LCD_CS=PD13, LCD_RST=PD14, LCD_DC=PD15, BL_EN=PB0.
 * YT400S006 is driven here with the ST7796S-compatible command set,
 * 480x320 landscape, RGB565.
 */

static SPI_HandleTypeDef hspi1_lcd;
static uint16_t fg=0xFFFFU, bg=0x0000U;

#define LCD_CS(v)  HAL_GPIO_WritePin(LABDAQ_LCD_CS_PORT,LABDAQ_LCD_CS_PIN,(v)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define LCD_DC(v)  HAL_GPIO_WritePin(LABDAQ_LCD_DC_PORT,LABDAQ_LCD_DC_PIN,(v)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define LCD_RST(v) HAL_GPIO_WritePin(LABDAQ_LCD_RST_PORT,LABDAQ_LCD_RST_PIN,(v)?GPIO_PIN_SET:GPIO_PIN_RESET)

static void tx(const uint8_t *p,uint16_t n){HAL_SPI_Transmit(&hspi1_lcd,(uint8_t*)p,n,100);}
static void cmd(uint8_t c){LCD_DC(0);LCD_CS(0);tx(&c,1);LCD_CS(1);}
static void data(const uint8_t *p,uint16_t n){LCD_DC(1);LCD_CS(0);tx(p,n);LCD_CS(1);}
static void wr(uint8_t c,const uint8_t *p,uint16_t n){cmd(c);if(n)data(p,n);}

static void window(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){
 uint8_t d[4];
 d[0]=x0>>8;d[1]=x0;d[2]=x1>>8;d[3]=x1;wr(0x2A,d,4);
 d[0]=y0>>8;d[1]=y0;d[2]=y1>>8;d[3]=y1;wr(0x2B,d,4);
 cmd(0x2C);
}
static void fill(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color){
 if(!w||!h||x>=LABDAQ_LCD_WIDTH||y>=LABDAQ_LCD_HEIGHT)return;
 if(x+w>LABDAQ_LCD_WIDTH)w=LABDAQ_LCD_WIDTH-x;
 if(y+h>LABDAQ_LCD_HEIGHT)h=LABDAQ_LCD_HEIGHT-y;
 window(x,y,x+w-1,y+h-1); LCD_DC(1);LCD_CS(0);
 uint8_t b[128];for(unsigned i=0;i<sizeof(b);i+=2){b[i]=color>>8;b[i+1]=color;}
 uint32_t px=(uint32_t)w*h;
 while(px){uint16_t n=(px>64U)?64U:(uint16_t)px;tx(b,n*2U);px-=n;}
 LCD_CS(1);
}

static const uint8_t *glyph(char c){
 static uint8_t g[5];
 if(c>='a'&&c<='z')c=(char)(c-'a'+'A');
 memset(g,0,5);
 switch(c){
 case 'A':{uint8_t a[5]={0x7E,0x11,0x11,0x11,0x7E};memcpy(g,a,5);}break;
 case 'B':{uint8_t a[5]={0x7F,0x49,0x49,0x49,0x36};memcpy(g,a,5);}break;
 case 'C':{uint8_t a[5]={0x3E,0x41,0x41,0x41,0x22};memcpy(g,a,5);}break;
 case 'D':{uint8_t a[5]={0x7F,0x41,0x41,0x22,0x1C};memcpy(g,a,5);}break;
 case 'E':{uint8_t a[5]={0x7F,0x49,0x49,0x49,0x41};memcpy(g,a,5);}break;
 case 'F':{uint8_t a[5]={0x7F,0x09,0x09,0x09,0x01};memcpy(g,a,5);}break;
 case 'G':{uint8_t a[5]={0x3E,0x41,0x49,0x49,0x7A};memcpy(g,a,5);}break;
 case 'H':{uint8_t a[5]={0x7F,0x08,0x08,0x08,0x7F};memcpy(g,a,5);}break;
 case 'I':{uint8_t a[5]={0x00,0x41,0x7F,0x41,0x00};memcpy(g,a,5);}break;
 case 'J':{uint8_t a[5]={0x20,0x40,0x41,0x3F,0x01};memcpy(g,a,5);}break;
 case 'K':{uint8_t a[5]={0x7F,0x08,0x14,0x22,0x41};memcpy(g,a,5);}break;
 case 'L':{uint8_t a[5]={0x7F,0x40,0x40,0x40,0x40};memcpy(g,a,5);}break;
 case 'M':{uint8_t a[5]={0x7F,0x02,0x0C,0x02,0x7F};memcpy(g,a,5);}break;
 case 'N':{uint8_t a[5]={0x7F,0x04,0x08,0x10,0x7F};memcpy(g,a,5);}break;
 case 'O':{uint8_t a[5]={0x3E,0x41,0x41,0x41,0x3E};memcpy(g,a,5);}break;
 case 'P':{uint8_t a[5]={0x7F,0x09,0x09,0x09,0x06};memcpy(g,a,5);}break;
 case 'Q':{uint8_t a[5]={0x3E,0x41,0x51,0x21,0x5E};memcpy(g,a,5);}break;
 case 'R':{uint8_t a[5]={0x7F,0x09,0x19,0x29,0x46};memcpy(g,a,5);}break;
 case 'S':{uint8_t a[5]={0x46,0x49,0x49,0x49,0x31};memcpy(g,a,5);}break;
 case 'T':{uint8_t a[5]={0x01,0x01,0x7F,0x01,0x01};memcpy(g,a,5);}break;
 case 'U':{uint8_t a[5]={0x3F,0x40,0x40,0x40,0x3F};memcpy(g,a,5);}break;
 case 'V':{uint8_t a[5]={0x1F,0x20,0x40,0x20,0x1F};memcpy(g,a,5);}break;
 case 'W':{uint8_t a[5]={0x3F,0x40,0x38,0x40,0x3F};memcpy(g,a,5);}break;
 case 'X':{uint8_t a[5]={0x63,0x14,0x08,0x14,0x63};memcpy(g,a,5);}break;
 case 'Y':{uint8_t a[5]={0x07,0x08,0x70,0x08,0x07};memcpy(g,a,5);}break;
 case 'Z':{uint8_t a[5]={0x61,0x51,0x49,0x45,0x43};memcpy(g,a,5);}break;
 case '0':{uint8_t a[5]={0x3E,0x51,0x49,0x45,0x3E};memcpy(g,a,5);}break;
 case '1':{uint8_t a[5]={0x00,0x42,0x7F,0x40,0x00};memcpy(g,a,5);}break;
 case '2':{uint8_t a[5]={0x42,0x61,0x51,0x49,0x46};memcpy(g,a,5);}break;
 case '3':{uint8_t a[5]={0x21,0x41,0x45,0x4B,0x31};memcpy(g,a,5);}break;
 case '4':{uint8_t a[5]={0x18,0x14,0x12,0x7F,0x10};memcpy(g,a,5);}break;
 case '5':{uint8_t a[5]={0x27,0x45,0x45,0x45,0x39};memcpy(g,a,5);}break;
 case '6':{uint8_t a[5]={0x3C,0x4A,0x49,0x49,0x30};memcpy(g,a,5);}break;
 case '7':{uint8_t a[5]={0x01,0x71,0x09,0x05,0x03};memcpy(g,a,5);}break;
 case '8':{uint8_t a[5]={0x36,0x49,0x49,0x49,0x36};memcpy(g,a,5);}break;
 case '9':{uint8_t a[5]={0x06,0x49,0x49,0x29,0x1E};memcpy(g,a,5);}break;
 case '.':g[2]=0x60;break; case ':':g[2]=0x36;break; case '-':g[2]=0x08;break;
 case '/':{uint8_t a[5]={0x20,0x10,0x08,0x04,0x02};memcpy(g,a,5);}break;
 case '|':g[2]=0x7F;break; case ' ':default:break;
 }
 return g;
}
static void chr(uint16_t x,uint16_t y,char c,uint8_t scale,uint16_t color){
 const uint8_t *g=glyph(c);
 for(uint8_t col=0;col<5;col++)for(uint8_t row=0;row<7;row++)
  if(g[col]&(1U<<row))fill(x+col*scale,y+row*scale,scale,scale,color);
}
static void textxy(uint16_t x,uint16_t y,const char *s,uint8_t scale,uint16_t color){
 while(*s&&x+(6U*scale)<LABDAQ_LCD_WIDTH){chr(x,y,*s++,scale,color);x+=6U*scale;}
}

static void lcd_hw_init(void){
 __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOD_CLK_ENABLE();__HAL_RCC_SPI1_CLK_ENABLE();
 GPIO_InitTypeDef g={0};
 g.Pin=LABDAQ_LCD_CS_PIN|LABDAQ_LCD_RST_PIN|LABDAQ_LCD_DC_PIN;g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;HAL_GPIO_Init(GPIOD,&g);
 g.Pin=LABDAQ_LCD_SPI_SCK_PIN;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF5_SPI1;HAL_GPIO_Init(GPIOA,&g);
 g.Pin=LABDAQ_LCD_SPI_MISO_PIN;HAL_GPIO_Init(GPIOA,&g);
 g.Pin=LABDAQ_LCD_SPI_MOSI_PIN;HAL_GPIO_Init(GPIOB,&g);
 LCD_CS(1);LCD_DC(1);LCD_RST(1);
 hspi1_lcd.Instance=SPI1;hspi1_lcd.Init.Mode=SPI_MODE_MASTER;hspi1_lcd.Init.Direction=SPI_DIRECTION_2LINES;
 hspi1_lcd.Init.DataSize=SPI_DATASIZE_8BIT;hspi1_lcd.Init.CLKPolarity=SPI_POLARITY_LOW;hspi1_lcd.Init.CLKPhase=SPI_PHASE_1EDGE;
 hspi1_lcd.Init.NSS=SPI_NSS_SOFT;hspi1_lcd.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_4;hspi1_lcd.Init.FirstBit=SPI_FIRSTBIT_MSB;
 hspi1_lcd.Init.TIMode=SPI_TIMODE_DISABLE;hspi1_lcd.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;hspi1_lcd.Init.CRCPolynomial=7;HAL_SPI_Init(&hspi1_lcd);
 HAL_Delay(20);LCD_RST(0);HAL_Delay(20);LCD_RST(1);HAL_Delay(120);
 cmd(0x01);HAL_Delay(120);
 {uint8_t d=0x55;wr(0x3A,&d,1);} /* RGB565 */
 {uint8_t d=0x28;wr(0x36,&d,1);} /* landscape/BGR */
 {uint8_t d[]={0x80,0x02,0x3B};wr(0xF0,d,3);}
 {uint8_t d[]={0x80,0x02,0x3B};wr(0xF0,d,3);}
 {uint8_t d=0x00;wr(0xB4,&d,1);}
 {uint8_t d[]={0x80,0x02,0x02};wr(0xB6,d,3);}
 {uint8_t d=0xC6;wr(0xB7,&d,1);}
 {uint8_t d=0x24;wr(0xC5,&d,1);}
 cmd(0x11);HAL_Delay(120);cmd(0x29);HAL_Delay(20);
 fill(0,0,LABDAQ_LCD_WIDTH,LABDAQ_LCD_HEIGHT,0x0000);
}

void LABDAQ_DisplayHW_Init(void){
 lcd_hw_init();
 fill(0,0,LABDAQ_LCD_WIDTH,LABDAQ_LCD_HEIGHT,0x0000);
 textxy(24,25,"IMONITOR",3,0xFFFF);
 textxy(24,62,"AZERBAIJAN INDUSTRIAL PROCESSING CO.",1,0x07FF);
 textxy(24,82,"IMONITOR.IR",2,0xFFE0);
 textxy(24,120,"LABDAQ BOOT OK",2,0x07E0);
}
void LABDAQ_DisplayHW_BeginFrame(void){fill(0,0,LABDAQ_LCD_WIDTH,LABDAQ_LCD_HEIGHT,bg);}
void LABDAQ_DisplayHW_Header(const char*a,const char*b,const char*c,const char*d){
 (void)b;textxy(8,6,a,2,0xFFFF);textxy(8,25,c,1,0x07FF);textxy(300,6,d,1,0xFFE0);fill(0,40,LABDAQ_LCD_WIDTH,2,0x39E7);
}
void LABDAQ_DisplayHW_Text(uint16_t row,const char*t){uint16_t y=50U+row*16U;if(y<LABDAQ_LCD_HEIGHT-8U)textxy(8,y,t,1,fg);}
void LABDAQ_DisplayHW_Graph(const float*s,uint16_t n,uint16_t head,const char*x,const char*y){
 (void)x;(void)y;if(!s||n<2)return;uint16_t gx=20,gy=90,gw=440,gh=200;fill(gx,gy,gw,1,0x39E7);fill(gx,gy+gh,gw,1,0x39E7);fill(gx,gy,1,gh,0x39E7);
 float mn=s[0],mx=s[0];for(uint16_t i=1;i<n;i++){if(s[i]<mn)mn=s[i];if(s[i]>mx)mx=s[i];}if(mx-mn<0.001f)mx=mn+1.0f;
 for(uint16_t i=0;i<n;i++){uint16_t idx=(head+i)%n;float v=s[idx];uint16_t px=gx+(uint32_t)i*(gw-1)/(n-1);uint16_t py=gy+gh-1-(uint16_t)((v-mn)*(gh-1)/(mx-mn));fill(px,py,2,2,0x07E0);}
}
void LABDAQ_DisplayHW_EndFrame(void){}
