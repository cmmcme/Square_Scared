#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "_main.h"
#include "_glcd.h"
#include "_adc.h"
#include "_buzzer.h"
unsigned int Data_ADC4 = 0;

// 각 속도를 가지는 구조체
struct obj{
	int x;	// x 좌표 (세로)
	int y;	// y 좌표 (가로)
	int sp; // 속도
};

struct obj fall[20]; // 장애물 배열
struct obj heart[10]; // 하트 배열
int life = 5; // 생명력
int f_index = 0; // 장애물 배열의 index
int h_index = 0; // 하트 배열의 index
unsigned int cnt = 0; // Timer Interrupt 를 위해 쓰이는 cnt 값
int score = 0; // 총 점수를 가지고 있는 변수
int sec = 0; // cnt가 7000이 넘으면 1 증가하는 초를 세는 변수
bool isGameOver = 0; // Game이 끝남을 알리는 bool 타입 변수

// 포트 초기화
void Port_init(void){
	PORTA=0x00; DDRA=0xff;
	PORTB=0xff; DDRB = 0b11111111;
	PORTC=0x00; DDRC=0xf0;
	PORTD=0x80; DDRB = 0b11111111;
	PORTE=0x00; DDRE=0xff;
	PORTF=0x00; DDRF=0x00;
}

// 인터럽트 초기화 CTC MODE 사용
void Interrupt_init(void){

	TCCR0 |= (1<<WGM01 | 1<<CS01) ;
	TIMSK |= (1<<OCIE0);
	OCR0 = 100;
	EICRA=0x02; // 하강 에지 트리
	EIMSK=0x03; //INT0 사용
}

// device 전체 초기화
void init_devices(void){

	cli();   // disable all interrupts
	Interrupt_init();
	Port_init();  	//initialize Port
	Adc_init();		//initialize ADC
	lcd_init();     //initialize GLCD
	sei();  	//re-enable interrupts
}

// Timer Interrupt Routine
ISR(TIMER0_COMP_vect){
	cnt++;
	if(cnt > 7000){  // cnt 가 7000이 되면 1초 증가 함
		cnt = 0;
		sec++;

		// 장애물을 생성
		fall[f_index].x = 15;
		fall[f_index].y = (rand()%128);		// 랜덤한 y의 위치에 장애물을 생성
		fall[f_index].sp = (rand()%5)+2;	// 떨어지는 속도를 다르게 하기 위해 랜덤값 사용

		f_index++;			// 인덱스 위치 증가
		if(f_index >= 20)
		f_index = 0;

		if(sec % 3 == 0) {		// 장애물이 3개 생성 되면 하트를 1개 생성하기 위해 sec % 3 == 0 사용
			heart[h_index].x = 15;
			heart[h_index].y = (rand()%128);		// 랜덤한 y의 위치에 장애물을 생성
			heart[h_index].sp = (rand()%5)+1;	// 떨어지는 속도를 다르게 하기 위해 랜덤값 사용

			h_index++;			// 인덱스 위치 증가
			if(h_index >= 10)
			h_index = 0;
		}
	}

}

// 0 Pin Interrupt Routine
ISR(INT0_vect){

	int i;
	int hit = 0;

	// 캐릭터와 하트가 닿는 범위를 계산하여 그 범위 안에 들어왔으면 life를 증가 시킴
	for(i = 0; i < 10; i++){
		if(life!=5&&((63 >= heart[i].x) &&(55 <= heart[i].x)&&(Data_ADC4+8 >= heart[i].y) &&(Data_ADC4-8 <= heart[i].y))){
			S_S2();
			life ++;
			score++;
			hit = 1;	// 범위 안에 들어 왔음을 알려줌
			// 해당 하트를 지움
			heart[i].x = 0;
			heart[i].y = 0;
			heart[i].sp = 0;
		}
	}
	// 범위 안에 들어오지 않았을 경우
	if(hit == 0){
		life--;		// 생명력을 1 감소시킴.

	}

	// 생명력이 0이 된 경우
	if(life == 0){
		PORTB = 0xff;		// LED를 모두 꺼줌
		S_S6();				// Buzzer를 울림
		isGameOver = true;		// Game이 끝났음을 알림
	}

}

// 1 Pin Interrupt Routine
ISR(INT1_vect){
	// 모든 변수를 초기화 시키고, LCD를 CLEAR 함
	lcd_clear();
	S_Start();
	int i;
	isGameOver = false;
	life = 5;
	for(i = 0 ; i < 20; i ++){
		fall[i].x = 0;
		fall[i].y = 0;
		fall[i].sp = 0;
	}
	for(i = 0; i < 10; i++) {
		heart[i].x = 0;
		heart[i].y = 0;
		heart[i].sp = 0;
	}
	score = 0;
	f_index = 0;
	h_index = 0;
	sec = 0;
	cnt = 0;
	
}

// Show Heart Pixel
void GLCD_Heart(int x1, int y1){
	int i;
	// GLCD_Dot 을 이용해 Heart Pixel 을 찍음
	for(i = 0; i < 5; i++){
		if(i == 0){
			GLCD_Dot(x1-2, y1-1);
			GLCD_Dot(x1-2, y1+1);
		}
		if(i ==1 || i == 2){
			int j;
			int cnt = 2-i;
			for(j = 0; j<5; j++){
				GLCD_Dot(x1-cnt, y1-2+j);
			}
		}
		if(i == 3){
			GLCD_Dot(x1+1,y1-1);
			GLCD_Dot(x1+1, y1);
			GLCD_Dot(x1+1, y1+1);
		}
		if(i == 4){
			GLCD_Dot(x1+2, y1);
		}
	}
	
}

// Show LCD
void Show_Screen(void) {
	int i;

	// 3자리의 점수를 띄움
	lcd_string(0,50,"Score : ");
	lcd_xy(0,60);
	GLCD_3DigitDecimal(score);

	// 남은 생명력의 갯수를 띄움
	for(i = 0; i<life; i++){
		lcd_xy(0,i+1);
		lcd_char('*');
		lcd_xy(0,0);
	}

	unsigned char led = 0xff;

	// LED를 이용해 남은 생명럭의 갯수를 밝힘
	for(i = 0; i <= life; i++) {
		PORTB = led;
		led <<= 1;
	}

	GLCD_Rectangle(63,Data_ADC4,55,Data_ADC4+8); // 캐릭터 위치 출력

	for(i = 0; i < 20; i++){
		if(fall[i].x!=0 && fall[i].y!= 0)
			GLCD_Circle(fall[i].x+=fall[i].sp,fall[i].y,2); // 장애물 위치 출력
	}

	for(i = 0; i < 10; i++) {
		if(heart[i].x!=0 && heart[i].y!= 0)
			GLCD_Heart(heart[i].x+=heart[i].sp,heart[i].y); // 하트 위치 출력
	}

}

// IsTough the falling fall
void IsTouch(void) {

	int i;

	// 캐릭터와 장애물이 닿는 범위를 계산하여 그 범위 안에 들어왔으면 life를 감소 시킴
	for(i = 0; i < 20; i++){
		if((((63 >= fall[i].x) &&(55 <= fall[i].x)&&(Data_ADC4+8 >= fall[i].y) &&(Data_ADC4-8 <= fall[i].y)))){
			life --;
			fall[i].x = 0;
			fall[i].y = 0;
			fall[i].sp = 0;
		}
	}


	// 생명력이 0이 된 경우
	if(life == 0) {
		PORTB = 0xff;		// LED를 모두 꺼줌
		S_S6();				// Buzzer를 울림
		isGameOver = true;		// Game이 끝났음을 알림
	}

}

// Game start screen
void SquareScared(void){
	lcd_string(2,3,"==============");
	lcd_string(3,3,"Square Scared");
	lcd_string(4,3,"==============");
}

int main(void){

	init_devices();  // 디바이스 초기화
	lcd_clear();
	SquareScared();
	_delay_ms(2800);
	S_Start();
	while(1){
		Data_ADC4 = Read_Adc_Data(4) / 8; // 아날로그 0번 포트 읽기
		_delay_ms(100); // 딜레이 200ms
		lcd_clear(); // 그래픽 LCD 클리어
		ScreenBuffer_clear(); // 스크린 버퍼 클리어

		// 게임이 종료됨
		if(isGameOver) {
			init_devices(); // 디바이스 초기화
			lcd_string(2,0,"Game Over");
			lcd_string(3,0,"Game Over");

			// 점수를 알려줌
			lcd_string(4,0,"Your score : ");
			lcd_xy(4,10);
			GLCD_3DigitDecimal(score);

			// 1번 Pin을 누르면 재시작 함
			lcd_string(5,0,"Press 1Pin to Restart");
		}

		// 게임이 끝나지 않음
		else {
			Show_Screen();	// 게임 화면을 띄움
			IsTouch();		// 장애물과 닿았는지 확인함
		}
	}
}



