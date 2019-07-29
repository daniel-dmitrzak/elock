#include "stm32f0xx.h"
#include "config.h"
#include "util.h"
#include "W5200\w5200.h"
#include <stdio.h>
#include <stdarg.h>
#define ENABLE_PRINT
//extern CONFIG_MSG Config_Msg;
extern CHCONFIG_TYPE_DEF Chconfig_Type_Def; 

extern uint8 txsize[MAX_SOCK_NUM];
extern uint8 rxsize[MAX_SOCK_NUM];


#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

extern uint8 MAC[6];
extern uint8 IP[4];
extern uint8 GateWay[4];
extern uint8 SubNet[4];
extern uint8 Enable_DHCP;
extern uint8 Dest_IP[4] ;
extern uint16 Dest_PORT ;

void Set_network(void)
{
        //uint8 tmp_array[6];       
        //uint8 i;
        
        // MAC ADDRESS
        /*
        for (i = 0 ; i < 6; i++) Config_Msg.Mac[i] = MAC[i];
        // Local IP ADDRESS
        Config_Msg.Lip[0] = IP[0]; Config_Msg.Lip[1] = IP[1]; Config_Msg.Lip[2] = IP[2]; Config_Msg.Lip[3] = IP[3];
        // GateWay ADDRESS
        Config_Msg.Gw[0] = GateWay[0]; Config_Msg.Gw[1] = GateWay[1]; Config_Msg.Gw[2] = GateWay[2]; Config_Msg.Gw[3] = GateWay[3];
        // Subnet Mask ADDRESS
        Config_Msg.Sub[0] = SubNet[0]; Config_Msg.Sub[1] = SubNet[1]; Config_Msg.Sub[2] = SubNet[2]; Config_Msg.Sub[3] = SubNet[3];
        */
        setSHAR(MAC);
//        setSUBR(Config_Msg.Sub);
        saveSUBR(SubNet);
        setGAR(GateWay);
        setSIPR(IP);

        // Set DHCP
        //Config_Msg.DHCP = Enable_DHCP;    
        //Destination IP address for TCP Client
        Chconfig_Type_Def.destip[0] = Dest_IP[0]; Chconfig_Type_Def.destip[1] = Dest_IP[1];
        Chconfig_Type_Def.destip[2] = Dest_IP[2]; Chconfig_Type_Def.destip[3] = Dest_IP[3];
        Chconfig_Type_Def.port = Dest_PORT;

        //Set PTR and RCR register	
        setRTR(6000);
        setRCR(3);

        //Init. TX & RX Memory size
        sysinit(txsize, rxsize); 
}



void Reset_W5200(void)
{
	GPIO_ResetBits(GPIOB, WIZ_RESET);
	Delay_us(2);  
	GPIO_SetBits(GPIOB, WIZ_RESET);
	Delay_ms(150);  
}


void LED3_onoff(uint8_t on_off)
{
	if (on_off == ON) {
		GPIO_ResetBits(GPIOA, LED3); // LED on
	}else {
		GPIO_SetBits(GPIOA, LED3); // LED off
	}
}

void LED4_onoff(uint8_t on_off)
{
	if (on_off == ON) {
		GPIO_ResetBits(GPIOA, LED4); // LED on
	}else {
		GPIO_SetBits(GPIOA, LED4); // LED off
	}
}

void USART1_Init(void)
{
USART_InitTypeDef USART_InitStructure;

/* USARTx configuration ------------------------------------------------------*/
  /* USARTx configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No ;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
  /* Configure the USARTx */ 
  USART_Init(USART1, &USART_InitStructure);
  /* Enable the USARTx */
  USART_Cmd(USART1, ENABLE);


}


/*******************************************************************************
* Function Name  : Delay_us
* Description    : Delay per micro second.
* Input          : time_us
* Output         : None
* Return         : None
*******************************************************************************/

void Delay_us( uint8 time_us )
{
  register uint8 i;
  register uint8 j;
  for( i=0;i<time_us;i++ )    
  {
    for( j=0;j<3;j++ )          // 20CLK
    {        
    }      
  }                            
}

/*******************************************************************************
* Function Name  : Delay_ms
* Description    : Delay per mili second.
* Input          : time_ms
* Output         : None
* Return         : None
*******************************************************************************/

void Delay_ms( uint16 time_ms )
{
  register uint16 i;
  for( i=0;i<time_ms;i++ )
  {
    Delay_us(250);
    Delay_us(250);
    Delay_us(250);
    Delay_us(250);
  }
}

/*******************************************************************************
* Function Name  : PUTCHAR_PROTOTYPE
* Description    : Retargets the C library printf function to the USART.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

#ifndef ENABLE_PRINT
PUTCHAR_PROTOTYPE
{
  return 0;
}
int putchar(int ch){
  return 0;
}
#endif

/*
PUTCHAR_PROTOTYPE
{
  // Write a character to the USART
  USART_SendData(USART1, (uint8_t) ch);

  //  Loop until the end of transmission 
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  {
  }

  return ch;
}


int putchar(int ch)
{
  // Write a character to the USART
  USART_SendData(USART1, (uint8_t) ch);

  //  Loop until the end of transmission 
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  {
  }

  return ch;
}

int getchar(void)
{
  int ch;

	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET){
	}

	ch = USART_ReceiveData(USART1);

  return ch;
} */

int bufcmp(char* b1, const char* b2, int len){
  int match = 0;
  for ( int i = 0; i < len; i++ )
    if ( b1[i] == b2[i] )
      match++;
  return ( match == len ? 1 : 0 );
}

void bufprint(char* buf, int len){
  for( int i = 0; i < len; i++ )
    printf("%c", buf[i]);
}

/*******************************************************************************
* Function Name  : time_return
* Description    :  
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

uint32_t time_return(void) 
{
  extern uint32_t my_time; 
  return my_time;
}


#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif



