/**
 *
 *
 * @author Jose D Fuentes
 */

#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"

void GPIO_Peri_init (void);
void SSI0_sendData (uint16_t dat);
void pot_setVal (uint8_t slider);

uint8_t x = 0x00; // Valor inicial.

void Timer03ISR(void)
{
    TIMER3_ICR_R= 0x01 ; //LIMPIA BANDERA DE TIMER3
    pot_setVal(x++);
}

void SSI0ISR(void)
{
    SSI0_ICR_R=0xFF; //Limpio bandera de ISR
    TIMER3_CTL_R |= 0x01; //Se habilita timer 3 y cuenta
}

int main(void)
{
    GPIO_Peri_init();   // Función que habilita el SPI
    pot_setVal(x);
    while (1);
}

void GPIO_Peri_init (void) {
    SYSCTL_RCGCSSI_R = SYSCTL_RCGCSSI_R0; // Activa reloj al SSI0
    while ((SYSCTL_PRSSI_R & SYSCTL_PRSSI_R0) == 0); // Espera a que este listo

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // Activa reloj del GPIO A
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0) == 0); // Espera a que este listo

    GPIO_PORTA_AHB_AFSEL_R |= 0x3C; // Seleciona la función alterna de PA[2:5].
    GPIO_PORTA_AHB_PCTL_R = (GPIO_PORTA_AHB_PCTL_R & 0XFF0000FF) | 0x00FFFF00; // Configura las terminales de PA a su función de SSI0.
    GPIO_PORTA_AHB_AMSEL_R = 0x00; // Deshabilita la función analogica
    GPIO_PORTA_AHB_DIR_R = (GPIO_PORTA_AHB_DIR_R & ~0x3C) | 0x1C; // Configura al puerto como salida
    GPIO_PORTA_AHB_DEN_R |= 0x3C; // Habilita la función digital del puerto

    SSI0_CR1_R = 0x00; // Selecciona modo maestro/deshabilita SSI0. (p. 1247)
    SSI0_CPSR_R = 0x02; // preescalador (CPSDVSR) del reloj SSI (p. 1252)
    // configura para Freescale SPI; 16bit; 4 Mbps; SPO = 0; SPH = 0 (p. 1245)
    SSI0_CR0_R = (0x0100 | SSI_CR0_FRF_MOTO | SSI_CR0_DSS_16) & ~(SSI_CR0_SPO | SSI_CR0_SPH);

    SSI0_ICR_R=0xFF; //Limpio posible bandera pendiente de ISR
    SSI0_IM_R=0x40; //Habilito la interrupcion de fifo vacia y ultimo bit mandado
    NVIC_EN0_R= 1<<(7-0); //Habilito interrupcion en controlador de interrupciones

    SSI0_CR1_R |= SSI_CR1_SSE; // Habilita SSI0.

    //Inicializacion timer 3
    SYSCTL_RCGCTIMER_R |= 0X08; //HABILITA TIMER 3

    while ((SYSCTL_PRTIMER_R & 0x00000008) == 0){};  // Reloj del timer 3 listo?

    //Configuracion Timer 3
    TIMER3_CTL_R=0X00000000; //DESHABILITA TIMER EN LA CONFIGURACION
    TIMER3_CFG_R= 0X00000004; //CONFIGURAR PARA 16 BITS
    TIMER3_TAMR_R= 0X00000001; //CONFIGURAR PARA MODO ONE SHOT CUENTA HACIA ABAJO
    TIMER3_TAILR_R= 0XC8; // VALOR DE RECARGA
    TIMER3_TAPR_R= 0X00; // VALOR DE PREESCALADOR TIMER A
    TIMER3_ICR_R= 0X00000001 ; //LIMPIA POSIBLE BANDERA PENDIENTE DE TIMER3
    TIMER3_IMR_R |= 0X00000001; //ACTIVA INTRRUPCION DE TIMEOUT
    NVIC_EN1_R= 1<<(35-32); //HABILITA LA INTERRUPCION 35 DE  TIMER3 A

}

void SSI0_sendData (uint16_t dat) {
    // Envia dato de 16-bit
    SSI0_DR_R = dat; // envia dato.
}

void pot_setVal(uint8_t slider) {
    //Combine el valor del control deslizante con el código de comando de escritura.
    // Estructura del mensaje SPI: [comando (8-bits)][deslizador (8-bits)]
    SSI0_sendData(0x1100 | slider);
}
