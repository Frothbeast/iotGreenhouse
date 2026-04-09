// This program is for my greenhouse control
// Created February 2022 Dan Jubenville in ccsc
// now porting to MPLAB XC8



// Add these missing variables
char line1[21], line2[21], line3[21], line4[21];

// Define your server details
#define SERVER_IP "192.168.1.100"
#define SERVER_PORT "8080"

// Mode Enums
enum { display_only_mode, setpoint_mode, manual_output_mode, control_mode, pid_tuning_mode = 7 };

// Fix the function pointer
#define run_esp_handler process_esp_state_machine


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
#define light_SSR          LATCbits.LATC0          // solid state relay for grow light
#define TRISlight          TRISC0                  // solid state relay for grow light
#define fanSSR             LATCbits.LATC1          // solid state relay for fan
#define TRISfan            TRISC1                  // solid state relay for fan
#define coolSSR            LATCbits.LATC2          // solid state relay for cooling
#define TRIScool           TRISC2                  // solid state relay for cooling
#define heatSSR            LATCbits.LATC3          // solid state relay for heat  
#define TRISheat           TRISC3                  // solid state relay for heat

#define Display_pin        LATCbits.LATC4  
#define TRIS_Display       TRISC4          

#define sunlight           PORTAbits.RA0 
#define TRIS_sunlight      TRISA0         // photresistor 5V on one side and this pin with 10k pulldown on other side
#define thermistor1        PORTAbits.RA1 
#define TRIS_thermistor1   TRISA1         
#define thermistor2        PORTAbits.RA2 
#define TRIS_thermistor2   TRISA2         
#define thermistor3        PORTAbits.RA3 
#define TRIS_thermistor3   TRISA3

#define column1            PORTBbits.RB4 // input columns for keypad
#define TRIS_column1       TRISB4        // 1 2 3 
#define column2            PORTBbits.RB5 // 4 5 6
#define TRIS_column2       TRISB5        // 7 8 9     
#define column3            PORTBbits.RB6 // * 0 #
#define TRIS_column3       TRISB6                                                 
#define row1               LATBbits.LATB0// output rows for keypad
#define TRIS_row1          TRISB0          
#define row2               LATBbits.LATB1// output rows for keypad
#define TRIS_row2          TRISB1         
#define row3               LATBbits.LATB2// output rows for keypad
#define TRIS_row3          TRISB2         
#define row4               LATBbits.LATB3// output rows for keypad
#define TRIS_row4          TRISB3

// Circular Buffer for Display
#define DISP_BUF_SIZE 255
volatile char disp_buffer[DISP_BUF_SIZE];
volatile uint8_t disp_head = 0;
volatile uint8_t disp_tail = 0;

// Timing variables for Seetron serial backpack
uint16_t displayDelayCounter = 0; 
volatile uint16_t debounce_timer = 0;
volatile uint8_t keypad_pending = 0;
// Application Variables
volatile char error_msg[5] = " OK";
uint8_t triggerSecondCount = 0;
uint8_t timeToDisplay = 0;
uint32_t secondsSincePowerup = 0;
uint16_t hoursSincePowerup = 0, secondsCounter = 0;
uint16_t espFails;
uint8_t pumpState = 0; 
uint8_t initialSendDone = 0;
uint16_t tenMinuteCounter = 0;
uint8_t tenMinuteFlag = 0;
uint16_t backlightTime = 0;
uint8_t backlightState = 1;

uint8_t keypad_active = 0;    // Flag: 1 means we are currently debouncing
uint8_t last_key = 13;        // Stores the key found during the first hit

float heat_duty = 0, cool_duty = 0;
uint8_t suspend_heat_active = 1;

uint8_t MODcoldPID = 1;

// Filtered results
uint16_t sunlight_filtered = 500;
uint16_t inside_temp_filtered = 500;
uint16_t outside_temp_filtered = 500;
uint16_t box_temp_filtered = 500;
uint8_t key = 13;
uint8_t current_mode = 0;
uint16_t updateDisplayTimer = 0;
uint8_t pwm_timer = 0;
int16_t heat_output = 0, cool_output = 0;
uint8_t heat_setpoint = 20, cool_setpoint = 28;
uint8_t light_start = 6, light_stop = 20;
uint16_t sunlight_threshold = 400;

#define MANUAL_MODE 3 // Match your case statement
#define MAX_RECORDS 10 // Define the buffer size
uint8_t records_pending = 0;
uint8_t buf_head = 0, buf_tail = 0;
// Filter constants (Shift 3 = roughly 1/8th weight to new samples)
#define FILTER_SHIFT 3

//PID struct
typedef struct {
    int16_t Kp, Ki, Kd; 
    int32_t integral;  
    int16_t prev_error;
} pid_params_int_t;

pid_params_int_t cool_pid;
pid_params_int_t head_pid;

//EEPROM locations
#define EE_HEAT_KP    40  // 40-43
#define EE_HEAT_KI    44  // 44-47
#define EE_HEAT_KD    48  // 48-51
#define EE_COOL_KP    52  // 52-55
#define EE_COOL_KI    56  // 56-59
#define EE_COOL_KD    60  // 60-63

uint16_t low_val = 0, high_val = 0;
uint16_t low_filtered = 1000, high_filtered = 1000;
uint32_t lowSum = 0, highSum = 0;
uint16_t lastLatod = 0, lastHatod = 0;

typedef struct {
    uint16_t inside_temp;   // thermistor1 (RA1)
    uint16_t outside_temp;  // thermistor2 (RA2)
    uint16_t box_temp;      // thermistor3 (RA3)
    uint16_t light_level;   // sunlight (RA0)
    uint8_t  ssr_states;    // Bitmask: bit0=Heat, bit1=Cool, bit2=Fan, bit3=Light
} greenhouse_record_t;

greenhouse_record_t record_buffer[MAX_RECORDS];

void save_to_buffer(uint16_t h, uint16_t l, uint16_t hrs, uint16_t ont, uint16_t offt) {
    if (records_pending < MAX_RECORDS) {
        record_buffer[buf_head].inside_temp = inside_temp;
        record_buffer[buf_head].outside_temp = outside_temp;
        record_buffer[buf_head].box_temp = box_temp;
        record_buffer[buf_head].light_level = light_level;
        record_buffer[buf_head].ssr_states = ssr_states;
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
void load_pid_settings(void);
void eeprom_write_float(uint16_t addr, float value);
float eeprom_read_float(uint16_t addr);
void calculate_control_logic(int8_t current_temp);
void put_to_disp_buf(const char* str);
void process_display_buffer(void);
int8_t updateDisplayCoord(uint8_t line, uint8_t column, const char* str);
void software_putch(char data); 
void uart_send_string(const char* s);
void process_esp_state_machine(void);
uint16_t read_adc(uint8_t channel);
int16_t calculate_pid_int(pid_params_int_t *p, int16_t setpoint, int16_t current_val);
void update_all_filters(void);
void time_to_display(void);

void time_to_display(void) {
    switch(mode) {
        case control_mode: // Mode 4
            sprintf(line1, "In:%03u  Out:%03u    ", inside_temp_filtered, outside_temp_filtered);
            sprintf(line2, "H-Set:%02u C-Set:%02u  ", heat_setpoint, cool_setpoint);
            sprintf(line3, "Sun:%03u%% Light:%s  ", (uint32_t)sunlight_filtered * 100 / 1023, (light_SSR) ? "ON " : "OFF");
            
            if (cool_output > 50)      sprintf(line4, "Status: Cooling+Fan ");
            else if (cool_output > 0)  sprintf(line4, "Status: Cooling     ");
            else if (heat_output > 0)  sprintf(line4, "Status: Heating     ");
            else                       sprintf(line4, "Status: Balanced    ");
            break;

        case manual_output_mode: // Mode 3
            sprintf(line1, "--- MANUAL MODE --- ");
            sprintf(line2, "1:Heat[%c] 2:Cool[%c]", (heat_SSR)?'+':'-', (cool_SSR)?'+':'-');
            sprintf(line3, "3:Fan [%c] 4:Ligh[%c]", (fanSSR)?'+':'-', (light_SSR)?'+':'-');
            sprintf(line4, "Press # to Exit     ");
            break;

        case setpoint_mode: // Mode 1
            sprintf(line1, "--- SETPOINTS ---   ");
            sprintf(line2, "Heat Target: %03u    ", heat_setpoint);
            sprintf(line3, "Cool Target: %03u    ", cool_setpoint);
            sprintf(line4, "1/3:Heat  4/6:Cool  ");
            break;

        case display_only_mode: // Mode 0
            sprintf(line1, "In:%03u  Out:%03u    ", inside_temp_filtered, outside_temp_filtered);
            sprintf(line2, "Sunlight: %03u%%     ", (uint32_t)sunlight_filtered * 100 / 1023);
            sprintf(line3, "--- MONITORING ---  ");
            sprintf(line4, "OUTPUTS DISABLED    ");
            break;

        case pid_tuning_mode: // Mode 7
            if (MODcoldPID) {
                sprintf(line1, "COOL PID (Kp): %03d  ", cool_pid.Kp);
                sprintf(line2, "COOL PID (Ki): %03d  ", cool_pid.Ki);
                sprintf(line3, "COOL PID (Kd): %03d  ", cool_pid.Kd);
            } else {
                sprintf(line1, "HEAT PID (Kp): %03d  ", heat_pid.Kp);
                sprintf(line2, "HEAT PID (Ki): %03d  ", heat_pid.Ki);
                sprintf(line3, "HEAT PID (Kd): %03d  ", heat_pid.Kd);
            }
            sprintf(line4, "*-Toggle H/C 0-Save ");
            break;
    }

    updateDisplayCoord(1, 1, line1);
    updateDisplayCoord(2, 1, line2);
    updateDisplayCoord(3, 1, line3);
    updateDisplayCoord(4, 1, line4);
}

void update_all_filters(void) {
    uint16_t raw;
    raw = read_adc(0);
    sunlight_filtered = (uint16_t)((raw >> FILTER_SHIFT) + (sunlight_filtered - (sunlight_filtered >> FILTER_SHIFT)));
    raw = read_adc(1);
    inside_temp_filtered = (uint16_t)((raw >> FILTER_SHIFT) + (inside_temp_filtered - (inside_temp_filtered >> FILTER_SHIFT)));
    raw = read_adc(2);
    outside_temp_filtered = (uint16_t)((raw >> FILTER_SHIFT) + (outside_temp_filtered - (outside_temp_filtered >> FILTER_SHIFT)));
    raw = read_adc(3);
    box_temp_filtered = (uint16_t)((raw >> FILTER_SHIFT) + (box_temp_filtered - (box_temp_filtered >> FILTER_SHIFT)));
}

int16_t calculate_pid_int(pid_params_int_t *p, int16_t setpoint, int16_t current_val) {
    int16_t error = setpoint - current_val;
    
    p->integral += error;
    
    // Anti-windup: Clamp the integral to prevent it from growing too large
    if (p->integral > 1000) p->integral = 1000;
    if (p->integral < -1000) p->integral = -1000;

    int16_t derivative = error - p->prev_error;
    
    // Result = (P + I + D) / 100
    int32_t output = ((int32_t)p->Kp * error) + 
                     ((int32_t)p->Ki * p->integral) + 
                     ((int32_t)p->Kd * derivative);
    
    p->prev_error = error;
    return (int16_t)(output / 100);
}

void load_pid_settings(void) {
    if (eeprom_read(EE_HEAT_KP) == 0xFF) {
        heat_pid.Kp = 100; heat_pid.Ki = 10; heat_pid.Kd = 5;
        cool_pid.Kp = 100; cool_pid.Ki = 10; cool_pid.Kd = 5;
        save_all_pid_settings();
    } else {
        heat_pid.Kp = eeprom_read_float(EE_HEAT_KP);
        heat_pid.Ki = eeprom_read_float(EE_HEAT_KI);
        heat_pid.Kd = eeprom_read_float(EE_HEAT_KD);
        cool_pid.Kp = eeprom_read_float(EE_COOL_KP);
        cool_pid.Ki = eeprom_read_float(EE_COOL_KI);
        cool_pid.Kd = eeprom_read_float(EE_COOL_KD);
    }
}

void eeprom_write_float(uint16_t addr, float value) {
    uint8_t *ptr = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++) {
        eeprom_write(addr + i, ptr[i]);
    }
}

float eeprom_read_float(uint16_t addr) {
    float value;
    uint8_t *ptr = (uint8_t *)&value;
    for (uint8_t i = 0; i < 4; i++) {
        ptr[i] = eeprom_read(addr + i);
    }
    return value;
}

void calculate_control_logic(int8_t current_temp) {
    if (suspend_heat_active && (cool_duty > 0 || coolSSR == 1)) {heat_duty = 0;heatSSR = 0; }
    // PID calculations would update heat_duty and cool_duty here
}

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

    if (INTCONbits.RBIF) {
        uint8_t dummy = PORTB;    // Required to end the mismatch
        INTCONbits.RBIE = 0;      // STOP further interrupts immediately
        debounce_timer = 20;      // Start the 20ms countdown in TMR0 logic
        keypad_active = 1;        // Signal that a press is being processed
        INTCONbits.RBIF = 0;      // Clear flag
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

uint8_t check_inputs(void) {
    INTCONbits.RBIE = 0;
    uint8_t found_key = 13; 
    LATBbits.LATB0 = 0; LATBbits.LATB1 = 1; LATBbits.LATB2 = 1; LATBbits.LATB3 = 1;
    __delay_us(50);
    if (!PORTBbits.RB4)      found_key = 1;
    else if (!PORTBbits.RB5) found_key = 2;
    else if (!PORTBbits.RB6) found_key = 3;
    if (found_key != 13) goto finalize_scan;
    LATBbits.LATB0 = 1; LATBbits.LATB1 = 0; LATBbits.LATB2 = 1; LATBbits.LATB3 = 1;
    __delay_us(50);
    if (!PORTBbits.RB4)      found_key = 4;
    else if (!PORTBbits.RB5) found_key = 5;
    else if (!PORTBbits.RB6) found_key = 6;
    if (found_key != 13) goto finalize_scan;
    LATBbits.LATB0 = 1; LATBbits.LATB1 = 1; LATBbits.LATB2 = 0; LATBbits.LATB3 = 1;
    __delay_us(50);
    if (!PORTBbits.RB4)      found_key = 7;
    else if (!PORTBbits.RB5) found_key = 8;
    else if (!PORTBbits.RB6) found_key = 9;
    if (found_key != 13) goto finalize_scan;
    LATBbits.LATB0 = 1; LATBbits.LATB1 = 1; LATBbits.LATB2 = 1; LATBbits.LATB3 = 0;
    __delay_us(50);
    if (!PORTBbits.RB4)      found_key = 10; // *
    else if (!PORTBbits.RB5) found_key = 0;
    else if (!PORTBbits.RB6) found_key = 12; // #

  finalize_scan:
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    LATBbits.LATB2 = 0;
    LATBbits.LATB3 = 0;
    uint8_t dummy = PORTB;
    INTCONbits.RBIF = 0;
    INTCONbits.RBIE = 1;

    return found_key;
}

void main(void) {
    INTCON = 0x00;           // Disable all interrupts during setup
    PIE1 = 0x00;             // Disable peripheral interrupts
    ADCON1 = 0x0B;           
    ADCON2 = 0x92;           // Right justified, 4 TAD, Fosc/32
    TRISA = 0x0F;            
    CMCON = 0x07;            // Comparators OFF
    CVRCON = 0x00;           // Voltage Reference OFF
    TRISB = 0x70;            // 0111 0000 (RB4,5,6 as inputs)
    INTCONbits.RBIE = 1;     // Enable Port B change interrupt
    TRISlight = 0;           // RC0 Output
    TRISfan   = 0;           // RC1 Output
    TRIScool  = 0;           // RC2 Output
    TRISheat  = 0;           // RC3 Output
    TRIS_Display = 0;        // RC4 Output  
    TRISC6 = 0;              // TX Output
    TRISC7 = 1;              // RX Input
    Display_Pin = 0;         // Start display pin low
    T0CON = 0xC0;            // 8-bit, 1:2 prescaler (for 20MHz)    

    __delay_ms(100);
    software_putch(12); __delay_ms(100); software_putch(14);
    
    updateDisplayCoord(1, 1, "Greenhouse Control");
    updateDisplayCoord(2, 2, "Dan Jubenville");
    updateDisplayCoord(3, 3, "January 2026");
    
    while(disp_head != disp_tail) { process_display_buffer(); }

    SPBRG = 10; TXSTA = 0x24; RCSTA = 0x90; 
    
    volatile char dummy;
    dummy = RCREG; dummy = RCREG;
    RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;          

    PIE1bits.RCIE = 1; INTCONbits.TMR0IE = 1; INTCONbits.PEIE = 1; INTCONbits.GIE = 1;    
    SENSOR_PWR = 1;

    while(1) {
        CLRWDT(); 
        update_all_filters();
        if (keypad_active && debounce_timer == 0) {
            key = check_inputs();
            if (key != 13) {
                if (key == 10) { // '*'
                    if (mode == display_only_mode)    mode = setpoint_mode;
                    else if (mode == setpoint_mode)   mode = manual_output_mode;
                    else if (mode == manual_output_mode) mode = control_mode;
                    else if (mode == control_mode)    mode = pid_tuning_mode;
                    else mode = display_only_mode;
                    
                    software_putch(12); // Clear screen on mode change
                }
                switch(mode) {
                    case setpoint_mode: 
                        if (key == 1) heat_setpoint++;
                        if (key == 3) heat_setpoint--;
                        if (key == 4) cool_setpoint++;
                        if (key == 6) cool_setpoint--;
                        break;

                    case manual_output_mode: 
                        if (key == 1) heat_SSR = !heat_SSR;
                        if (key == 2) cool_SSR = !cool_SSR;
                        if (key == 3) fanSSR = !fanSSR;
                        if (key == 4) light_SSR = !light_SSR;
                        if (key == 12) mode = control_mode; // '#'
                        break;

                    case pid_tuning_mode: 
                        if (key == 12) MODcoldPID = !MODcoldPID; // '#' key
                        if (key == 0) {
                            save_all_pid_settings(); 
                            strcpy((char*)error_msg, "SAVD");
                        }
                        break;

                    case control_mode:
                    case display_only_mode:
                        break;
                }
            
            }
            keypad_active = 0;         
            INTCONbits.RBIF = 0;      
            INTCONbits.RBIE = 1;      
        }

        if (triggerSecondCount) {
            triggerSecondCount = 0;
            secondsSincePowerup++;
            if (secondsSincePowerup % 3600 == 0) hoursSincePowerup++;
            if (secondsSincePowerup == 10 && !initialSendDone) {
            queue_heartbeat_record(); 
            initialSendDone = 1;
            }
            if (updateDisplayTimer == 0) {
            time_to_display();      // Call the function we just built
            updateDisplayTimer = 10; // Reset timer (e.g., wait 100ms before next refresh)
            }
            pwm_timer++;
            if (pwm_timer >= 10) {
            pwm_timer = 0;
            heat_output = calculate_pid_int(&heat_pid, heat_setpoint, inside_temp_filtered);
            cool_output = calculate_pid_int(&cool_pid, cool_setpoint, inside_temp_filtered);
            }
            if (cool_output > 0 && cool_output <= 50) {
                coolSSR = (cool_output * 2 > (pwm_timer * 10)) ? 1 : 0;
                fanSSR = 0; 
            } else if (cool_output > 50) {
                coolSSR = 1;
                fanSSR = ((cool_output - 50) * 2 > (pwm_timer * 10)) ? 1 : 0;
            } else {
                coolSSR = 0;
                fanSSR = 0;
            }
            if (suspend_heat_active && (coolSSR || fanSSR)) {
                heatSSR = 0; 
            } else {
                heatSSR = (heat_output > (pwm_timer * 10)) ? 1 : 0;
            }
            if (hour >= light_start && hour < light_stop && sunlight_filtered < sunlight_threshold) {
                light_SSR = 1;
            } else {
                light_SSR = 0;
            }
            
            tenMinuteCounter++;
            if (tenMinuteCounter >= 600) {
            save_to_buffer(inside_temp_filtered, outside_temp_filtered, hoursSincePowerup, heat_output, cool_output); 
            tenMinuteCounter = 0;
            }
        }

        run_esp_handler(); 
      
    }
}