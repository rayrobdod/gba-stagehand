#ifndef ISR_H
#define ISR_H

enum InterruptType {
	II_VBLANK, II_HBLANK, II_VCOUNT,
	II_TIMER0, II_TIMER1, II_TIMER2, II_TIMER3,
	II_COM,
	II_DMA0, II_DMA1, II_DMA2, II_DMA3,
	II_KEYPAD, II_GAMEPACK,
	II_COUNT
};

typedef void (*isr)(void);

void isr_switchboard_init(void);
isr isr_switchboard_set(enum InterruptType type, isr handler);
void isr_enable(enum InterruptType type);
void isr_disable(enum InterruptType type);

#endif        //  #ifndef ISR_H
