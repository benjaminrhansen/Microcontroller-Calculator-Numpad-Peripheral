/*
 * Program: Number-pad Calculator using the MSP432 LaunchPad
 * File: main.c
 * Author: Benjamin Hansen as taught by Rex Fisher, BYU-Idaho
 * Description:
 *      This program is in development. Currently it can display 
 *      integers to the GLCD display. I'm working on making it
 *      display floating point numbers. Due to precision issues,
 *      the program only prints floating point numbers up to a predefined
 *      precision in the fractional part of the number.
 *      NOTE: 
 *              The program doesn't round, but truncates the from the last
 *              digit displayed in the fractional part
 */
#include "msp.h"
#include "stdio.h"

/* LEDs */
#define LED1 BIT0
#define LED2RED BIT0
#define LED2GREEN BIT1
#define LED2BLUE BIT2
#define RGB_LED LED2RED|LED2GREEN|LED2BLUE

/* pins */
#define CE  0x01    /* P6.0 chip select */
#define RESET 0x40  /* P6.6 reset */
#define DC 0x80     /* P6.7 register select */
#define S1 BIT1
#define DA BIT0

/* constants */
#define DELAY 5000000
#define PRECISION 4 /* fractional digits displayed */ 

/* types */
#define CALC_TYPE long double

/* define the pixel size of display */
#define GLCD_WIDTH  84
#define GLCD_HEIGHT 48

/* prototypes */
void GLCD_setCursor(unsigned char, unsigned char);
void GLCD_clear(void);
void GLCD_init(void);
void GLCD_data_write(unsigned char);
void GLCD_command_write(unsigned char);
void GLCD_putchar(int);
void GLCD_putstr(char *);
void display_current_state(); // refreshes the display
void set_focus(const CALC_TYPE *);
void SPI_init(void);
void SPI_write(unsigned char);
void blink(const int);
void error_blink(const int);
void assert(const int, char *);
void test_math_op();
void test_putnum();
void test_positive_ints();
void test_negative_ints();
void test_positive_floats();
void test_negative_floats();
void test_alphabet();

/* global variables */
/* variables directly affecting display */
CALC_TYPE lhs = 0;
CALC_TYPE rhs = 0;
char operation = '\0'; // null-char means no-operation
// start focus on the left hand side (lhs)
CALC_TYPE * focus = &lhs; // point to the address of lhs
int focus_on_fractional = 0; // focus on the whole part first until a decimal point
// 64-bit int for the powers of 10
long long int fractional_pow10 = 10; // 10^1 for the fractional divisor

/* other variables */
int i = 0, ind_formula=0;

/* sample font table */
/* rows 0-63 correspond to characters Space through _ in ASCII */
/* Strings using characters in this range 
 * can be displayed to the GLCD via GLCD_put_str() */
const char font_table[][6] = {
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  /*   */
   {0x00, 0x00, 0x5f, 0x00, 0x00, 0x00},  /* ! */
   {},  /* " */
   {},  /* # */
   {},  /* $ */
   {},  /* % */
   {},  /* & */
   {},  /* ' */
   {},  /* ( */
   {},  /* ) */
   {0x44, 0x28, 0x10, 0x28, 0x44, 0x00},  /* * */
   {0x08, 0x08, 0x7f, 0x08, 0x08, 0x00},  /* + */
   {},  /* , */
   {0x08, 0x08, 0x08, 0x08, 0x08, 0x00},  /* - */
   {0x00, 0x60, 0x60, 0x00, 0x00, 0x00},  /* . */
   {0x60, 0x30, 0x18, 0x0c, 0x06, 0x00},  /* / */
   {0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00},  /* 0 */
   {0x00, 0x44, 0x42, 0x7f, 0x40, 0x00},  /* 1 */
   {0x44, 0x62, 0x62, 0x52, 0x4c, 0x00},  /* 2 */
   {0x00, 0x41, 0x49, 0x49, 0x76, 0x00},  /* 3 */
   {0x00, 0x0f, 0x08, 0x08, 0x7f, 0x00},  /* 4 */
   {0x27, 0x49, 0x49, 0x49, 0x31, 0x00},  /* 5 */
   {0x00, 0x3c, 0x4a, 0x49, 0x31, 0x00},  /* 6 */
   {0x42, 0x22, 0x12, 0x0a, 0x06, 0x00},  /* 7 */
   {0x00, 0x36, 0x49, 0x49, 0x36, 0x00},  /* 8 */
   {0x06, 0x09, 0x09, 0x09, 0x7e, 0x00},  /* 9 */
   {},  /* : */
   {},  /* ; */
   {},  /* < */
   {0x00, 0x24, 0x24, 0x24, 0x24, 0x00},  /* = */
   {},  /* > */
   {},  /* ? */
   {},  /* @ */
   {0x7e, 0x11, 0x11, 0x11, 0x7e, 0x00},  /* A */
   {0x7f, 0x49, 0x49, 0x49, 0x36, 0x00},  /* B */
   {0x3e, 0x41, 0x41, 0x41, 0x22, 0x00},  /* C */
   {0x7f, 0x41, 0x41, 0x41, 0x3e, 0x00},  /* D */
   {0x7f, 0x49, 0x49, 0x49, 0x41, 0x00},  /* E */
   {0x7f, 0x09, 0x09, 0x09, 0x01, 0x00},  /* F */
   {0x3e, 0x41, 0x49, 0x49, 0x7a, 0x00},  /* G */
   {0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00},  /* H */
   {0x41, 0x41, 0x7f, 0x41, 0x41, 0x00},  /* I */
   {0x20, 0x40, 0x40, 0x40, 0x3f, 0x00},  /* J */
   {0x7f, 0x08, 0x14, 0x22, 0x41, 0x00},  /* K */
   {0x7f, 0x40, 0x40, 0x40, 0x40, 0x00},  /* L */
   {0x7f, 0x02, 0x0c, 0x02, 0x7f, 0x00},  /* M */
   {0x7f, 0x04, 0x08, 0x10, 0x7f, 0x00},  /* N */
   {0x3e, 0x41, 0x41, 0x41, 0x3e, 0x00},  /* O */
   {0x7f, 0x09, 0x09, 0x09, 0x06, 0x00},  /* P */
   {0x3e, 0x41, 0x51, 0x61, 0x7e, 0x00},  /* Q */
   {0x7f, 0x09, 0x19, 0x29, 0x46, 0x00},  /* R */
   {0x26, 0x49, 0x49, 0x49, 0x32, 0x00},  /* S */
   {0x01, 0x01, 0x7f, 0x01, 0x01, 0x00},  /* T */
   {0x3f, 0x40, 0x40, 0x40, 0x3f, 0x00},  /* U */
   {0x1f, 0x20, 0x40, 0x20, 0x1f, 0x00},  /* V */
   {0x3f, 0x40, 0x38, 0x40, 0x3f, 0x00},  /* W */
   {0x63, 0x14, 0x08, 0x14, 0x63, 0x00},  /* X */
   {0x03, 0x04, 0x78, 0x04, 0x03, 0x00},  /* Y */
   {0x61, 0x51, 0x49, 0x45, 0x43, 0x00},  /* Z */
   {},  /* [ */
   {},  /* \ */
   {},  /* ] */
   {},  /* ^ */
   {},  /* _ */
   {0x0c, 0x12, 0x24, 0x12, 0x0c, 0x00},  /* ♡ */
   /* smiley's first half */
   {0x00, 0x00, 0x7e, 0x81, 0xb5, 0xa1},
   /* smiley's second half */
   {0xa1, 0xb5, 0x81, 0x7e, 0x00, 0x00},
};

/**
 * MAIN
 * main setups the configurations for the peripherals, optionally 
 * (through comment/un-comment) runs all tests, and then
 * calls the calculator driver function which drives the devices
 * implementing a calculator using a number pad and an old Nokia
 * display
 */
int main(void) {

   WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;  /* hold the watchdog timer */

   /* configure calculator setup */
   P1->DIR |= BIT0;      /* set up pin P1.0 (red LED) as output */
   P2->DIR |= (RGB_LED);  /* set up pins P2.0, P2.1, P2.2 (R, G, and B LEDs) 
                             as output */
   P1->OUT &= ~LED1;     /* turn off red LED */
   P2->OUT &= ~(RGB_LED); /* turn off RGB LED */

   P4->DIR &= ~(BIT0|BIT1|BIT2|BIT3); // set up pins P4.0, P4.1, P4.2, P4.3 
   // (pins for the input from the keypad 
   //  controller) as input 
   P1->DIR &= ~S1;     /* set up pin P1.1 (S1) as input */
   P1->REN |= S1;      /* connect pull resistor to pin P1.1 (S1) */
   P1->OUT |= S1;      /* configure pull resistor as pull up */
   P1->IFG &= ~S1;     /* clear the interrupt flag for pin P1.1 (S1) */
   P1->IE |= S1;       /* enable the interrupt for pin P1.1 (S1) */

   P3->DIR &= ~DA; /* set up pin P3.0 (DA (interrupt pin)) as input */
   P3->REN |= DA;  /* connect pull resistor to pin P3.0 */
   P3->OUT |= DA;  /* configure pull resistor as pull up */
   P3->IFG &= ~DA; /* clear interrupt flag for pin P3.0 (DA (int pin)) */
   P3->IE |= DA;  /* enable the interrupt for pin P3.0 (DA (interrupt pin)) */

   NVIC->ISER[1] |= 0x20;  /* enable port 3 interrupts (see p. 89 in text)*/
   NVIC->ISER[1] |= 0x08; /* enable port 1 interrupts (see p. 89 in text)*/

   _enable_interrupts();

   /* configure GLCD */
   GLCD_init();    /* initialize the GLCD controller */
   GLCD_clear();   /* clear display and  home the cursor */

   /* start tests */
   test_math_op();
   GLCD_clear();   /* clear display and  home the cursor */
   test_alphabet();
   GLCD_clear();   /* clear display and  home the cursor */
   test_putnum();
   GLCD_clear();   /* clear display and  home the cursor */
   /* end tests */

   // display the current state (should display lhs = 0)
   display_current_state();

   while (1) {} /* wait for an interrupt */
}

/**
 * Set focus to the given pointer's value
 * Helps to reset the fractional boolean variable each time
 */
void set_focus(const CALC_TYPE * f) {
   focus = f; // assign the global to the given input
   focus_on_fractional = 0; // a new focus means to focus on the whole part
   // reset the fractional power of 10
   fractional_pow10 = 10; // 10^1
}

/***
 * Blink the green LED n times
 * deprecated
 ***/
void blink(int n) {
   // if the green light was on,
   // turn it off and then wait a little
   if ((P2->OUT & LED2GREEN) != 0x00) {
      P2->OUT &= ~(LED2GREEN); /* turn off the green LED */
      __delay_cycles(DELAY);
   }
   // toggle the green light off and on
   for (; n > 0; --n) {
      P2->OUT |= LED2GREEN; // turn the green light on
      //for (k = 0; k < DELAY; ++k) {}
      __delay_cycles(DELAY);
      P2->OUT &= ~(LED2GREEN); /* turn off the green LED */
      //for (k = 0; k < DELAY; ++k) {}
      __delay_cycles(DELAY);
   }
}
      
/***
 * Blink the red LEDs n times
 * deprecated
 ***/
void error_blink(int n) {
   // if the red LEDs were on,
   // turn them off and then wait a little
   if ((P2->OUT & (LED1 | LED2RED)) != 0x00) {
      P1->OUT &= ~(LED1); // turn off red LED
      P2->OUT &= ~(LED2RED); // turn off red LED of RGB LED
      __delay_cycles(DELAY);
   }
   // toggle the red LEDs on and off n times
   for (; n > 0; --n) {
      P1->OUT |= LED1; // turn on red LED
      P2->OUT |= LED2RED; // turn on red LED of RGB LED
      __delay_cycles(DELAY);
      P1->OUT &= ~(LED1); // turn off red LED
      P2->OUT &= ~(LED2RED); // turn off red LED of RGB LED
      __delay_cycles(DELAY);
   }
}

/***
 * math_op: An operation function to handle the addition, subtraction, 
 *      multiplication, or division of a LHS and RHS operand
 */
CALC_TYPE math_op(const CALC_TYPE lhs_operand, const char op, 
               const CALC_TYPE rhs_operand) {
   if (op == '=') {
      assert(0, "ILLEGAL MATH OP \'=\'");
   }
   CALC_TYPE result = 0;
   switch(op) {
      case '+': /* add */
         result = lhs_operand + rhs_operand;
         break;
      case '-': /* subtract */
         result = lhs_operand - rhs_operand;
         break;
      case '*': /* multiply */
         result = lhs_operand * rhs_operand;
         break;
      case '/': /* divide */
         if (rhs_operand != 0) {
            result = lhs_operand / rhs_operand;
         }
         // else simulate a division-by-zero error
         else {
            assert(0, "DIV BY ZERO");
         }
         break;

      default: /* set off an alarm */
         assert(0, "UNKOWN OP");
         break;
         
   }
   return result;
}

/***
* IRQ handler for port 1
***/
void PORT1_IRQHandler(void){

  uint32_t status;

  status = P1->IFG;    /* get the interrupt status for port 1 */
  P1->IFG &= ~BIT1;    /* clear the interrupt for port 1, pin 1 */

  if(status & BIT1){   /* if SW was pressed */
     P2->OUT &= ~LED2RED;  /*turn off red LED at pin P2.0 */
     P1->OUT &= ~LED1; /*turn off red LED at pin P1.0 */
     P2->OUT &= ~(LED2BLUE | LED2GREEN); /*turn off blue and green LEDs */
  }
}

/***
* IRQ handler for port 3
***/
void PORT3_IRQHandler(void){

  uint32_t status;

  // declare a key variable to decode
  // which key was pressed
  uint8_t key = 0;

  // first, get the status of the interrupt
  status = P3->IFG;   /* get the interrupt status for port 3 */
  P3->IFG &= ~DA;    /* clear the interrupt for port 3, pin 0 */

  if(status & BIT0){  /* if any key was pressed */
     key = keypad_decode();  /* determine which key was pressed */
     // determine how to update the global state
     // check if the key was an opeartion 
     if (key >= 0xA && key <= 0xD) { 
        // check if the previous operation was the null character
        // if the focus is already pointing to the rhs
        if (operation != '\0' && focus == &rhs) {
           // then we have a math operation to perform
           // combine operands
           lhs = math_op(lhs, operation, rhs); // e.g. 3 '+' 4 = 7 = lhs
           rhs = 0;
           // operations are reset later in the switch statement
        }
        // else we're adding a rhs operand
        // operations are reset later in the switch statement
        // operation isn't the null character and we're focusing on the lhs
        // always set the focus on the rhs
        set_focus(&rhs);
     }

     // perform the key's function
     // SWITCH on the key
     switch (key) {
        /* handle addition: operation = '+' */
        case 0xA: /* “A” was pressed */
           operation = '+';
           break;
        /* handle subtraction: operation = '-' */
        case 0xB: /* if “B” was pressed */
           operation = '-';
           break;
        /* handle multiplication: operation = '*' */
        case 0xC:  /* if “C” was pressed */
           operation = '*';
           break;
        /* handle division: operation = '/' */
        case 0xD:  /* if "D" was pressed */
           operation = '/';
           break;
        /* handle decimal point */
        case 0xE:  /* if "*" was pressed */
           focus_on_fractional = 1; // focus on the fractional part
           break;
        /* handle equality: operation = '=' */
        case 0xF:  /* if "#" was pressed */
           // is the focus on the rhs
           if (focus == &rhs) {
              // combine operands
              lhs = math_op(lhs, operation, rhs); // e.g. 3 '-' 4 = -1 = lhs
              rhs = 0; // reset rhs
              operation = '='; // initially set operation to the equal sign
           }
           // else focus is on the lhs and we're hitting equal again
           // simulate a clear on the device (reset to zero)
           else {
              assert(focus == &lhs, "ERROR IN LOGIC");
              lhs = 0; // reset lhs
              rhs = 0; // for robustness
              // reset the operation 
              operation = '\0'; // no operation currently
              GLCD_clear(); // clear the screen
           }
           // an equality seeks to always show the answer
           // refocus on the lhs
           set_focus(&lhs); // for robustness, reset focus and boolean

           break;
        /* handle numbers */
        default: /* a number was pressed */
           /* modify the value of the focus */

           // if the focus was zero
           // make sure the focus was pointing to NULL (zero)
           if (focus != 0) {
              // IF we should be focusing on the fractional part, 
              // add a fractional part to the previous value and
              // modify the fractional power of 10 for the divisor of the 
              // next input
              if (focus_on_fractional) {
                 // dereference focus to assign its value to
                 // itself plus the new input divided by 10
                 // e.g. 3 -> 3 + 1/10 = 3.1
                 *focus = (*focus) + (CALC_TYPE)key / (CALC_TYPE)fractional_pow10; 
                 // increase the power of 10
                 // to take 10^2 to 10^3 just multiply the left by 10
                 fractional_pow10 *= 10; // 10 -> 10*10 = 10^2  
              }
              // work on the whole part
              else {
                 // dereference focus to assign its value to
                 // itself times 10 plus the new input
                 *focus = (*focus) * 10 + key; // e.g. 3 -> 3(10) + 4 = 34
              }

           }
           break;
     }
  }

  // always refresh the display on every input
  display_current_state();
}


/***
* keypad decoder function
***/
uint8_t keypad_decode() {
  uint8_t key = 0;
  uint8_t port = 0;

  port += P4->IN & BIT0; /* input value from pin P4.0 */
  port += P4->IN & BIT1; /* input value from pin P4.1 */
  port += P4->IN & BIT2; /* input value from pin P4.2 */
  port += P4->IN & BIT3; /* input value from pin P4.3 */

  switch(port){
    case 0x0D: key = 0x0; break; /* 0 */
    case 0x00: key = 0x1; break; /* 1 */
    case 0x01: key = 0x2; break; /* 2 */
    case 0x02: key = 0x3; break; /* 3 */
    case 0x04: key = 0x4; break; /* 4 */
    case 0x05: key = 0x5; break; /* 5 */
    case 0x06: key = 0x6; break; /* 6 */
    case 0x08: key = 0x7; break; /* 7 */
    case 0x09: key = 0x8; break; /* 8 */
    case 0x0A: key = 0x9; break; /* 9 */
    case 0x03: key = 0xA; break; /* A */
    case 0x07: key = 0xB; break; /* B */
    case 0x0B: key = 0xC; break; /* C */
    case 0x0F: key = 0xD; break; /* D */
    case 0x0C: key = 0xE; break; /* * */
    case 0x0E: key = 0xF; break; /* # */
  }

  return key;

}

/**
 * assert: assert the truth of the condition
 *      if condition is not true, sound the alarm
 */
void assert(const int condition, char * message) {
   
   if (!condition) {
      // display the error message for a little while
      // clear the GLCD first
      GLCD_clear();
      GLCD_putstr("ERROR: ");
      GLCD_putstr(message);
      __delay_cycles(4*DELAY); // delay between errors as necessary
      GLCD_clear();
      display_current_state(); // turn back to the current state
      // turn on alarm
      P2->OUT |= LED2RED; // red of the RGB LED
      P1->OUT |= LED1;    // red LED
   }
}

/**
 * Test the operation function
 */
void test_math_op() {
   // add 
   //   integers
   assert(math_op(4,'+',5) == 4+5,"MATH OP ASSERT 1");
   assert(math_op(5,'+',4) == 5+4,"MATH OP ASSERT 2");
   assert(math_op(292,'+',123) == 292+123,"MATH OP ASSERT 3");
   assert(math_op(-233,'+',343) == -233+343,"MATH OP ASSERT 4");
   assert(math_op(-233,'+',-233) == -233+(-233),"MATH OP ASSERT 5");
   assert(math_op(9898,'+',-9899) == 9898+(-9899),"MATH OP ASSERT 6");
   assert(math_op(9898,'+',-9897) == 9898+(-9897),"MATH OP ASSERT 7");
   //   float
   assert(math_op(9898.5,'+',-9897.5) == 9898.5+(-9897.5),"MATH OP ASSERT 8");
   assert(math_op(9898.5,'+',-9897) == 9898.5+(-9897),"MATH OP ASSERT 9");
   assert(math_op(9898.5,'+',-9898) == 9898.5+(-9898),"MATH OP ASSERT 10");
   assert(math_op(-9898.5,'+',-9898.5) == -9898.5+(-9898.5),"MATH OP ASSERT 11");
   assert(math_op(-9898.75,'+',-9898.5) == -9898.75+(-9898.5),"MATH OP ASSERT 12");
   assert(math_op(9898.75,'+',9898.5) == 9898.75+9898.5,"MATH OP ASSERT 13");
   //   long double

   // subtract
   assert(math_op(4,'-',5) == 4-5,"MATH OP ASSERT 14");
   assert(math_op(292,'-',123) == 292-123,"MATH OP ASSERT 15");
   // multiply
   assert(math_op(2,'*',2) == 2*2,"MATH OP ASSERT 16");
   assert(math_op(4,'*',2) == 4*2,"MATH OP ASSERT 17");
   assert(math_op(4,'*',5) == 4*5,"MATH OP ASSERT 18");
   assert(math_op(123,'*',13) == 123*13,"MATH OP ASSERT 19");
   // divide
   assert(math_op(10,'/',5) == 10.0/5.0,"MATH OP ASSERT 20");
   assert(math_op(20,'/',5) == 20.0/5.0,"MATH OP ASSERT 21");
   assert(math_op(14,'/',4) == (CALC_TYPE)14/(CALC_TYPE)4,"MATH OP ASSERT 22");
   assert(math_op(12,'/',8) == (CALC_TYPE)12/(CALC_TYPE)8,"MATH OP ASSERT 23");
   //assert(math_op(0,'/',0) == 0,"MATH OP ASSERT 24"); // should set the alarm off
}

/*
 * The smiley face is defined to be the last two characters of the array
 * not currently used
 */
void GLCD_put_smiley_face() {
   int size = sizeof(font_table) / sizeof(font_table[0]);
   GLCD_putchar(size - 2); // first half
   GLCD_putchar(size - 1); // second half
}

/**
 * Display a c-string on the GLCD
 */
void GLCD_putstr(char * str) {
   // increment the string pointer on byte at a time
   // through each char
   for(; *str != '\0'; ++str) {
      // take the ASCII value of the character
      // subtract 32 to get the index related to
      // the char to put in the range of
      // 0 (for Space) to 62 (for @)
      GLCD_putchar(*str - 32);
   }
}

/**
 * Display a number on the GLCD
 */
void GLCD_putnum(CALC_TYPE num) {
   /* variables */
   int i;     // floating-point display FOR loop
   int digit; // range = 0 through 9

   // 64-bit integer
   long long int pow10 = 1; // 10^0 = 1
   long long int whole = 0; // must be set later
   CALC_TYPE fractional_part = 0; // must be set later
   
   /* STEP 1 */
   // check if the number is negative
   if (num < 0) {
      // display the negative sign
      GLCD_putstr("-"); 
      // negate to a positive number
      // possible loss of data if num is the maximum
      // negative number possible given the calculation type
      num = -num; // e.g. -(-5) = 5
   }
   // NOT IMPLEMENTED YET
    
   /* STEP 2 */ 
   // extract the whole part from the fractional
   // 64-bit integer
   whole = (long long int)num; // whole part of num, e.g. (int)3.14 = 3 
   fractional_part = num - whole; // e.g. 3.14 - 3 = 0.14


   /* STEP 3 */ 
   // find the greatest power of 10
   // we can integer-divide the whole part of the
   // number by that is meaningful to display
   // (instead of an infinite-trail of leading zeros)
   // WHILE there is a greater power of 10
   // that we can integer-divide the
   // number by and obtain a non-zero answer
   while (pow10 * 10 <= whole) {
      pow10 *= 10;
   } 

   /* STEP 4 */
   // display the whole part of the number one
   // digit at a time
   // WHILE the power of 10 hasn't gone below 10^0 = 1
   while (pow10 >= 1) {
      digit = whole / pow10; // e.g. 313 / 10^2 = 3
      whole %= pow10;         // e.g. 313 % 10^2 = 13
      // print digit
      // add 48 to convert the number to its ASCII 
      // visual representation
      // subtract 32 to index from 0 (Space) to the underscore
      // in the font table
      // 48-32=16 is leftover needing to be added
      GLCD_putchar(digit + 16); 

      // update the power of 10
      pow10 /= 10; // e.g. 10^3 / 10 = 10^2
   }
   //GLCD_putstr("+");
   
   /* STEP 5 */
   // check if there is a fractional part
   if (fractional_part != 0.0) {
      GLCD_putstr("."); // if so, display the decimal point

      // display the fractional part of the number one
      // digit at a time up to the defined precision
      // this way of displaying the fractional part of the number
      // is at the mercy of the precision of floating-point operations 
      // on the MSP432
      for (i = 0; i < PRECISION; ++i) {
         fractional_part *= 10; // e.g. 10(0.314) = 3.14
         digit = (int)fractional_part; // e.g. (int)3.14 = 3
         fractional_part -= digit;     // e.g. 3.14 - 3 = 0.14


         /* print digit */
         // add 48 to convert the number to its ASCII 
         // visual representation
         // subtract 32 to index from 0 (Space) to the underscore
         // in the font table
         // 48-32=16 is leftover needing to be added
         // under the assumption that multiplying by 10 and casting
         // that result to an integer should give an integer
         // in the range 0-9, make sure that is the case
         // IF digit is in [0, 9]
         if (digit < 10 && digit >= 0) {
            GLCD_putchar(digit + 16); 
         }
      }
   }

}

/**
 * Put the character on the GLCD
 * according to the 6 integers at the 
 * given indexed-column in the font table
 */
void GLCD_putchar(int c)
{
    int i;
    for(i = 0; i < 6; i++)
        GLCD_data_write(font_table[c][i]);
}

void GLCD_setCursor(unsigned char x, unsigned char y)
{
    GLCD_command_write(0x80 | x); /* column */
    GLCD_command_write(0x40 | y); /* bank (8 rows per bank) */
}

/* clears the GLCD by writing zeros to the entire screen */
void GLCD_clear(void)
{
    int32_t index;
    for(index = 0; index < (GLCD_WIDTH * GLCD_HEIGHT / 8); index++)
        GLCD_data_write(0x00);
    GLCD_setCursor(0, 0); /* return to the home position */
}

/* send the initialization commands to PCD8544 GLCD controller */
void GLCD_init(void)
{
    SPI_init();
    /* hardware reset of GLCD controller */
    P6->OUT |= RESET;   /* deasssert reset */

    GLCD_command_write(0x21);   /* set extended command mode */
    GLCD_command_write(0xB8);   /* set LCD Vop for contrast */
    GLCD_command_write(0x04);   /* set temp coefficient */
    GLCD_command_write(0x14);   /* set LCD bias mode 1:48 */
    GLCD_command_write(0x20);   /* set normal command mode */
    GLCD_command_write(0x0C);   /* set display normal mode */
}

/* write to GLCD controller data register */
void GLCD_data_write(unsigned char data)
{
    P6->OUT |= DC;              /* select data register */
    SPI_write(data);            /* send data via SPI */
}

/* write to GLCD controller command register */
void GLCD_command_write(unsigned char data)
{
    P6->OUT &= ~DC;             /* select command register */
    SPI_write(data);            /* send data via SPI */
}

void SPI_init(void)
{
    EUSCI_B0->CTLW0 = 0x0001;   /* put UCB0 in reset mode */
    EUSCI_B0->CTLW0 = 0x69C1;   /* PH=0, PL=1, MSB first, Master, SPI, SMCLK */
    EUSCI_B0->BRW = 3;          /* 3 MHz / 3 = 1MHz */
    EUSCI_B0->CTLW0 &= ~0x001;   /* enable UCB0 after config */

    P1->SEL0 |= 0x60;           /* P1.5, P1.6 for UCB0 */
    P1->SEL1 &= ~0x60;

    P6->DIR |= (CE | RESET | DC); /* P6.7, P6.6, P6.0 set as output */
    P6->OUT |= CE;              /* CE idle high */
    P6->OUT &= ~RESET;          /* assert reset */
}

void SPI_write(unsigned char data)
{
    P6->OUT &= ~CE;             /* assert /CE */
    EUSCI_B0->TXBUF = data;     /* write data */
    while(EUSCI_B0->STATW & 0x01);/* wait for transmit done */
    P6->OUT |= CE;              /* deassert /CE */
}



/**
 * Display the current state of the
 * calculator. If the operation is not the null character
 * or equal sign, display the operation and RHS
 */
void display_current_state() {
   // always clear the display before displaying the
   // current state
   GLCD_clear();
   GLCD_putnum(lhs);
   // IF the opeartion is not the null character or the equal sign
   if (operation != '\0' && operation != '=') {
      // display the operation and the rhs
      GLCD_putchar(operation - 32); // get operation within index range
      GLCD_putnum(rhs);       // display the rhs
   }
}

/**
 * Display the currently available alphabet
 */
void test_alphabet() {
   // get the number of elements in the array
   int num_elements = sizeof(font_table) / sizeof(font_table[0]);
   int num_char; // used in for loop
   for (num_char = 0; num_char < num_elements; ++num_char) {
      GLCD_putchar(num_char);    // display the num_char letter
   }
}

/**
 * Test some positive integers
 */
void test_positive_ints() {

   GLCD_putnum(0);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(1);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(2);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(10);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(11);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(36);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(313);
   GLCD_putchar(' '); // put a space in between
   // test the biggest int possible: 2^64 - 1
   // should map to -(2^64) + (2^64 - 1) rem 2^64
   // where rem is the modulo
   // this should display the character just before
   // 0 because that is -1 according to the logic in putnum
   //GLCD_putnum(18446744073709551615);
   //GLCD_putchar(' '); // put a space in between
   // test the biggest unsigned int possible: 2^32 - 1
   GLCD_putnum(4294967295);
   GLCD_putchar(' '); // put a space in between
   __delay_cycles(DELAY);
}

/**
 * Test some egative integers
 */
void test_negative_ints() {

   GLCD_putnum(-1);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-3);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-313);
   GLCD_putchar(' '); // put a space in between
   // test the biggest unsigned int possible: -(2^32)
   GLCD_putnum(-4294967296);
   GLCD_putchar(' '); // put a space in between
   __delay_cycles(DELAY);
}
/**
 * Test some positive floats
 */
void test_positive_floats() {
   GLCD_putnum(3.14);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(3.1);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(0.3);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(0.33333333333);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(0.00000000001);
   GLCD_putchar(' '); // put a space in between
   // a more precise float
   GLCD_putnum(0.000000000000000001);
   GLCD_putchar(' '); // put a space in between
   // big floating point
   GLCD_putnum(33333.14159265359);
   GLCD_putchar(' '); // put a space in between
   __delay_cycles(DELAY);
}

/**
 * Test some negative floats
 */
void test_negative_floats() {
   GLCD_putnum(-3.14);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-3.1);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-0.3);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-0.33333333333);
   GLCD_putchar(' '); // put a space in between
   GLCD_putnum(-0.00000000001);
   GLCD_putchar(' '); // put a space in between
   // a more precise float
   GLCD_putnum(-0.000000000000000001);
   GLCD_putchar(' '); // put a space in between
   // big floating point
   GLCD_putnum(-33333.14159265359);
   GLCD_putchar(' '); // put a space in between
   __delay_cycles(DELAY);
}

/**
 * Display the currently available alphabet
 */
void test_putnum() {
   /* positive integers */
   test_positive_ints();
   GLCD_clear();

   /* negative integers */
   test_negative_ints();
   GLCD_clear();

   /* positive floats */
   test_positive_floats();
   GLCD_clear();

   /* negative floats */
   test_negative_floats();
   GLCD_clear();

}
