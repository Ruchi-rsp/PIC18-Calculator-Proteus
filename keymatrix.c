#include <p18f4550.h>

// Configuration Bits
#pragma config FOSC = HSPLL_HS
#pragma config PLLDIV = 5
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config PWRT = ON
#pragma config BOR = ON
#pragma config WDT = OFF
#pragma config PBADEN = OFF
#pragma config LVP = OFF

// Define LCD Pins
#define LCD_EN LATAbits.LATA1
#define LCD_RS LATAbits.LATA0
#define LCDPORT LATB

// Define Keypad Rows & Columns
#define ROWS TRISD
#define COLS LATD
#define KEYPAD PORTD

// Keypad Lookup Table
const unsigned char KeyLookupTbl[4][4] = {
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

// Function Prototypes
void lcd_delay(unsigned int time);
void InitLCD(void);
void SendInstruction(unsigned char command);
void SendData(unsigned char lcddata);
unsigned char ReadKey(void);
void display_number(unsigned int num);

// Delay Function
void lcd_delay(unsigned int time) {
    unsigned int i, j;
    for (i = 0; i < time; i++) {
        for (j = 0; j < 50; j++);
    }
}

// Send Command to LCD
void SendInstruction(unsigned char command) {
    LCD_RS = 0; // Command Mode
    LCDPORT = command;
    LCD_EN = 1;
    lcd_delay(10);
    LCD_EN = 0;
    lcd_delay(10);
}

// Send Data to LCD
void SendData(unsigned char lcddata) {
    LCD_RS = 1; // Data Mode
    LCDPORT = lcddata;
    LCD_EN = 1;
    lcd_delay(10);
    LCD_EN = 0;
    lcd_delay(10);
}

// Initialize LCD
void InitLCD(void) {
    ADCON1 = 0x0F;  // Configure all analog pins as digital
    TRISB = 0x00;   // LCD Data Port as Output
    TRISA = 0x00;   // RS, EN as Output

    SendInstruction(0x38); // 8-bit mode, 2-line, 5x7 font
    SendInstruction(0x06); // Entry mode
    SendInstruction(0x0C); // Display ON, Cursor OFF
    SendInstruction(0x01); // Clear display
    SendInstruction(0x80); // Move cursor to start
}

// Read Key from Keypad
unsigned char ReadKey(void) {
    unsigned char row, col;

    for (col = 0; col < 4; col++) {
        LATD = ~(1 << col); // Set one column LOW at a time
        lcd_delay(5);

        for (row = 0; row < 4; row++) {
            if (!(PORTD & (1 << (row + 4)))) { // Check if row is LOW
                while (!(PORTD & (1 << (row + 4)))); // Wait for key release
                return KeyLookupTbl[row][col]; // Return corresponding key value
            }
        }
    }
    return 0xFF; // No key pressed
}

// Function to Display Numbers on LCD
void display_number(unsigned int num) {
    char buffer[6];
    unsigned char i = 0;

    if (num == 0) {
        SendData('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i > 0) {
        SendData(buffer[--i]);
    }
}

// Main Function
void main() {
    unsigned char Key;
    unsigned int num1 = 0, num2 = 0, result = 0;
    char operation = 0;

    TRISD = 0xF0;   // RD4-RD7 (Rows) as Input, RD0-RD3 (Columns) as Output
    LATD = 0x0F;    // Ensure columns are HIGH
    InitLCD();

    // Keypad Scanning Loop
    while (1) {
        Key = ReadKey();   // Read Key

        if (Key != 0xFF) { // If a valid key is pressed
            if (Key >= '0' && Key <= '9') {
                if (operation == 0) {
                    num1 = (num1 * 10) + (Key - '0'); // Store first number
                } else {
                    num2 = (num2 * 10) + (Key - '0'); // Store second number
                }
                SendData(Key); // Display Key Pressed
            } 
            else if (Key == '+' || Key == '-' || Key == '*' || Key == '/') {
                operation = Key;
                SendData(Key); // Display Operation
            } 
            else if (Key == '=') {
                switch (operation) {
                    case '+': result = num1 + num2; break;
                    case '-': result = num1 - num2; break;
                    case '*': result = num1 * num2; break;
                    case '/': result = (num2 != 0) ? (num1 / num2) : 0; break;
                    default: result = 0;
                }
                SendInstruction(0xC0); // Move cursor to 2nd line
                display_number(result); // Display Result
                num1 = num2 = result = 0;
                operation = 0;
            }
            else if (Key == 'C') { // Clear Display
                SendInstruction(0x01);
                num1 = num2 = result = 0;
                operation = 0;
            }
        }

        lcd_delay(100);
    }
}
