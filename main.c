/*
 * This program receives SO FAR two input integers 
 * and performs a mathematical operation (ADD) on them
 */
#include "msp.h"

#define LED1 BIT0
#define LED2RED BIT0
#define LED2GREEN BIT1
#define LED2BLUE BIT2
#define RGB_LED LED2RED|LED2GREEN|LED2BLUE
#define S1 BIT1
#define DA BIT0
#define DELAY 250000
#define OP_MAX_SIZE 100000000
#define CALC_TYPE float

// global variables
int i = 0, ind_formula=0;
CALC_TYPE lhs_operand = 4;
//int temp_formula[FORMULA_LIMIT]; 
// no bool in base C
//int is_first_num; // is this the first number in the formula
int enter_time = 0, alarm = 0;

// prototypes
void blink(const int n);
void error_blink(const int n);
void test_math_op();

int main(void) {

   WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;  /* hold the watchdog timer */

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

   // testing
   //blink(3);
   test_math_op();
   // testing

   while (1); /* wait for an interrupt */
}

/***
 * Blink the green LED n times
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
CALC_TYPE math_op(const CALC_TYPE lhs_operand, const char operation, 
               const CALC_TYPE rhs_operand) {
   CALC_TYPE result = 0;
   switch(operation) {
      case '+': /* add */
         result = lhs_operand + rhs_operand;
         //blink(result);
         break;
      case '-': /* subtract */
         result = lhs_operand - rhs_operand;
         //blink(result);
         break;
      case '*': /* multiply */
         result = lhs_operand * rhs_operand;
         if (result < 10 && result > 0) {
            blink(result);
            __delay_cycles(4*DELAY);
         }
         break;
      case '/': /* divide */
         if (rhs_operand != 0) {
            result = lhs_operand / rhs_operand;
            //blink(result);
         }
         // else simulate a division-by-zero error
         else {
            P2->OUT |= (LED2RED | LED1);
         }
         break;
     
      default: /* set off an alarm */
         P2->OUT |= (LED2RED | LED1);
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
     alarm = 0;
     enter_time = 0;
     P2->OUT &= ~LED2RED;  /*turn off red LED at pin P2.0 */
     P1->OUT &= ~LED1; /*turn off red LED at pin P1.0 */
     P2->OUT &= ~(LED2BLUE | LED2GREEN); /*turn off blue and green LEDs */

     // testing //
     blink(3);
     // end testing //
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
     if(!alarm){
        // switch on the key
        switch (key) {
           case 0xA: /* “A” was pressed */
              //P2->OUT &= ~(RGB_LED); /* turn off the RGB LED */
              break;
           case 0xB: /* if “B” was pressed */
              //P2->OUT &= ~(RGB_LED); /* turn off the RGB LED */
              // turn on the blue light to confirm formulaword save
              //P2->OUT |= LED2BLUE;   /* turn on the blue LED (P2.2) */
              break;
           case 0xC:  /* if “C” was pressed */
              //P2->OUT &= ~(RGB_LED); /* turn off the RGB LED */      
              break;
           case 0xD:  /* if "D" was pressed */
              //P2->OUT &= ~(RGB_LED); /* turn off the RGB LED */
              //P2->OUT |= LED2GREEN;  /* turn on the green LED (P2.1) */
              //enter_time = 0; // Reset enter-try counter
              blink(lhs_operand);
              break;
           case 0xE:
              break;
           case 0xF:
              break;
           default:
              break;
        }
     }
  }
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
void assert(const int condition, CALC_TYPE error_blink_n) {
   if (!condition) {
      error_blink(error_blink_n);
      __delay_cycles(4*DELAY); // delay between errors as necessary
   }
}

/**
 * Test the operation function
 */
void test_math_op() {
   int j = 0; // for denoting which asserts fired
   // add 
   //   integers
   assert(math_op(4,'+',5) == 4+5,++j);
   assert(math_op(5,'+',4) == 5+4,++j);
   assert(math_op(292,'+',123) == 292+123,++j);
   assert(math_op(-233,'+',343) == -233+343,++j);
   assert(math_op(-233,'+',-233) == -233+(-233),++j);
   assert(math_op(9898,'+',-9899) == 9898+(-9899),++j);
   assert(math_op(9898,'+',-9897) == 9898+(-9897),++j);
   //   float
   assert(math_op(9898.5,'+',-9897.5) == 9898.5+(-9897.5),++j);
   assert(math_op(9898.5,'+',-9897) == 9898.5+(-9897),++j);
   assert(math_op(9898.5,'+',-9898) == 9898.5+(-9898),++j);
   assert(math_op(-9898.5,'+',-9898.5) == -9898.5+(-9898.5),++j);
   assert(math_op(-9898.75,'+',-9898.5) == -9898.75+(-9898.5),++j);
   assert(math_op(9898.75,'+',9898.5) == 9898.75+9898.5,++j);
   //   long double

   // subtract
   assert(math_op(4,'-',5) == 4-5,++j);
   assert(math_op(292,'-',123) == 292-123,++j);
   // multiply
   assert(math_op(2,'*',2) == 2*2,++j);
   assert(math_op(4,'*',2) == 4*2,++j);
   assert(math_op(4,'*',5) == 4*5,++j);
   assert(math_op(123,'*',13) == 123*13,++j);
   // divide
   assert(math_op(10,'/',5) == 10.0/5.0,++j);
   assert(math_op(20,'/',5) == 20.0/5.0,++j);
   assert(math_op(9,'/',5) == (float)9/(float)5,++j);
   assert(math_op(11,'/',5) == (float)11.0/(float)5.0,++j);
   //assert(math_op(0,'/',0) == 0,++j); // should set the alarm off
   
   // test edge cases
}

