#include "stm32f0xx.h"
#include <stdio.h>
#include "config.h"
#include "util.h"
#include "W5200\w5200.h"
#include "W5200\socket.h"
#include "W5200\spi2.h"
#include "W5200\md5.h"
//#define ENABLE_PRINT

#include <string.h>
#include <stdlib.h>

#define START_ADDR 0x08007C00
#define RESET_CODE "0000000"
#define CODE_LEN 7
#define RFID_LEN 12

CONFIG_MSG Config_Msg;
CHCONFIG_TYPE_DEF Chconfig_Type_Def; 

// Configuration Network Information of W5200

uint8 Enable_DHCP = ON;
uint8 MAC[6] = {0x00, 0x08, 0xDC, 0x01, 0x02, 0x03};//MAC Address

//TX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
//RX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
uint8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};
uint8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};

uint8 ch_status[MAX_SOCK_NUM] = { 0, };	/** 0:close, 1:ready, 2:connected */

GPIO_InitTypeDef GPIO_InitStructure;
SPI_InitTypeDef SPI_InitStructure;
USART_InitTypeDef USART_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef TIM_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
TIM_OCInitTypeDef TIM_OCInitStructure;

char keymap[4][3] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

uint8 IPd[4] = {192, 168, 0, 123};      //IP Address
uint8 GWd[4] = {192, 168, 0, 1};        //Gateway Address
uint8 MASKd[4] = {255, 255, 255, 0};    //SubnetMask Address
uint8 Dest_IPd[4] = {176, 9, 145, 239}; // Default DST_IP Address 
const char check[] = "check.php";
const char reglock[] = "register.php";
const char host[] = "rskd.vipserv.org";
const char def_pass[] = "4586231274132528320310200DE032BF4";

uint8 temp[51];
uint8 *conf_byte;       // Bajt konfiguracji
uint8 *time;            // Czas otwarcia zamka
char *admin1;           // Kod admin1 4586231
char *admin2;           // Kod admin2 2741325
char *service;          // Kod service 2832031
char *rfidadmin;        // Klucz RFID 0200DE032BF4
uint8 *Dest_IP;         // IP serwera
uint8 *IP;              // IP zamka 
uint8 *GW;              // Brama domyœlna
uint8 *MASK;            // Maska podsieci

uint16 locknum;         // Numer zamka
uint16 PORT;            // Port do po³¹czenia z serwerem

uint8 *ptr = 0;         // Uniwersalny wskaŸnik danych u¿ywany do programowania zamka

// Bufory zawieraj¹ce kody i numery RFID
char rfid_buffer[RFID_LEN];
char buffer[CODE_LEN] = "xxxxxxx";

int8 buffer_ptr = 0;
uint8 rfid_buffer_ptr = 0;

// Zmienne pomocnicze
uint8 row = 0;
uint8 octet = 0;

// Funkcje inicjalizuj¹ce
void Periph_Init();
void Int_Init();
void WDG_Init();
void WIZ_Config();
void TIM_Init();

// Funkcje obs³ugi zamka
void open();
void delay();
void Clear_Buffer();
void Check_Buffer();
void Check_rfidBuffer();
void Check_Reset();
void Register_Lock();
int askServer(const char*, int, int);

// Funkcje zarz¹dzania pamiêci¹
void MemInit();
void FactorySet();
uint8 MemoryCheck();

// Funkcje obs³ugi trybu serwisowego
void ServiceMode_Enter();
void ServiceMode_Exit();
void ServiceMode_Check();
void ServiceMode_Reflash();
void ServiceMode_ChP();
void ServiceMode_ChIP();

// WskaŸnik funkcji sprawdzaj¹cej kod
void (*Check_Function)() = Check_Buffer;

// Sól do hashowania kodów
const char salt[] = "S722J3t3S68pZ1pN";

/*******************************************
 *
 *    MAIN
 *
 ******************************************/

int main()
{
  
  Delay_ms(100);
  Periph_Init();
  GPIO_SetBits(GPIOB, GPIO_Pin_5);
  TIM_init();
  WDG_Init();
  Int_Init();    
  IWDG_Enable();
  MemInit();
  RCC_ClearFlag();
  
  if(*conf_byte & 0x01)
    GPIO_SetBits(GPIOA, GPIO_Pin_2);
  GPIO_SetBits(GPIOF, GPIO_Pin_7);
  
  Reset_W5200();
  saveSUBR(MASK);
  setSUBR();
  setGAR(GW);
  setSIPR(IP);
  WIZ_Config();
  
  while(1){
    if ( row == 3 )
      row = 0;
    else
      row++;
 
    GPIO_ResetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_5 | GPIO_Pin_3);
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
    
    switch (row){
      case 0:
        GPIO_SetBits(GPIOB, GPIO_Pin_8);
        break;
      case 1:
        GPIO_SetBits(GPIOA, GPIO_Pin_15);
        break;
      case 2:
        GPIO_SetBits(GPIOB, GPIO_Pin_3);
        break;
      case 3:
        GPIO_SetBits(GPIOB, GPIO_Pin_5);
        break;
      default:
        break;
    }
    delay();   
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)){
      if(row == 3){
          Check_Reset();
          Check_Function();
      }
      else
        buffer[buffer_ptr] = keymap[row][2];
      while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4));
    }
    else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)){
      buffer[buffer_ptr] = keymap[row][1];
      while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9));
    }
    else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6)){
      if(row == 3)
        Clear_Buffer();
      else
        buffer[buffer_ptr] = keymap[row][0];
      while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6));
    }
    else{
      buffer_ptr--;
    }
    
    if( buffer_ptr >= CODE_LEN-1 )
      buffer_ptr = 0;
    else
      buffer_ptr++;
    IWDG_ReloadCounter();
  }
}

void delay(){
  for (int j = 99999; j > 0; j--);
}

void open(){
  GPIO_SetBits(GPIOB, GPIO_Pin_13); // Otwórz zaczep
  GPIO_SetBits(GPIOF, GPIO_Pin_6);  // Zaœwieæ diodê zielon¹
  GPIO_ResetBits(GPIOF, GPIO_Pin_7);// Zgaœ diodê czerwon¹
  GPIO_ResetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_5 | GPIO_Pin_3); 
  GPIO_SetBits(GPIOB, GPIO_Pin_5); 
  GPIO_ResetBits(GPIOA, GPIO_Pin_15);
    
  for(int i = 0; i < time[0]*100; i++){
    IWDG_ReloadCounter();
    Delay_ms(10);
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6))
      break;
  }
  
  GPIO_ResetBits(GPIOF, GPIO_Pin_6);
  GPIO_ResetBits(GPIOA, GPIO_Pin_4);
  GPIO_SetBits(GPIOF, GPIO_Pin_7);
  GPIO_ResetBits(GPIOB, GPIO_Pin_13);
}

void MemInit(){
  conf_byte = temp;
  time = conf_byte+1;
  admin1 = (char*)time+1;
  admin2 = admin1+CODE_LEN;
  service = admin2+CODE_LEN;
  rfidadmin = service+CODE_LEN;
  Dest_IP = (uint8*)rfidadmin+RFID_LEN; 
  IP = Dest_IP+4;
  GW = IP+4;
  MASK = GW+4;
  memcpy(temp, (const void*)START_ADDR, sizeof(temp));
  memcpy(&locknum, (const void*)(START_ADDR+sizeof(temp)+1), 2);
  memcpy(&PORT, (const void*)(START_ADDR+sizeof(temp)+3), 2);
  if(!MemoryCheck() || RCC_GetFlagStatus(RCC_FLAG_IWDGRST) || (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) && GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)))
    FactorySet();
}

void FactorySet(){
  locknum = 2;
  PORT = 80;
  *conf_byte = 1;
  *time = 3;
  memcpy((void*)admin1, (const void*)def_pass, sizeof(def_pass)-1);
  memcpy((void*)Dest_IP, (const void*)Dest_IPd, 4);
  memcpy((void*)IP, (const void*)IPd, 4);
  memcpy((void*)GW, (const void*)GWd, 4);
  memcpy((void*)MASK, (const void*)MASKd, 4);
  ServiceMode_Reflash();
}

uint8 MemoryCheck(){
  char codes[CODE_LEN*3];
  char rfid[RFID_LEN];
  memcpy(codes, (const void*)admin1, CODE_LEN*3);
  memcpy(rfid, (const void*)(admin1+CODE_LEN*3), RFID_LEN);
  for(int i = 0; i < CODE_LEN*3; i++)
    if(!((codes[i] >= '0' && codes[i] <= '9') || codes[i] == 'x'))
      return 0;
  for(int i = 0; i < RFID_LEN; i++)
    if(!((rfid[i] >= 'A' && rfid[i] <= 'Z') || (rfid[i] >= '0' && rfid[i] <= '9')))
      return 0;
  return 1;
}

void Clear_Buffer(){
  for( int j = 0; j < CODE_LEN; j++ )
    buffer[j] = 'x';
  buffer_ptr = -1;
}

void Check_Buffer(){
  if(askServer(buffer, CODE_LEN, 0)) open();
  else if( bufcmp(buffer, service, CODE_LEN)) ServiceMode_Enter();
  else if( bufcmp(buffer, admin1, CODE_LEN) || bufcmp(buffer, admin2, CODE_LEN)) open();
  Clear_Buffer();
}

void Register_Lock(){
  if(askServer(buffer, CODE_LEN, 1)) open();
  Clear_Buffer();
  ServiceMode_Enter();
}

void Check_rfidBuffer(){
  USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  if(askServer(rfid_buffer, RFID_LEN, 0) || bufcmp(rfid_buffer, rfidadmin, RFID_LEN))
    open();
  USART_RequestCmd(USART1, USART_Request_RXFRQ, ENABLE);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

int askServer(const char* ptr, int len, int reg){
  
  const uint8 newline[] = { '\r', '\n' };
  const char headers[] = "\r\nAccept-Language: en-GB\r\nAccept: text/html;q=0.9,text/plain;q=0.8\r\nAccept-Charset: utf-8\r\nAccept-Encoding: deflate\r\nTE: chunked\r\nContent-Length: 48\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n";
  uint8 dig[16];
  char digc[32];
  char approved[32];
  int resp = 0;
  signed int retry = 5;
  uint8 buf[256];
  char locknumc[5] = { '0', '0', '0', '0', '0' };
  int size;
  
  int16tostr(locknum, locknumc);
  
  md5_ctx md5context;
  md5_init(&md5context);
  md5_update(&md5context, (uint8*)ptr, len);
  md5_update(&md5context, (uint8*)salt, 16);
  md5_final(dig, &md5context);
  hextostr(dig, digc, 16);
  
  md5_init(&md5context);
  md5_update(&md5context, (uint8*)ptr, len);
  md5_update(&md5context, (uint8*)salt, 16);
  md5_update(&md5context, (uint8*)"OK", 2);
  md5_final(dig, &md5context);
  hextostr(dig, approved, 16);
  
  while(retry > 0){
    socket(0, Sn_MR_TCP, PORT, 0);
    Delay_ms(100);
    connect(0, Dest_IP, PORT);
    Delay_ms(100);
    
    if(getSn_SR(0) == 0x17){
      send(0, "POST /", 6, 0);
      if(reg)
        send(0, (const uint8*)reglock, sizeof(reglock)-1, 0);
      else
        send(0, (const uint8*)check, sizeof(check)-1, 0);
      send(0, " HTTP/1.1\r\nHost: ", 17, 0);
      send(0, (const uint8*)host, sizeof(host)-1, 0);
      send(0, (const uint8*)headers, sizeof(headers)-1, 0);
      send(0, "code=", 5, 0);
      send(0, (const uint8*)digc, 32, 0);
      send(0, "&lock=", 6, 0);
      send(0, (const uint8*)locknumc, 5, 0);
      send(0, newline, 2, 0);
      send(0, newline, 2, 0);
      
      while(!(size = getSn_RX_RSR(0)) && retry > 0) Delay_ms(100); retry--;
      if(size){
        recv(0, buf, size);
        if(bufcmp((char*)buf, "HTTP/1.1 200 OK\r\n", 17)){
          for(int i = 0; i < size; i++){
            if(buf[i] == '\r' && buf[i+2] == '\r'){
              char *endptr = "\r";
              int resp_size = strtol((char const*)&buf[i+4], &endptr, 10);
              if(resp_size == 20)
                if(bufcmp((char*)&buf[i+8], approved, sizeof(approved)))
                  resp = 1;
                else
                  resp = 0;
                break;
            }
          }
        }
      }
        break;
    } else {
      retry--;
    }
  }
  disconnect(0);
  close(0);
  return resp;
}

void Check_Reset(){
  if( bufcmp(buffer, RESET_CODE, CODE_LEN) )
    NVIC_SystemReset();
}

void ServiceMode_Enter(){
  USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  Check_Function = ServiceMode_Check;
  GPIO_SetBits(GPIOF, GPIO_Pin_7);
  GPIO_SetBits(GPIOF, GPIO_Pin_6);
  /* TRYB SERWISOWY ZAMKA:
  11. Otwarcie
  12. Zmiana kodu admin1
  13. Zmiana kodu admin2
  14. Zmiana czasu otwarcia zamka
  15. Zmiana kodu serwisowego
  16. Zmiana administracyjnej karty rfid
  17. Zmiana IP serwera 
  18. W³¹czenie/wy³¹czenie podœwietlenia klawiatury
  21. Zmiana IP zamka           WYMAGANY RESTART
  22. Zmiana bramy domyœlnej    WYMAGANY RESTART
  23. Zmiana maski podsieci     WYMAGANY RESTART
  24. Zmiana portu docelowego serwera
  25. Zmiana numeru zamka
  26. Rejestrowanie zamka w systemie
  9.  Reset zamka
  0.  Wyjœcie
  */
}

void ServiceMode_Check(){
  if( buffer[0] == '1' ){
    if( buffer[1] == '1' ){
      open();
      ServiceMode_Exit();
    }
    else if( buffer[1] == '2' ){
      ptr = admin1;
      Check_Function = ServiceMode_ChP;
    }
    else if( buffer[1] == '3'){
      ptr = admin2;
      Check_Function = ServiceMode_ChP;
    }
    else if( buffer[1] == '4' ){
      char *endptr = "x";
      buffer[4] = 'x'; 
      time[0] = strtol(&buffer[2], &endptr, 10);
      ServiceMode_Reflash();
    }
    else if( buffer[1] == '5'){
      ptr = service;
      Check_Function = ServiceMode_ChP;
    }
    else if( buffer[1] == '6' ){
      memcpy(rfidadmin, rfid_buffer, RFID_LEN);
      ServiceMode_Reflash();
      ServiceMode_Enter();
    }
    else if( buffer[1] == '7' ){
      ptr = Dest_IP;
      Check_Function = ServiceMode_ChIP;
    }
    else if( buffer[1] == '8' ){
      if(conf_byte[0] & 0x01){
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
        conf_byte[0] &= ~0x01;
        ServiceMode_Reflash();
      }
      else {
        GPIO_SetBits(GPIOA, GPIO_Pin_2);
        conf_byte[0] |= 0x01;
        ServiceMode_Reflash();
      }
    }
  }
  else if( buffer[0] == '2' ){
    if( buffer[1] == '1' ){
      ptr = IP;
      Check_Function = ServiceMode_ChIP;
    }
    else if( buffer[1] == '2' ){
      ptr = GW;
      Check_Function = ServiceMode_ChIP;
    }
    else if( buffer[1] == '3'){
      ptr = MASK;
      Check_Function = ServiceMode_ChIP;
    }
    else if( buffer[1] == '4' ){
      char *endptr = "x";
      buffer[6] = 'x'; 
      PORT = strtol(&buffer[2], &endptr, 10);
      ServiceMode_Reflash();
    }
    else if( buffer[1] == '5' ){
      char *endptr = "x";
      buffer[6] = 'x'; 
      locknum = strtol(&buffer[2], &endptr, 10);
      ServiceMode_Reflash();
    }
    else if( buffer[1] == '6' ){
      Check_Function = Register_Lock;
    }
  }
  else if( buffer[0] == '9' ){
      NVIC_SystemReset();
  }
  else if( buffer[0] == '0' ){
      ServiceMode_Exit();
  }
  Clear_Buffer();
}

void ServiceMode_Exit(){
  Check_Function = Check_Buffer;
  GPIO_ResetBits(GPIOF, GPIO_Pin_6);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void ServiceMode_ChP(){
  if(ptr == admin1 || ptr == admin2 || ptr == service){
    memcpy(ptr, buffer, CODE_LEN);
    ServiceMode_Reflash();
    Clear_Buffer();
    ServiceMode_Enter();
  }
}

void ServiceMode_ChIP(){
  if(ptr == Dest_IP || ptr == IP || ptr == MASK || ptr == GW){
    char *endptr = "x";
    ptr[octet] = (strtol(buffer, &endptr, 10) > 255 ? 255 : strtol(buffer, &endptr, 10));
    Clear_Buffer();
    if( octet < 3 ){
      octet++;
    } else {
      ServiceMode_Reflash();
      octet = 0;
      ServiceMode_Enter();
    }
  }
}

void ServiceMode_Reflash(){
  int i;
  do{
    FLASH_Unlock();
    FLASH_ErasePage(START_ADDR);
    i = 0;
    for (; i < sizeof(temp); i=i+4){
      FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPERR);
      FLASH_ProgramWord(START_ADDR+i, temp[i] | temp[i+1]<<8 | temp[i+2]<<16 | temp[i+3]<<24);
    }
    FLASH_ProgramWord(START_ADDR+i, locknum | PORT<<16);
    FLASH_Lock();
  } while(!MemoryCheck());
  memcpy(temp, (const void*)START_ADDR, sizeof(temp));
  memcpy(&locknum, (const void*)(START_ADDR+i), 2);
  memcpy(&PORT, (const void*)(START_ADDR+i+2), 2);
}

void WIZ_Config(void)
{
  // Call Set_network(): Set network configuration, Init. TX/RX MEM SIZE., and  Set RTR/RCR
  Set_network(); // at util.c
}       

void WDG_Init(){
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_256);
  IWDG_SetReload(0xFFF);
}

void Int_Init(){
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_Init(&NVIC_InitStructure);

}

void TIM_Init(){
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  TIM_TimeBaseInitStructure.TIM_Prescaler = 0x00FF;
  TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStructure.TIM_Period = 0x00FF;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  
  TIM_Cmd(TIM2, ENABLED);
  
}

void Periph_Init(){
 
 // Pod³¹czenie sygna³u zegara do peryferiów
 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
 
 // Ustawienie funkcji alternatywnych dla wejœæ/wyjœæ
 GPIO_PinAFConfig(GPIOA, GPIO_PinSource4 | GPIO_PinSource6 | GPIO_PinSource5 | GPIO_PinSource7, GPIO_AF_0);
 GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_0);
 
 // W5200
  /***************
  *              *
  *  PA4  - NSS  *
  *  PA5  - SCK  *
  *  PA6  - MISO *
  *  PA7  - MOSI *
  *              *
  ****************/
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7| GPIO_Pin_6;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 // PA4
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 // PB0 - PWDN, PB1 - RST
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
 GPIO_Init(GPIOB, &GPIO_InitStructure);
 // PA3 - INT
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 
 // RDM6300
 // PB7 - RX USART1 
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
 GPIO_Init(GPIOB, &GPIO_InitStructure);
 
 // Klawiatura
 // Kolumny
  /***************
  *              *
  *  PB9  -  C2  *
  *  PB6  -  C1  *
  *  PB4  -  C3  *
  *              *
  ****************/
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_6 | GPIO_Pin_4;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
 GPIO_Init(GPIOB, &GPIO_InitStructure);
 
 // Rzêdy
 /****************
  *              *
  *  PB8  -  R1  *
  *  PB5  -  R4  *
  *  PB3  -  R3  *
  *  PA15 -  R2  *
  *              *
  ****************/
 // PB8, PB5, PB3
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_5 | GPIO_Pin_3;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOB, &GPIO_InitStructure);
 // PA15
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 
 // Diody i elektrozaczep
 // PC8 - Test, PC9 - Test
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_8 | GPIO_Pin_9;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOC, &GPIO_InitStructure);
 // PF6 - Dioda zielona, PF7 - Dioda czerwona
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOF, &GPIO_InitStructure);
 // PA2 - Podœwietlenie klawiatury
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 // PB13 - Elektrozaczep
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
 GPIO_Init(GPIOB, &GPIO_InitStructure);
 
 // SPI1 do W5200
 //SPI_I2S_DeInit(SPI1);
 SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
 SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
 SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
 SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
 SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
 SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
 SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
 SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
 SPI_Init(SPI1, &SPI_InitStructure);
 SPI_InitStructure.SPI_CRCPolynomial = 7;
 SPI_Cmd(SPI1, ENABLE);
 
 // UART1 do RDM6300
 USART_InitStructure.USART_BaudRate = 9600;
 USART_InitStructure.USART_WordLength = USART_WordLength_8b;
 USART_InitStructure.USART_StopBits = USART_StopBits_1;
 USART_InitStructure.USART_Parity = USART_Parity_No;
 USART_InitStructure.USART_Mode = USART_Mode_Rx;
 USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
 USART_Init(USART1, &USART_InitStructure);
 USART_OverrunDetectionConfig(USART1, USART_OVRDetection_Disable);
 USART_Cmd(USART1, ENABLE);
}

// Handlery do przerwañ
void USART1_IRQHandler (void){
  char data = USART_ReceiveData(USART1);
  if(data == 0x02){
    rfid_buffer_ptr = 0;
  }
  else if(data == 0x03){
    Check_rfidBuffer();
  }
  else{
    rfid_buffer[rfid_buffer_ptr] = data;
    if(rfid_buffer_ptr == RFID_LEN-1)
      rfid_buffer_ptr = 0;
    else
      rfid_buffer_ptr++;
  }
  USART_RequestCmd(USART1, USART_Request_RXFRQ, ENABLE);
}

void HardFault_Handler (unsigned int * hardfault_args){
  NVIC_SystemReset();
}