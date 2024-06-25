/*
 * File:   microbot.c
 * Author: angel
 *
 * Created on 20 de septiembre de 2023, 09:11 PM
 * 
 * rev 1 27/04/24
 * Activacion de led de alerta pulsado
 * Se declara todas las variables a 8bits "char"
 * Se mejora el control de led
 * se elimino la funcion check estaba sin usar
 * 
 * rev2
 * Se modifica la tecnica de detencion
 * blanco (x RPM)-> habilita negro
 * 
 * 
 * 1RPM -> 0.01 = 10mS = 360
 *  */


#include <xc.h>
#include<stdint.h>
//#include "microbot.h"
//#include <12F508.h>

#define _XTAL_FREQ 4000000

#pragma config OSC = IntRC, WDT = OFF, CP = OFF, MCLRE = OFF

/************ DEFINICIONES ***********************/

#define			motor_d 		GPIObits.GP0              //Pin 7
#define			motor_i 		GPIObits.GP1              //Pin 6
#define			opto_d   		GPIObits.GP2              //Pin 5
#define			opto_i			GPIObits.GP3              //Pin 4
#define			btn_led   		GPIObits.GP4              //Pin 3
#define			bat_ctrl  		GPIObits.GP5              //Pin 2

/******************* Macros  ******************/

//LED
#define         led_on           btn_led = 0
#define         led_off           btn_led = 1
#define         control_on       bat_ctrl = 1
#define         control_off      bat_ctrl = 0

/************************************************/

/*              Variables           */
unsigned char  cont_led, cont, ctrl, buffer, buffer_anterior, f;
unsigned int  z; 
//unsigned int f;


__bit timer0 (){
    
    if (TMR0 >= 250 ){
        TMR0 = 0x00;
        return 1;
    }
    else{
        return 0;
    }
}
int sensores(void){
		
	if (bat_ctrl == 0){ //bateria fallo?
		TRISGPIO = 0b00001100; // Se configura para control opto, led
        control_off;
        return 0;		//falló; Voltaje <= 3.4v (3v es el limite menor segun datasheet)
    }
	else{						//No falló
		TRISGPIO = 0b00001100; // Se configura para control opto, led	
        control_on;         //Activa los opto acopladores
		__delay_ms(50);		//50 mili
        
		for(int a=0; a < 10; a++){
			
            __delay_ms(10);		//10 mili cada ciclo --> 200mili total
            
			if (opto_d == 0 && a < 5){
				cont++;
  			}
						
					
			if (opto_i == 0 && a > 4){
				cont++;
            }
		}		
        if(cont == 10){
                 cont = 0;
                 control_off;
                return 1;				//Optos OK
        }
            else{
                cont = 0;
                control_off;
                return 2;				//Falla en los opto
        }
    }
	}
	

void izquierda(void){
	motor_i=0;
	motor_d=1;
}

void derecha(void){
	motor_i=1;
	motor_d=0;
}

void adelante(void){
	motor_i=1;
	motor_d=1;
}

void para(void){
	motor_i=0;
	motor_d=0;
}

void led_alerta(int cant_led ){
    for(f=0; f <= cant_led;f++){
		 
            buffer = btn_led;
			if(btn_led == 1){ //led prendido?
			led_on;		//prende
			}
			else{
			led_off;			//apaga
			}
	__delay_ms(10);
    }
    led_off;
}

void pulsador(void){
    TRISGPIO = 0b00011100; // Se configura para control opto, pulsador
__delay_us(10);

buffer = btn_led;
z=0;
while (buffer == 1){
    __delay_ms(100);
	 buffer = btn_led;
    z += 1;
        if(z == 1000){
            SLEEP();
            z=0;
                 
       }
}
__delay_ms(500);  // tiempo para retirar el dedo del pulsador

TRISGPIO = 0b00001100; // Se configura para control opto, led
//__delay_us(10);

}

void manejo_errores(void){

int prueba = sensores();

if (prueba == 0){//tres pulsos Ocurrio una falla en bateria
	while(1){
		for(f=0;f <=3;f++){
        led_alerta(12); //led prendido por pulsos durante 120 milis
        __delay_ms(100);
        }
        __delay_ms(350);
        }
}
if (prueba == 2){ // Dos pulsos falla en los sensores
        led_alerta(20);
        __delay_ms(100);
        led_alerta(20);
        __delay_ms(500);
	
}
}

void led_run(void){
     if(cont_led == 8){ // toggle LED
         	if(btn_led){ //led prendido?, 1 = apagado
			led_on;		//prende
			}
			else{
			led_off;			//apaga
			}
			cont_led =0;
		}
     if(timer0()){
         cont_led++;
     }
             
}

void setup(void){
    TRISGPIO = 0b00101100; // se configura para leer estado bateria, led
	OPTION = 0b01000111; 
    para();
	cont_led = 0;
    buffer = 0;
    buffer_anterior = 0xff;
    z=0;
    TMR0 = 0x00;
    
}	

void main(void){
setup();
manejo_errores();
pulsador();	
	while(1){
		//activación de motores
	control_on;	
	 buffer = GPIO;
	buffer &= 0b00001100; //0x03
	
		switch (buffer){
			case 0x0c: // detener motores
			    if(z==3000){ //filtro para evitar bloqueos 
                    para();
                    pulsador();
                    z = 0;
                }
                else{
                    z++;
                    adelante();
                }
            break;
			case 0x04:
			derecha(); // activa el motor derecho detiene motor izquierdo, robot gira a la izquiera
            break;
			case 0x08:
			izquierda();// activa el motor izquierdo detiene motor derecho, robot gira a la derecha
            break;
			case 0x00:
			adelante();// activa ambos motores, robot abanza dercho al frente
            break;
		}
        buffer_anterior = buffer;
        control_off; 
        led_run();
         if(buffer != 0x00)para();  
         if(buffer != 0x0c)z = 0;;
     	}

}
