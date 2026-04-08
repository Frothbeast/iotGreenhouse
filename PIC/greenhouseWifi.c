// This program is for my greenhouse control
// Created February 2022 Dan Jubenville in ccsc
// now porting to MPLAB XC8

/////////////////////////////////////////////////////////////////////////////////
//******************    Pre-processor definitions    ****************************
/////////////////////////////////////////////////////////////////////////////////
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "config.h"

// Direct register mapping for XC8 v3.10 compatibility
#ifndef CMCON
   #define CMCON  (*(volatile __near unsigned char*)0xFB4)
#endif
#define CVRCON (*(volatile __near unsigned char*)0xFB5)
#define PIR2_REG (*(volatile __near unsigned char*)0xFA1)


// Configuration for 18LF2580
#pragma config OSC = HS          
#pragma config WDT = OFF         
#pragma config WDTPS = 4096      
#pragma config BOREN = OFF       
#pragma config LVP = OFF        
#define _XTAL_FREQ 20000000

// Hardware Mapping
#define light_SSR          LATC0          // solid state relay for grow light
#define TRISlight          TRISC0         // solid state relay for grow light
#define fanSSR             LATB0          // solid state relay for fan
#define TRISfan            TRISB0         // solid state relay for fan
#define coolSSR            LATC1          // solid state relay for cooling
#define TRIScool           TRISC1         // solid state relay for cooling
#define heatSSR            LATC2          // solid state relay for heat  
#define TRISheat           TRISC2         // solid state relay for heat


#define Display_Pin        LATBbits.LATB4  
#define TRIS_Display       TRISB4          
#define thermistorIn       PIN_a3         
#define thermistorOut      PIN_a4         
#define thermistorbox      PIN_a2 
//#define dans_SPI_clock_pin PIN_c3
//#define dans_SPI_data_pin  PIN_c4
#define column1            PIN_b6         // 3 columns for keypad
#define column2            PIN_b7         // 1 2 3
#define column3            PIN_b2         // 4 5 6                                        
#define row1               PIN_c7         // 7 8 9     
#define row2               PIN_c5         // * 0 #
#define row3               PIN_a5         // 4 rows for keypad
#define row4               PIN_b5
#define sunlight_pin       pin_a0      // photresistor 5V on one side and this pin with 10k pulldown on other side
#define one_wire_pin       pin_a1      // one wire temp or other

// Circular Buffer for Display
#define DISP_BUF_SIZE 255
volatile char disp_buffer[DISP_BUF_SIZE];
volatile uint8_t disp_head = 0;
volatile uint8_t disp_tail = 0;

// Timing variables for Seetron serial backpack
uint16_t displayDelayCounter = 0; 

// Application Variables
volatile char error_msg[5] = " OK";
uint8_t wasOn = 0, wasOff = 0, triggerSecondCount = 0;
uint8_t highLevelStatus, lowLevelStatus, timeToDisplay = 0;
uint32_t secondsSincePowerup = 0;
uint16_t hoursSincePowerup = 0, currentOnTime = 0, currentOffTime = 0, secondsCounter = 0, duty = 50;
uint16_t lastOnTime = 0, lastOffTime = 0, espFails;
uint8_t pumpState = 0; 
uint8_t initialSendDone = 0;
uint16_t lowSampleCount = 0, highSampleCount = 0;
uint16_t tenMinuteCounter = 0;
uint8_t tenMinuteFlag = 0;
uint16_t backlightTime = 0;
uint8_t backlightState = 1;

// Raw and Filtered ADC values
uint16_t low_val = 0, high_val = 0;
uint16_t low_filtered = 1000, high_filtered = 1000;

// Data capture
uint32_t lowSum = 0, highSum = 0;
uint16_t lastLatod = 0, lastHatod = 0;

// --- NEW DATA BUFFERING LOGIC ---
#define MAX_RECORDS 10
typedef struct {
    uint16_t h_adc;
    uint16_t l_adc;
    uint16_t hrs;
    uint16_t on_t;
    uint16_t off_t;
} pump_record_t;

pump_record_t record_buffer[MAX_RECORDS];
uint8_t buf_head = 0; 
uint8_t buf_tail = 0; 
uint8_t records_pending = 0;

void save_to_buffer(uint16_t h, uint16_t l, uint16_t hrs, uint16_t ont, uint16_t offt) {
    if (records_pending < MAX_RECORDS) {
        record_buffer[buf_head].h_adc = h;
        record_buffer[buf_head].l_adc = l;
        record_buffer[buf_head].hrs = hrs;
        record_buffer[buf_head].on_t = ont;
        record_buffer[buf_head].off_t = offt;
        buf_head = (buf_head + 1) % MAX_RECORDS;
        records_pending++;
    } else {
        strcpy((char*)error_msg, "FULL");
    }
}

// ESP State Machine setup
typedef enum { 
    ESP_IDLE, ESP_START_CONNECT, ESP_WAIT_AT, ESP_WAIT_CONNECT, 
    ESP_START_SEND_CMD, ESP_WAIT_PROMPT, ESP_SEND_DATA, 
    ESP_WAIT_SEND_OK, ESP_WAIT_HANDSHAKE 
} esp_state_t;
esp_state_t currentEspState = ESP_IDLE;
uint16_t espTimer = 0;
volatile char rx_buf[64];
volatile uint8_t rx_idx = 0;


////////////////////////////////////////////////////////////////////////////////
//*********************** Routines or functions ********************************
/////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void put_to_disp_buf(const char* str);
void process_display_buffer(void);
int8_t updateDisplayCoord(uint8_t line, uint8_t column, const char* str);
void software_putch(char data); 
void uart_send_string(const char* s);
void process_esp_state_machine(void);
uint16_t read_adc(uint8_t channel);

// interrupt routines
void __interrupt() v_isr(void) {
    if (INTCONbits.TMR0IF) {
        secondsCounter++;
        if (secondsCounter >= 9223) { 
            secondsCounter = 0;
            triggerSecondCount = 1;
            if (espTimer > 0) espTimer--; 
        }
        INTCONbits.TMR0IF = 0; 
    }
    if (PIR1bits.RCIF) {
        char c = RCREG; 
        if (rx_idx < 63) {
            rx_buf[rx_idx++] = c;
            rx_buf[rx_idx] = '\0';
        }
        if (RCSTAbits.OERR) { RCSTAbits.CREN = 0; RCSTAbits.CREN = 1; }
    }
}

// read a/d
uint16_t read_adc(uint8_t channel) {
    ADCON0 = (uint8_t)((channel << 2) & 0x3C); 
    ADCON0bits.ADON = 1;
    __delay_us(20); 
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);
    return (uint16_t)((ADRESH << 8) | ADRESL);
}

//send character to display
void software_putch(char data) {
    uint8_t status = INTCONbits.GIE;
    INTCONbits.GIE = 0; 
    Display_Pin = 1; 
    __delay_us(104); 
    for(uint8_t b=0; b<8; b++) {
        if((data >> b) & 0x01) Display_Pin = 0;
        else Display_Pin = 1;
        __delay_us(104);
    }
    Display_Pin = 0; 
    __delay_us(104);
    INTCONbits.GIE = status; 
}

// build string for display
void put_to_disp_buf(const char* str) {
    while(*str) {
        uint8_t next = (disp_head + 1) % DISP_BUF_SIZE;
        if(next != disp_tail) {
            disp_buffer[disp_head] = *str++;
            disp_head = next;
        } else break;
    }
}

// send buffer to display
void process_display_buffer(void) {
    if (displayDelayCounter > 0) { displayDelayCounter--; return; }
    
    if(disp_head != disp_tail && currentEspState == ESP_IDLE) {
        char c = disp_buffer[disp_tail];
        software_putch(c);
        if (c < 32) displayDelayCounter = 100; 
        else displayDelayCounter = 2; 
        disp_tail = (disp_tail + 1) % DISP_BUF_SIZE;
    }
}

// manage display character location
int8_t updateDisplayCoord(uint8_t line, uint8_t column, const char* str) {
    uint8_t addr;
    switch (line) {
        case 1: addr = 64 + (column - 1); break; 
        case 2: addr = 84 + (column - 1); break; 
        case 3: addr = 104 + (column - 1); break;
        case 4: addr = 124 + (column - 1); break;
        default: return 0;
    }
    char cmd_seq[3] = {16, addr, '\0'};
    put_to_disp_buf(cmd_seq);
    put_to_disp_buf(str);
    return 1;
}

// send data to server
void uart_send_string(const char* s) {
    while(*s) {
        while(!PIR1bits.TXIF); 
        TXREG = *s++;          
    }
}

void process_esp_state_machine(void) {
    char data_str[24], cmd_str[64];
    if (pumpState == 1 && tenMinuteFlag == 0) { currentEspState = ESP_IDLE; return; }

    // Kick off connection if records are waiting
    if (currentEspState == ESP_IDLE && records_pending > 0) {
        currentEspState = ESP_START_CONNECT;
    }

    switch(currentEspState) {
        case ESP_IDLE: break;
        case ESP_START_CONNECT:
            RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;
            rx_idx = 0; rx_buf[0] = '\0';
            uart_send_string("ATE0\r\n");
            espTimer = 3; currentEspState = ESP_WAIT_AT; strcpy((char*)error_msg, "er1");
            break;
        case ESP_WAIT_AT:
            if(strstr((const char*)rx_buf, "OK")) {
                rx_idx = 0; rx_buf[0] = '\0';
                sprintf(cmd_str, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", SERVER_IP, SERVER_PORT);
                uart_send_string(cmd_str);
                espTimer = 10; currentEspState = ESP_WAIT_CONNECT; strcpy((char*)error_msg, "er2");
            } else if (espTimer == 0) {
                currentEspState = ESP_IDLE; strcpy((char*)error_msg, "er3");
                espFails++;
            }
            break;
        case ESP_WAIT_CONNECT:
            if(strstr((const char*)rx_buf, "OK") || strstr((const char*)rx_buf, "ALREADY CONNECTED")) {
                rx_idx = 0; rx_buf[0] = '\0';
                currentEspState = ESP_START_SEND_CMD; strcpy((char*)error_msg, "er4");
            } else if (espTimer == 0) { 
                currentEspState = ESP_IDLE; strcpy((char*)error_msg, "er5");
                espFails++;
            }
            break;
        case ESP_START_SEND_CMD:
            snprintf(data_str, sizeof(data_str), "%04X%04X%04X%04X%04X\r\n", 
                    record_buffer[buf_tail].h_adc, record_buffer[buf_tail].l_adc, 
                    record_buffer[buf_tail].hrs, record_buffer[buf_tail].on_t, 
                    record_buffer[buf_tail].off_t);
            sprintf(cmd_str, "AT+CIPSEND=%d\r\n", (int)strlen(data_str));
            rx_idx = 0; rx_buf[0] = '\0';
            uart_send_string(cmd_str);
            espTimer = 2; currentEspState = ESP_WAIT_PROMPT; strcpy((char*)error_msg, "er6");
            break;
        case ESP_WAIT_PROMPT:
            if(strstr((const char*)rx_buf, ">")) {
                rx_idx = 0; rx_buf[0] = '\0';
                currentEspState = ESP_SEND_DATA; strcpy((char*)error_msg, "er7");
            } else if (espTimer == 0) {
                currentEspState = ESP_IDLE; strcpy((char*)error_msg, "er8");
                espFails++;
            }
            break;
        case ESP_SEND_DATA:
            snprintf(data_str, sizeof(data_str), "%04X%04X%04X%04X%04X\r\n", 
                    record_buffer[buf_tail].h_adc, record_buffer[buf_tail].l_adc, 
                    record_buffer[buf_tail].hrs, record_buffer[buf_tail].on_t, 
                    record_buffer[buf_tail].off_t);
            rx_idx = 0; rx_buf[0] = '\0';
            uart_send_string(data_str);
            espTimer = 3; currentEspState = ESP_WAIT_SEND_OK; strcpy((char*)error_msg, "er9");
            break;
        case ESP_WAIT_SEND_OK:
            if(strstr((const char*)rx_buf, "SEND OK")) {
                rx_idx = 0; rx_buf[0] = '\0';
                espTimer = 5; 
                currentEspState = ESP_WAIT_HANDSHAKE;
                strcpy((char*)error_msg, "hnd");
            } else if (espTimer == 0) {
                currentEspState = ESP_IDLE; strcpy((char*)error_msg, "er0S");
                espFails++;
            }
            break;
        case ESP_WAIT_HANDSHAKE:
            if(strstr((const char*)rx_buf, "ACK")) {
                // Success: remove from buffer
                buf_tail = (buf_tail + 1) % MAX_RECORDS;
                records_pending--;
                
                updateDisplayCoord(4, 1, "Server: ACK         ");
                currentEspState = ESP_IDLE;
                strcpy((char*)error_msg, " OK");
            } else if(strstr((const char*)rx_buf, "ERR")) {
                updateDisplayCoord(4, 1, "Server: ERR         ");
                currentEspState = ESP_IDLE;
                strcpy((char*)error_msg, "eHD");
            } else if (espTimer == 0) {
                updateDisplayCoord(4, 1, "Server: H-OUT       ");
                currentEspState = ESP_IDLE;
                strcpy((char*)error_msg, "eTO");
                espFails++;
            }
            break;
    }
}


void main(void) {
    INTCON = 0x00; PIE1 = 0x00; RCSTA = 0x00;
    ADCON1 = 0x0D; ADCON2 = 0x92; TRISA = 0x07; TRISA3 = 0; TRIS_SSR = 0; 
    Display_Pin = 0; TRIS_Display = 0; CVRCON = 0x00; CMCON = 0x07;  
    TRISC6 = 0; TRISC7 = 1; 
    T0CON = 0xC0;       

    __delay_ms(100);
    software_putch(12); __delay_ms(100); software_putch(14);
    
    updateDisplayCoord(1, 1, "Sump Wifi Control");
    updateDisplayCoord(2, 2, "Dan Jubenville");
    updateDisplayCoord(3, 3, "January 2026");
    
    while(disp_head != disp_tail) { process_display_buffer(); }

    SPBRG = 10; TXSTA = 0x24; RCSTA = 0x90; 
    
    volatile char dummy;
    dummy = RCREG; dummy = RCREG;
    RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;          

    PIE1bits.RCIE = 1; INTCONbits.TMR0IE = 1; INTCONbits.PEIE = 1; INTCONbits.GIE = 1;    
    SENSOR_PWR = 1;

    while (1) {
        process_display_buffer(); 

        low_val = read_adc(0);
        high_val = read_adc(1);
        low_filtered = (uint16_t)((low_val >> 3) + (low_filtered - (low_filtered >> 3)));
        high_filtered = (uint16_t)((high_val >> 3) + (high_filtered - (high_filtered >> 3)));

        lowLevelStatus = (low_filtered < 900) ? 1 : 0;
        highLevelStatus = (high_filtered < 900) ? 1 : 0;
        
        if (highLevelStatus) { 
            if (pumpState == 0) {
                uint32_t rapidSum = 0;
                for(uint8_t i = 0; i < 10; i++) {
                    rapidSum += read_adc(1); __delay_ms(1);           
                }
                lastHatod = (uint16_t)(rapidSum / 10);        
                SSR_out = 1; pumpState = 1; 
            }
        }
        else if (!lowLevelStatus && !highLevelStatus) { 
            SSR_out = 0; 
            if (pumpState == 1) { 
                if (lowSampleCount > 0) lastLatod = (uint16_t)(lowSum / lowSampleCount);
                lastOnTime = currentOnTime;
                currentOnTime = 0;
                wasOn = 0;
                                
                save_to_buffer(lastHatod, lastLatod, hoursSincePowerup, lastOnTime, lastOffTime);

                lowSum = 0; lowSampleCount = 0; 
                if ((lastOnTime + lastOffTime) > 0) {
                    duty = (uint16_t)((100UL * lastOnTime) / (lastOnTime + lastOffTime));
                }
            }
            pumpState = 0; 
        }

        if (triggerSecondCount) {
            triggerSecondCount = 0;
            timeToDisplay = 1;
            secondsSincePowerup++;
            if (secondsSincePowerup == 10 && !initialSendDone) {
                if (lowSampleCount > 0) lastLatod = (uint16_t)(lowSum / lowSampleCount);
                if (highSampleCount > 0) lastHatod = (uint16_t)(highSum / highSampleCount);
                
                save_to_buffer(lastHatod, lastLatod, hoursSincePowerup, 0, 0);
                
                initialSendDone = 1;
                espFails=0;
            }
            if (pumpState == 1) {
                if (backlightState == 0) { 
                    char cmd[2] = {14, '\0'};
                    put_to_disp_buf(cmd); 
                    backlightState = 1; 
                }
                backlightTime = 0;
                highSum += high_val; highSampleCount++;
                currentOnTime++; wasOn = 1;
                tenMinuteCounter++;
                if (wasOff) { lastOffTime = (currentOffTime == 0) ? 1 : currentOffTime; currentOffTime = 0; wasOff = 0; }
                if (tenMinuteCounter >= 600) {
                    tenMinuteFlag = 1;
                    if (highSampleCount > 0) lastHatod = (uint16_t)(highSum / highSampleCount);
                    save_to_buffer(lastHatod, 0, hoursSincePowerup, tenMinuteCounter, lastOffTime);
                    tenMinuteCounter = 0;
                }
            } else {
                if (backlightTime < 31) {
                    backlightTime++;
                    if (backlightTime > 30 && backlightState == 1) {
                        char cmd[2] = {15, '\0'};
                        put_to_disp_buf(cmd);
                        backlightState = 0;
                    }
                }
                lowSum += low_val; lowSampleCount++;
                currentOffTime++; wasOff = 1;
                tenMinuteCounter = 0; tenMinuteFlag = 0;
            }
            if (secondsSincePowerup % 3600 == 0) hoursSincePowerup++;
        }

        if (timeToDisplay) {
            char line1[21], line2[21], line3[21], line4[21];
            sprintf(line1, "L:%04u H:%04u %s %02d", low_val, high_val, (pumpState) ? "ON " : "OFF", duty);
            sprintf(line2, "On:%04u Off:%04u %s", (pumpState) ? currentOnTime : lastOnTime, (pumpState) ? lastOffTime : currentOffTime, error_msg);
            sprintf(line3, "Hrs: %05u espX:%04u", hoursSincePowerup, espFails);
            sprintf(line4, "Pending: %u/%u      ", records_pending, MAX_RECORDS);
            
            updateDisplayCoord(1, 1, line1);
            updateDisplayCoord(2, 1, line2);
            updateDisplayCoord(3, 1, line3);
            if (pumpState == 1) updateDisplayCoord(4, 1, "Pumping Cycle...    ");
            else updateDisplayCoord(4, 1, line4);
            timeToDisplay = 0;
        }
        process_esp_state_machine();
    }
}


int8 check_inputs();
int8 get_new_mode(int8);
void get_packet_from_NVRAM();
void startup();


/////////////////////////////////////////////////////////////////////////////////
//****************************     Constants     ********************************
/////////////////////////////////////////////////////////////////////////////////
const int8 display_only_mode                  =   0;     //  just a constant 'state of program' type number
const int8 setpoint_mode                      =   1;     //  just a constant 'state of program' type number
const int8 manual_output_mode                 =   3;     //  just a constant 'state of program' type number
const int8 control_mode                       =   4;     //  just a constant 'state of program' type number

//**************************** EEPROM Locations     ********************************
const int16 stored_mode_location                =23;
const int16 first_ever_run_location             =27;
const int16 heat_setpoint_location              =28;
const int16 cool_setpoint_location              =29;

const int8 heating                            =   0;     //  just a constant 'heating or cooling status' type number
const int8 cooling                            =   1;     //  just a constant 'heating or cooling status' type number
const int8 nothing                            =   2;     //  just a constant 'heating or cooling status' type number
const int8 blowing                            =   3;     //  just a constant 'heating or cooling status' type number
const int8 closing                            =   4;     //  just a constant 'heating or cooling status' type number

const char heat_string[5][8]={"Heating","Cooling","Nothing","Blowing","Closing"};

/////////////////////////////////////////////////////////////////////////////////
//****************************     Variables     ********************************
/////////////////////////////////////////////////////////////////////////////////
int1 time_to_measure_sunlight ;                                        
int1 time_to_send_all_data;
int1 time_to_display_flag;                            
int1 time_to_control;                            
int1 light_on;

int8 heat_mode;                                  
int8 mode;                                            // which mode is this program in eg control mode, manual mode, change time
int8 key;                                             // holds keypad value
int8 one_second_counter;                              // timer interrupt counter
int8 heat_setpoint;
int8 cool_setpoint;
int8 difference;

int8 temp;                          // temporary temperature
int8 tempIn;                  // main temperature
int8 tempOut;                  // auxilliary temperature
int8 tempBox;                  // outdoor temperature 

signed int16 heat_error;
signed int16 cool_error;

int8 light_value;

char line1[22];      // holds display characters
char line2[22];
char line3[22];
char line4[22];
char time_string[6];                         // fill 5 characters with string
char date_string[6];
char light_string[4];

int16 Light_morning_start;
int16 Light_evening_stop;

/////////////////////////////////////////////////////////////////////////////////
//****************************    INTERRUPTS     ********************************
/////////////////////////////////////////////////////////////////////////////////

#INT_TIMER1
void  TIMER1_isr(void) 
{
   one_second_counter++;

}

/////////////////////////////////////////////////////////////////////////////////
//****************************     MAIN          ********************************
/////////////////////////////////////////////////////////////////////////////////
void main()
{
   
   
/////////////////////////////////////////////////////////////////////////////////
//****************************     initialize    ********************************
/////////////////////////////////////////////////////////////////////////////////

   temp                                      =0;                          // temporary temperature
   onboard_temp                              =0;                  // main temperature
   extra_temp_1                              =0;                  // auxilliary temperature
   extra_temp_2                              =0;                  // outdoor temperature 
   time_to_measure_sunlight                  =0 ;
   time_to_display_flag                      =0 ;
   time_to_control                           =0 ;
   startup(); 
   
///////////////////////////////////////////////////////////
//****************************     Main Loop    ********************************
/////////////////////////////////////////////////////////////////////////////////
loop:
   enable_interrupts(INT_TIMER1);
   enable_interrupts(GLOBAL);
   restart_wdt();                   // reset countdown to hardware reset
  
   if(light_on)
   {
      light_string="ON ";
      output_high(light_SSR);
   }
   else
   {
      light_string="OFF";
      output_low(light_SSR);
   }
   
   switch(heat_mode)
   {
      case heating:
         output_low(cool_SSR);
         output_low(cool_not);
         output_high(heat_SSR);
         break;
      case nothing:
         output_low(cool_SSR);
         output_low(cool_not);
         output_low(heat_SSR);
         break;
      case closing:
         output_high(cool_SSR);
         output_low(cool_not);
         output_LOW(heat_SSR);
         break;
      case cooling:
         output_high(cool_SSR);
         output_low(cool_not);
         output_low(heat_SSR);
         break;
      case blowing:
         output_high(cool_SSR);
         output_float(cool_not);
         output_low(heat_SSR);
         break;
          
      default:
         break;
   }
   
   if(one_second_counter>=10)       // set lots of flags and variables every second
   {
      time_to_display_flag=1;
      time_to_control=1;
      time_to_measure_sunlight=1;

      one_second_counter=0;
   }
   
   key=check_inputs();                  // get user inputs from keypad
 
   if(key==12) mode=get_new_mode(mode);      
   
   if(key!=13)
   {
      switch(mode) //  depending on mode, choose mode or enter value in a string
      {
         case control_mode:// controlling greenhouse in auto
            switch(key)
            {
               case 10:
                  if(recording_on_flag) recording_on_flag =0;
                  else recording_on_flag=1;
                  write_eeprom(recording_on_location,recording_on_flag);
                  break;
               default:
                  break;
            }
            break;
         case display_only_mode:// display info only
            switch(key)
            {
               case 10:
                  if(recording_on_flag) recording_on_flag =0;
                  else recording_on_flag=1;
                  write_eeprom(recording_on_location,recording_on_flag);
                  break;
               default:
                  break;
            }            
            break;
         case manual_output_mode:
            time_to_control=0;
            switch(key)
            {
               case 1://heat
                  if(heat_mode==heating) heat_mode=nothing;
                  else heat_mode=heating;
                  break;
               case 2://cool fan and window down
                  if(heat_mode==cooling) heat_mode=nothing;
                  else heat_mode=cooling;
                  break;
               case 3://light
                  if(light_on)
                     light_on=0;
                  else
                     light_on=1;
                  break;
               case 4://blow fan only
                  if(heat_mode==blowing) heat_mode=nothing;
                  else heat_mode=blowing;
                  break;
 
               case 10:
                  if(recording_on_flag) recording_on_flag =0;
                  else recording_on_flag=1;
                  write_eeprom(recording_on_location,recording_on_flag);
                  break;               
               default:
                  break;
            }
            break;
         case historical_data_mode:
            switch(key)
            {
               case 1:  // subtract one from pointer
                  if(temporary_data_pointer>=number_of_bytes_in_a_record)
                  {
                     temporary_data_pointer=temporary_data_pointer-(number_of_bytes_in_a_record);
                     get_packet_from_NVRAM();
                  }
                  break;
               case 3: // add one to pointer
                  if(temporary_data_pointer<make16(read_eeprom(stored_index_location_high),read_eeprom(stored_index_location_low)))
                  {
                     temporary_data_pointer=temporary_data_pointer+(number_of_bytes_in_a_record);
                     get_packet_from_NVRAM();
                  }
                  break;
               case 0://  get ready to erase data
                  confirm_erase=1;
                  break;
               case 8://  confirm erase
                  if(confirm_erase)
                  {
                     temporary_data_pointer=0;
                     write_eeprom(stored_index_location_high,0);
                     write_eeprom(stored_index_location_low,0);
                     get_packet_from_NVRAM();
                     confirm_erase=0;
                  }
                  break;
               default:
                  break;
            }        
            break;
         case transfer_data_mode:
            switch(key)
            {
               default:
                  break;
            }        
            break;
         case setpoint_mode:
            switch(key)
            {
               case 0:
                  write_eeprom(heat_setpoint_location,heat_setpoint);
                  write_eeprom(cool_setpoint_location,cool_setpoint);
                  mode=read_eeprom(stored_mode_location);
                  break;
               case 1:  // subtract one from heat
               if(heat_setpoint>0) heat_setpoint--;
                  break;
               case 3: // add one to heat
               if(heat_setpoint<(cool_setpoint-5)) heat_setpoint++;
                  break;
               case 4:  // subtract one cool
               if(cool_setpoint>(heat_setpoint+5)) cool_setpoint--;
                  break;
               case 6: // add one to cool
               if(cool_setpoint<110) cool_setpoint++;
                  break;
               default:
                  break;
            }        
            break;
         case program_time_mode:
            switch(key)
            {
               case 1:
                  if(hour>0) hour--;
                  break;
               case 2:
                  if(hour<23) hour++;
                  break;
               case 4:
                  if(minute>0) minute--;
                  break;
               case 5:
                  if(minute<59) minute++;
                  break;
               case 7:
                  if(month>0) month--;
                  break;
               case 8:
                  if(month<11) month++;
                  break;
               case 6:
                  if(day>0) day--;
                  break;
               case 3:
                  if(day<31) day++;
                  break;
               case 9:
                  if(year>=72)
                     year=22;
                  else
                     year++;
                  break;
               case 0:
                  minute = bin2bcd(minute);
                  hour =  bin2bcd(hour);
                  day =  bin2bcd(day);
                  month =  bin2bcd(month);
                  year =  bin2bcd(year);
                  set_time();           // set time in binary coded decimal

               mode=read_eeprom(stored_mode_location);
                  break;
               default:
                  break;
            }         
            break;
         default:
            break;
      }
      do          // wait for key release
      {
         check_inputs();
         restart_wdt();
      }
      while(key!=13);
  }
 

   if(adc_done())  
   {
      light_value=read_adc(ADC_READ_ONLY);// analog input
      read_adc(ADC_START_ONLY);
      scratchpad=light_value;
      scratchpad=scratchpad*100;
      scratchpad=scratchpad/256;
      light_value=scratchpad;
   }  
   

   
   if(time_to_control && mode!=manual_output_mode && mode!=display_only_mode)
{
   heat_mode=nothing;
   difference=(cool_setpoint-heat_setpoint)/2; // centre of difference between setpoints(negative number)
   
   if(onboard_temp<heat_setpoint) heat_mode=heating;   // heat when window closed and not blowing and air is less than heat setpoint 
   if(onboard_temp<heat_setpoint+2 && onboard_temp>heat_setpoint) heat_mode=nothing;       // do nothing from 1 to 2 degrees
   if(onboard_temp<=heat_setpoint+difference-1 && onboard_temp>heat_setpoint+2) heat_mode=closing;// close window if between 2 degrees and difference away from heating
   if(onboard_temp<=cool_setpoint && onboard_temp>heat_setpoint+difference) heat_mode=cooling; //  open window when halfway between cool and heat
   if(onboard_temp>cool_setpoint) heat_mode=blowing;// additional cooling when temperature is over cool setpoint

   
   if(onboard_temp>=120) heat_mode=nothing;
   
   light_on=0;
   light_string="OFF";
   if(hour>=Light_morning_start && hour<=Light_evening_stop && light_value<50) light_on=1;
//control everything
}
   goto loop;

}

/////////////////////////////////////////////////////////////////////////////////
//****************************     Functions    ********************************
/////////////////////////////////////////////////////////////////////////////////
void time_to_display()
{
   sprintf(line4,"%s L%u %03u %s",heat_string[heat_mode],light_on,light_value,time_string);
   sprintf(line3," Aux:%03u.%02lu %01u  %s",extra_temp_1,extra_temp_1_eighth_value,recording_on_flag,date_string);
   switch(mode)
   {   
   case manual_output_mode:
      sprintf(line1,"Temp:%03u.%02lu 1-H 2-C ",onboard_temp,onboard_temp_eighth_value);
      sprintf(line2," Out:%03u.%02lu 3-L 4-B ",extra_temp_2,extra_temp_2_eighth_value);
      break;      
   case control_mode:// controlling greenhouse in auto
      sprintf(line1,"Temp:%03u.%01lu H:%03u    ",onboard_temp,onboard_temp_eighth_value,heat_setpoint);
      sprintf(line2," Out:%03u.%01lu C:%03u %03lu",extra_temp_2,extra_temp_2_eighth_value,cool_setpoint,store_data_counter);
      break;
   case display_only_mode:// display info only
      sprintf(line1,"Temp:%03u.%02lu CONTROL ",onboard_temp,onboard_temp_eighth_value);
      sprintf(line2," Out:%03u.%02lu OFF     ",extra_temp_2,extra_temp_2_eighth_value);
       break;
   case historical_data_mode:// display saved memory
      sprintf(line1,"P%03lu E%03lu -1-  +3+",temporary_data_pointer,make16(read_eeprom(stored_index_location_high),read_eeprom(stored_index_location_low)));
      sprintf(line2,"T:%03u O:%03u A:%03u ",stored_temp,stored_extra_temp_1,stored_extra_temp_2);
      if(!confirm_erase) sprintf(line3,"%s %s 0-Erase ",stored_date_string,stored_time_string);
      else sprintf(line3,"%s %s 8Confirm",stored_date_string,stored_time_string);
      sprintf(line4,"%s L:%u %03u ",heat_string[stored_heat_mode],stored_light_on,stored_light_value);
     time_to_display_flag=1;
      break;
   case setpoint_mode:// display setpoints
      sprintf(line1,"Heat-%03u -1- +3+ ",heat_setpoint);
      sprintf(line2,"cool-%03u -4- +6+ ",cool_setpoint);
      line3=" Done 0 ";
      line4="";
      break;
   case program_time_mode: // sync the clock
      sprintf(line1,"  %02u:%02u %s%02u 20%u  ",hour,minute,month_name[month],day,year);
      sprintf(line2,"-1-Hour+2+ -4-Min+5+ ");
      sprintf(line3,"-7-Mon+8+  -3-Day+6+ ");
      sprintf(line4,"0-Save 9+Year #-Exit");
      time_to_display_flag=1;
      break;
   default:
      break;
   }
}

int8 check_inputs()
{
   const int8 key_aquire_delay      =   15;//ms

   key=13;
   output_low(column1);
   delay_ms(key_aquire_delay);
   if(!input(row1))   Key=1;
   if(!input(row2))   key=4;
   if(!input(row3))   key=7;
   if(!input(row4))   key=10;//*
   output_float(column1);
   output_low(column2);
   delay_ms(key_aquire_delay);
   if(!input(row1))   Key=2;
   if(!input(row2))   key=5;
   if(!input(row3))   key=8;
   if(!input(row4))   key=0;
   output_float(column2);
   output_low(column3);
   delay_ms(key_aquire_delay);
   if(!input(row1))   Key=3;
   if(!input(row2))   key=6;
   if(!input(row3))   key=9;
   if(!input(row4))   key=12;//#
   output_float(column3);

   return key;
}

int8 get_new_mode(int8 original_mode)
{
   confirm_erase                              =0;// when leaving historical data mode without finishing
   Disable_interrupts(global);
   do
      check_inputs(); 
   while(key!=13);
   
   strcpy(line1, "1-Setpoints 2-Time  ");
   strcpy(line2, "3-Manual   4-Control");
   strcpy(line3, "5-Xfer     6-History");
   strcpy(line4, "0-Display  *-Back   ");
   display4lines();
   key=13;
   do
      check_inputs();
   while   (key==13);
   if(key>=0 && key<=6)
   {
      mode = key;
      if (mode==display_only_mode | mode==control_mode | mode==manual_output_mode) write_eeprom(stored_mode_location,mode);
   }
   else mode=original_mode;
   if(key==10)
   {
      enable_interrupts(global);
      return mode;
   }
   do // wait for key release
      check_inputs();
   while(key!=13);
   switch(mode)
   {     
   case transfer_data_mode:
      strcpy(line1, "Transfer data mode  ");
      time_to_send_all_data=1;
      break;
   case setpoint_mode:
      strcpy(line1, " Setpoint mode      ");
      
      break;
   case historical_data_mode:
      strcpy(line1, "View Data Mode      ");
      temporary_data_pointer=0;
      get_packet_from_NVRAM();
      time_to_display_flag=1;
      break;
   case program_time_mode:
      strcpy(line1, "Program Time Mode   ");
      get_time(); // receive time in binary with a time_string and a date_string
      time_to_display_flag=1;
      break;
    case manual_output_mode:
      strcpy(line1, " Manual Mode        ");
      break;
    case display_only_mode:
      strcpy(line1, "Display Only Mode   ");
      break;
    case control_mode:
      strcpy(line1, " Control Mode       ");
      break;
   }
   sprintf(line2,line1);
   sprintf(line3,line1);
   sprintf(line4,line1);
   
   display4lines();
   delay_ms(1000);
   key=13;
   enable_interrupts(global);
   return mode;
}

void startup()
{
   
   if(read_eeprom(first_ever_run_location)!=69)
   {
   //////////////////////////////////////////////////////////////////////////
   /////  First ever powerup ////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////
      heat_setpoint =15;
      cool_setpoint =40;
      write_eeprom(cool_setpoint_location,cool_setpoint);
      write_eeprom(heat_setpoint_location,heat_setpoint);
      mode = display_only_mode; // which mode is this program in eg control mode, manual mode, change time stored in NVRAM in case of power fail
      write_eeprom(stored_mode_location,display_only_mode);
      store_data_counter=0;
      tempIn=222;
      tempOut=222;
      tempBox=222;
      heat_mode=nothing;
      light_value=100;
      light_on=1;
   }
   else
   {
   //////////////////////////////////////////////////////////////////////////
   /////  Regular powerup ///////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////
   
      heat_setpoint =read_eeprom(heat_setpoint_location);
      cool_setpoint =read_eeprom(cool_setpoint_location);
      recording_on_flag= read_eeprom(recording_on_location);
      mode =read_eeprom(stored_mode_location); // which mode is this program in eg control mode, manual mode, change time stored in NVRAM in case of power fail
   }

   key                                       =13 ;  // holds keypad value
   one_second_counter =0                        ;   // timer interrupt counter
   sprintf(line1," POWERUP ");
   display4lines();
   delay_ms(1000);
   setup_wdt(WDT_2304MS);      //~? s reset

   setup_adc_ports(AN0);
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_256|RTCC_8_bit);      //13.1 ms overflow
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);      //104 ms overflow
   setup_timer_2(T2_DIV_BY_16,255,16);      //819 us overflow, 13.1 ms interrupt
   setup_adc_ports(AN0);

   setup_adc(ADC_CLOCK_internal);
   set_adc_channel(0);
   
   read_adc(ADC_START_ONLY);                                      // begin A/D conversion

   output_float(cool_SSR);
   output_float(cool_not);
   
}


void get_packet_from_NVRAM()
{
   disable_interrupts(global);
   int8 local_data[number_of_bytes_in_a_record];
   int8 local_i;

   for(local_i=0;local_i<=number_of_bytes_in_a_record,local_i++;)
   {
      i2c_start();                  // initiate i2c
      I2c_write(0xA0);              // Start WRITE condition for address
      i2c_write(make8(temporary_data_pointer,1)); // set address to pointer high byte
      i2c_write(make8(temporary_data_pointer,0));  // set address to pointer low byte
      i2c_start();                  // initiate
      I2c_write(0xa1);              // Start a READ condition
      local_data[local_i]=i2c_read();
   }
   i2c_stop(); 
   delay_ms(20);
 
   stored_mode = local_data[0];
   stored_temp  = local_data[1];
   stored_extra_temp_1  = local_data[2];
   stored_extra_temp_2  =  local_data[3];
   stored_hour =  local_data[4];
   stored_minute =  local_data[5];
   stored_day = local_data[6];
   stored_month =  local_data[7];
   stored_year =  local_data[8];
   stored_heat_mode =  local_data[9];
   stored_light_value = local_data[10];
   stored_light_on =  local_data[11];
 
   // in order to make the string I used binary coded decimal(easier in hex)
   if(stored_heat_mode>3) stored_heat_mode=3;
   if(stored_month>11) stored_month=11;
   sprintf(stored_time_string,"%02x:%02x",bin2bcd(stored_hour),bin2bcd(stored_minute));
   sprintf(stored_date_string,"%s%02x",month_name[stored_month],bin2bcd(stored_day));
   return;
}

