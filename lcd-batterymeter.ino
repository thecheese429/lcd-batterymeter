#include <LiquidCrystal.h>
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 

#define INTERVAL 200 //the amount of time to delay between readings
#define RES 0.62     //define value of resistor used as shunt
#define ENDVOLT 2.9  //define the voltage at which the test stops
#define VIN A1       //define the pin number for input A, which will be used to read the cell positive voltage
#define SHUNT A0     //define the pin number for B, which tells us the voltage on the other side of the shunt resistor
#define GATE1 0      //the pin used to control the gate on the first mosfet        
#define GATE2 1       //The pin used to control the gate on the second mosfet
#define CONTRAST 20   //The contrast level. Change the value between 0 and 255 if you don't see anything
#define CONTRASTPIN 9 //The pin connected to the contrast on the LCD. Must have a PWM output
#define RESCHECKVOLT 3.5 //the resistance check will occur as soon as the voltage drops below this value
#define MINSTART 4.10		//This is the minimum voltage on a cell for the test to begin





double now = 0;
double lastTime = 0;

double voltage = 0;
double current = 0;
double cellResistance = 0;
double capacity = 0;

boolean zeroVolts = 0;
boolean newCell = 0;

byte state = 0; //keep track of the current state of the program

byte ohm[8] = {   
  0b00000,
  0b00000,
  0b01110,
  0b10001,
  0b10001,
  0b10001,
  0b01010,
  0b11011
  };

// initialize the library with the numbers of the interface pins
//make sure to change this to the appropriate pins for your setup
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void updateDisplay(){
  long currTime = now;
	int hourTens = currTime/36000000;
	currTime -= hourTens * 36000000;
	int hourOnes = currTime / 3600000;
	currTime -= hourOnes * 3600000;
	int minuteTens = currTime / 600000;
	currTime -= minuteTens * 600000;
	int minuteOnes = currTime / 60000;
	currTime -= minuteOnes * 60000;
	int secondTens = currTime / 10000;
	currTime -= secondTens * 10000;
	int secondOnes = currTime / 1000;
	
	
	
	lcd.clear();
  lcd.leftToRight();
  lcd << "V=" << voltage << "v"; //Streaming (<<) is an easier way to write varied outputs like these


  lcd.setCursor(15,0);
  lcd.rightToLeft();
  lcd << "  " << current;       // this is a cheap way to write something on the right side of the screen
  lcd.leftToRight();            // Write something right to left (minus one character), then without moving the cursor
  lcd << "I=" << current << "A";// write the same thing left to right (with all the characters, this time)

  lcd.setCursor(0,1);
  lcd.leftToRight();
  lcd << (int)capacity << "mAh";

  lcd.setCursor(8,1);
	//lcd << hour(now) << ":" << minute(now) << ":" << second(now) ;
  lcd << hourTens << hourOnes << ":" << minuteTens << minuteOnes << ":" << secondTens << secondOnes;
}

double capacityCalc(){
	//take the current values and check the time since last called, then return the charge that has passed since last call in units of mAh
	return(current * (now - lastTime) / 3600 );
}

void updateMeasurements(){
	int voltRaw = analogRead(VIN);		//read the values from the pins without doing any math
	int shuntRaw = analogRead(SHUNT); //so that the values are read as close to simultaneous as possible
	
	lastTime = now;		//shift now back to lastTime
	now = millis();		//keep track of when the measurements were made
	
	voltage = (double)voltRaw *5/1024;
	current = (double)(voltRaw - shuntRaw)*5/1024 / RES;
	
}

void readResistance(){
  double v1;
  double v2;
  double i1;
  double i2;
  
	digitalWrite(GATE1, HIGH);
	digitalWrite(GATE2, HIGH);
	delay(10);
	updateMeasurements();
	v1 = voltage;
	i1 = current;
	delay(10);
	digitalWrite(GATE1, LOW);
	delay(10);
	updateMeasurements();
	v2 = voltage;
	i2 = current;

	digitalWrite(GATE1, HIGH);
	cellResistance = (v1 - v2) / (i2-i1); 
}

void setup() {
  pinMode(GATE1, OUTPUT);       //Set the appropriate pin modes
  pinMode(GATE2, OUTPUT);
  pinMode(CONTRASTPIN, OUTPUT);
  analogWrite(CONTRASTPIN, CONTRAST);//Set the contrast for the LCD screen
  digitalWrite(GATE1, LOW);
  digitalWrite(GATE2, LOW);
  
  lcd.begin(16, 2);
	lcd.clear();
	lcd << "hello";
  lcd.createChar(0, ohm);
	delay(1000);
  
}

void loop() {
	
	if(state == 0){ //This is the first state. It will check the voltage and direct us to the appropriate next state
		updateMeasurements();
		
		if(voltage < .5){
			state = 1;
		}
		else if(voltage > MINSTART){ // this means that the voltage is high enough to begin the test
			state = 4;
		}
		else{
			state = 3; // this will occur if the battery is above the "empty" value but below the minimum beginning value
		}
		
	}
	else if(state == 1){ //This is the state if the program started with no battery
	lcd.clear();
	lcd << "Insert battery";
	lcd.setCursor(0,1);
	lcd << "then press reset";
		while(state == 1){
			updateMeasurements();
			delay(50);
			if(voltage>1){
				state = 2;
			}
		}
	}
	else if(state == 2){ //This is the state after a battery has been inserted, after starting with no battery
		//prompt for a reset
		lcd.clear();
		lcd << "Press reset";
		while(true){
			lcd.setCursor(0,1);
			lcd << "V = " << voltage;
			updateMeasurements();
			delay(100);
		}
		
		
	}
	else if(state == 3){ //This is the state if the program was started with a weak battery
		//Inform of weak battery, display voltage
		lcd.clear();
		lcd << "Weak battery";
		while(true){
			lcd.setCursor(0,1);
			lcd << "V = " << voltage;
			updateMeasurements();
			delay(100);
		}
		

	}                    
	else if(state == 4){ //This is the state in which the capacity is measured until the resistance check
		digitalWrite(GATE1, HIGH);
		digitalWrite(GATE2, HIGH);
		while(state == 4){
			updateMeasurements();
			capacity += capacityCalc();
			delay(INTERVAL);
			updateDisplay();
			if(voltage < RESCHECKVOLT){
				state = 5;
			}
		}
	}                    
	else if(state == 5){ //This is the state in which the resistance is measured
		lcd.clear();
		lcd << "Testing res";
		delay(1000);
		readResistance();
		if(cellResistance != 0){
			state = 6;
		}
	}                    
	else if(state == 6){ //This is the state in which the capacity is measured, after the resistance check, until the battery gets low
		digitalWrite(GATE1, HIGH);
		digitalWrite(GATE2, HIGH);
		while(state == 6){
			updateMeasurements();
			capacity += capacityCalc();
			delay(INTERVAL);
			updateDisplay();
			if(voltage < ENDVOLT){ //once the voltage gets below the lower threshold, stop the test and go to state 7.
				state = 7;
				digitalWrite(GATE1, LOW);
				digitalWrite(GATE2, LOW);				
			}
		}
	}                    
	else if(state == 7){ //This is the state after the check is complete and the battery is still in place
		lcd.setCursor(0,0);
		lcd << "Done.           ";
		lcd.setCursor(15,0);
		lcd.rightToLeft();
		lcd << "R=" << cellResistance;
		lcd.leftToRight();
		lcd << "R= " << cellResistance;
		lcd.write( byte(0) );
		while(state==7){
			if(voltage<.5){	//go to state 8 when the voltage drops below half a volt, or when the battery is removed.
				state = 8;
			}
			updateMeasurements();
			delay(100);
		}
	}                    
	else if(state == 8){ //This is the state after the check is complete and the battery is removed
		lcd.clear();
		lcd << "Insert new cell";
		lcd.setCursor(0,1);
		lcd << (int)capacity << "mAh";
		lcd.setCursor(15,1);
		lcd.rightToLeft();
		lcd << "R=" << cellResistance;
		lcd.leftToRight();
		lcd << "R=" << cellResistance;
		lcd.write(byte(0));
		while(state==8){
			if(voltage>1){ //go to the next state once a battery has been inserted, and the voltage goes over 1v
				state = 9;
			}				
			updateMeasurements();
			delay(100);
		}
	}                    
	else if(state == 9){ //This is the state after the measurement is complete and a new battery has been inserted
		lcd.setCursor(0,0);
		lcd << "Reset to begin  ";
		while(state == 9){
		}
	}
}
