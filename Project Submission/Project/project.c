#include <msp.h>

//MARK: Constants
#define DEBOUNCE_TIME 1500

void PORT1_IRQHandler(void);
void EUSCIA0_IRQHandler(void);

int main(void)
{
	
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; //Code segment to disable watchdog timer.
	
	//Begin Confirgurations. Start by clearing SEL.
	
	P1->SEL0 &= (uint8_t)(~((1<<4) | (1<<1) | (1<<0)));
	P1->SEL1 &= (uint8_t)(~((1<<4) | (1<<1) | (1<<0)));
	
	P2->SEL0 &= (uint8_t)(~(1<<0));
	P2->SEL1 &= (uint8_t)(~(1<<0));
	
	//Next, configure the input pins.
	
	P1->DIR &= (uint8_t)(~((1<<4) | (1<<1)));
	P1->REN |= (uint8_t)((1<<4) | (1<<1));
	P1->OUT |= (uint8_t)((1<<4) | (1<<1));
	P1->IE  &= (uint8_t)(~((1<<4) | (1<<1)));
	
	//Configure outpins now.
	
	P1->DIR |= (uint8_t) (1<<0);
	P2->DIR |= (uint8_t) (1<<0);
	P1->DS &= (uint8_t) (~(1<<0));
	P2->DS &= (uint8_t) (~(1<<0));
	P1->OUT &= (uint8_t)~(1<<0);
	P2->OUT &= (uint8_t)~(1<<0);
	P1->IE &= (uint8_t)~(1<<0);
	P2->IE &= (uint8_t)~(1<<0);
	
	//Next, configure inturrupts
	
	P1->IES |= (uint8_t)((1<<4) | (1<<1));
	P1->IFG &= (uint8_t)~((1<<4) | (1<<1));
	P1->IE |= (uint8_t)((1<<4) | (1<<1));
	
	NVIC_SetPriority(PORT1_IRQn, 2);
	NVIC_ClearPendingIRQ(PORT1_IRQn);
	NVIC_EnableIRQ(PORT1_IRQn);
	
	//UART Echo
	
	CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
	CS->CTL0 = 0;                           // Reset tuning parameters
	CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
	CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO
					CS_CTL1_SELS_3 |                // SMCLK = DCO
					CS_CTL1_SELM_3;                 // MCLK = DCO
	CS->KEY = 0;                            // Lock CS module from unintended accesses

	// Configure UART pins
	P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function

	// Configure UART
	EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
	EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
					EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK
	// Baud Rate calculation
	// 12000000/(16*9600) = 78.125
	// Fractional portion = 0.125
	// User's Guide Table 21-4: UCBRSx = 0x10
	// UCBRFx = int ( (78.125-78)*16) = 2
	EUSCI_A0->BRW = 78;                     // 12000000/16/9600
	EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
					EUSCI_A_MCTLW_OS16;

	EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
	EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
	EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

	// Enable sleep on exit from ISR
	SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;

	// Enable global interrupt
	__enable_irq();

	// Enable eUSCIA0 interrupt in NVIC module
	NVIC->ISER[0] = 1 << ((EUSCIA0_IRQn) & 31);

	// Enter LPM0
	__sleep();
	__no_operation();    
	
	__ASM("CPSIE I");
	
	while(1)
	{
		__WFI();
	}
	return 0; //Hopefully never reached.
	
	
}

void PORT1_IRQHandler(void)
{
	static uint8_t state = 0;
	if((P1->IFG & (uint8_t)(1<<4)) != 0)
	{
			P1->IFG &= ~(1<<4);
			if(state == 0)
			{
				P2->OUT ^= (1<<0);
				state ++;
			}
			else if(state == 1)
			{
				P1->OUT ^= (1<<0);
				P2->OUT &= ~(1<<0);
				state ++;
			}
			else if(state == 2)
			{
				P2->OUT ^= (1<<0);
				state ++;
			}
			else if(state == 3)
			{
				P1->OUT &= ~(1<<0);
				P2->OUT &= ~(1<<0);
				state = 0;
			}
	}
	else if((P1->IFG & (uint8_t)(1<<1)) != 0)
	{
			P1->IFG &= ~(1<<1);
			if(state == 0)
			{
				P1->OUT ^= (1<<0);
				P2->OUT ^= (1<<0);
				state = 3;
			}
			else if(state == 1)
			{
				P2->OUT &= ~(1<<0);
				state --;
			}
			else if(state == 2)
			{
				P1->OUT &= ~(1<<0);
				P2->OUT ^= (1<<0);
				state --;
			}
			else if(state == 3)
			{
				P2->OUT &= ~(1<<0);
				state --;
			}
	}
}
	
	// UART interrupt service routine
void EUSCIA0_IRQHandler(void)
{
	static uint8_t changeState = 0;
		
	if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
	{
			// Check if the TX buffer is empty first
			while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
		
			if(EUSCI_A0->RXBUF == 'f'){
				
				if(changeState == 0)
				{
					P2->OUT ^= (1<<0);
					changeState ++;
				}
				else if(changeState == 1)
				{
					P1->OUT ^= (1<<0);
					P2->OUT &= ~(1<<0);
					changeState ++;
				}
				else if(changeState == 2)
				{
					P2->OUT ^= (1<<0);
					changeState ++;
				}
				else if(changeState == 3)
				{
					P1->OUT &= ~(1<<0);
					P2->OUT &= ~(1<<0);
					changeState = 0;
				}
				
				EUSCI_A0->TXBUF = changeState+'0';
			}
			else if(EUSCI_A0->RXBUF == 'b'){
		
				if(changeState == 0)
				{
					P1->OUT ^= (1<<0);
					P2->OUT ^= (1<<0);
					changeState = 3;
				}
				else if(changeState == 1)
				{
					P2->OUT &= ~(1<<0);
					changeState --;
				}
				else if(changeState == 2)
				{
					P1->OUT &= ~(1<<0);
					P2->OUT ^= (1<<0);
					changeState --;
				}
				else if(changeState == 3)
				{
					P2->OUT &= ~(1<<0);
					changeState --;
				}
				
				EUSCI_A0->TXBUF = changeState+'0';
			}
			//Echo the received character back
	}
}
	

	


	
