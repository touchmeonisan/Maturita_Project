/*|Start of imports||Start of imports||Start of imports||Start of imports||Start of imports||Start of imports|*/
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "laser.h"
#include "line.h"

/*|End of imports||End of imports||End of imports||End of imports||End of imports||End of imports||End of imports|*/
/*|Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff|*/
TaskHandle_t Task1;

int Global_RunButtonPressed = 0;
int Global_ModeSelectVar = 0;

const char* ssid = "Sumec_#01";			/*sumec's ap ssid*/
const char* password = "SumecPass1";	/*sumec's ap pass*/
IPAddress local_IP(182,168,1,22);		/*sumec's local ip*/

IPAddress gateway(182,168,1,5);
IPAddress subnet(255,255,255,0);

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String webpage = "<!DOCTYPE html><html style = 'font-family:consolas; line-height:0.7;'><head><title>Sumec's main control</title></head><body><h1>Sumec_In</h1><h2>Mode: <select name='Mode_Select' id='ModeSelect'><option value='0'>Stop</option><option value='1'>Normal</option><option value='2'>Debug</option></select><button type='button' id='RunButton'>Run</button></h2><h3>Curent: <span id='CurrentMode'>-</span></h3><br><h1>Sumec_Out</h1><h2>VL53L0X sensors (distance)</h2><h3>Range: <span id='Dist_Range'>-</span></h3><table style='width:100%' border= 1px;><tr><th><h3>Left:</h3></th><th><h3>Front:</h3></th><th><h3>Right:</h3></th></tr><tr><th><span id='Dist_Left'>-</span></th><th><span id='Dist_Front'>-</span></th><th><span id='Dist_Right'>-</span></th></tr></table><h2>QRE1113 sensors (line)</h2><h3>Threshold: <span id='Line_Thre'>-</span></h3><table style='width:100%' border= 1px;><tr><th><h3>Front left:</h3></th><th><h3>Front right:</h3></th></tr><tr><th><span id='Line_FL'>-</span></th><th><span id='Line_FR'>-</span></th></tr><tr><th><h3>Back left:</h3></th><th><h3>Back right:</h3></th></tr><tr><th><span id='Line_BL'>-</span></th><th><span id='Line_BR'>-</span></th></tr></table><h2>Code pos.</h2><h3><span id='Code_Pos'>-</span></h3></body><script>var Socket;document.getElementById('RunButton').addEventListener('click', button_send_back);function init() {Socket = new WebSocket('ws://' + window.location.hostname + ':81/');Socket.onmessage = function(event) {processCommand(event);};}function button_send_back() {var SendObj = {RunButton: '1',ModeSelectVar: document.getElementById('ModeSelect').value};Socket.send(JSON.stringify(SendObj));}function processCommand(event) {var obj = JSON.parse(event.data);document.getElementById('CurrentMode').innerHTML = obj.CurrentMode;document.getElementById('Dist_Range').innerHTML = obj.Dist_Range;document.getElementById('Dist_Left').innerHTML = obj.Dist_Left;document.getElementById('Dist_Front').innerHTML = obj.Dist_Front;document.getElementById('Dist_Right').innerHTML = obj.Dist_Right;document.getElementById('Line_Thre').innerHTML = obj.Line_Thre;document.getElementById('Line_FL').innerHTML = obj.Line_FL;document.getElementById('Line_FR').innerHTML = obj.Line_FR;document.getElementById('Line_BL').innerHTML = obj.Line_BL;document.getElementById('Line_BR').innerHTML = obj.Line_BR;document.getElementById('Code_Pos').innerHTML = obj.Code_Pos;console.log(obj.CurrentMode);console.log(obj.Dist_Range);console.log(obj.Dist_Left);console.log(obj.Dist_Front);console.log(obj.Dist_Right);console.log(obj.Line_Thre);console.log(obj.Line_FL);console.log(obj.Line_FR);console.log(obj.Line_BL);console.log(obj.Line_BR);console.log(obj.Code_Pos);}window.onload = function(event) {init();}</script></html>";

int interval = 500;
unsigned long prevoiusMillis = 0;

StaticJsonDocument<400> JsonDoc_tx;
StaticJsonDocument<400> JsonDoc_rx;

void webSocketEvent (byte num, WStype_t type, uint8_t * payload, size_t length) {
	switch (type) {
		case WStype_DISCONNECTED:
			Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
			Serial.print("Client #");
			Serial.print(num);
			Serial.println(" Disconnected");
			break;
		
		case WStype_CONNECTED:
			Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
			Serial.print("Client #");
			Serial.print(num);
			Serial.println(" Connected");
			break;
		
		case WStype_TEXT:
			DeserializationError error = deserializeJson(JsonDoc_rx, payload);
			if(error) {
				Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
				Serial.println("deserializeJson() failed");
				return;
			}
			else {	/*recieve*/
				const char* RunButton = JsonDoc_rx["RunButton"];
				const int ModeSelectVar = JsonDoc_rx["ModeSelectVar"];
				
				Global_RunButtonPressed = String(RunButton).toInt();
				Global_ModeSelectVar = String(ModeSelectVar).toInt();
				Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
				Serial.println(RunButton);
				Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
				Serial.println(ModeSelectVar);
			}
			break;
	}
}
/*|Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff||Webserver and AP stuff|*/

String Global_CurrentMode = "[starting]";			/*mode*/
String Global_Dist_Range = "Empty";			/*distance sensors*/
String Global_Dist_Left = "Empty";			/*distance sensors*/
String Global_Dist_Front = "Empty";			/*distance sensors*/
String Global_Dist_Right = "Empty";			/*distance sensors*/
String Global_Line_Thre = "Empty";				/*line sensors*/
String Global_Line_FL = "Empty";				/*line sensors*/
String Global_Line_FR = "Empty";				/*line sensors*/
String Global_Line_BL = "Empty";				/*line sensors*/
String Global_Line_BR = "Empty";				/*line sensors*/
String Global_Code_Pos = "Nothing yet";			/*code pos*/

/*|Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0|*/
void CodeForTask1(void * parameter) {	/*Code for core 0*/
	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.println("] started");
	
	for(;;) {	/*|void loop() core 0||void loop() core 0||void loop() core 0||void loop() core 0||void loop() core 0||void loop() core 0||void loop() core 0||void loop() core 0|*/
		server.handleClient();
		webSocket.loop();
		
		unsigned long now = millis();
		if(now - prevoiusMillis > interval) {	/*send*/
			String jsonString = "";
			JsonObject object = JsonDoc_tx.to<JsonObject>();
			object["CurrentMode"] = Global_CurrentMode;	/*mode*/
			
			object["Dist_Range"] = Global_Dist_Range;	/*distance sensors*/
			object["Dist_Left"] = Global_Dist_Left;
			object["Dist_Front"] = Global_Dist_Front;
			object["Dist_Right"] = Global_Dist_Right;
			
			object["Line_Thre"] = Global_Line_Thre;	/*line sensors*/
			object["Line_FL"] = Global_Line_FL;
			object["Line_FR"] = Global_Line_FR;
			object["Line_BL"] = Global_Line_BL;
			object["Line_BR"] = Global_Line_BR;
			
			object["Code_Pos"] = Global_Code_Pos;	/*code pos*/
			
			serializeJson(JsonDoc_tx, jsonString);
			webSocket.broadcastTXT(jsonString);
			
			prevoiusMillis = now;
			
			
			
			Global_Dist_Left = String(LASER_Get(2, 0, 1));
			Global_Dist_Front = String(LASER_Get(3, 0, 1));
			Global_Dist_Right = String(LASER_Get(1, 0, 1));
			
			Global_Line_FL = String(LINE_Get(1, 0, 1));
			Global_Line_FR = String(LINE_Get(2, 0, 1));
			Global_Line_BL = String(LINE_Get(3, 0, 1));
			Global_Line_BR = String(LINE_Get(4, 0, 1));
			
		}
	}
}
/*|Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0||Core 0|*/

/*declares and defines before void setup()*/

/*|===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===|*/
/*|===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===||===Add all imports and defines===|*/
/*#include "laser.h" already exists*/
/*#include "line.h" already exists*/

#include <Arduino.h>
#include "Wire.h"
#include <VL53L0X.h>
#include "motors.h"

//defines for demo
#define Button 13
#define led 14
int Sensor1 = 0;
int Sensor2 = 0;
int Sensor3 = 0;
int Sensor = 0;
int ButtonDown = 0;


int hodnota_cary = 1000;
int Range = 150;

int cas_zaznam = 0;

int stop = 1;

int laser_number;
// promněná určující mód programu
int Global_ModeSelectvar;
// určuje zdali je barva spíš bílá nebo černá

int SumecMode = 0;


/*|void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()|*/
void setup() {
	Serial.begin(115200);
	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.println("] started");
	
	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
	Serial.print("Setting up Access Point ... ");
	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
	Serial.print("Starting Access Point ... ");
	Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");

	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
	Serial.print("AP SSID = ");
	Serial.println(ssid);
	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
	Serial.print("AP Pass = ");
	Serial.println(password);
	Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");
	Serial.print("IP address = ");
	Serial.println(WiFi.softAPIP());
	
	server.on("/", [] () {
		server.send(200, "text/html", webpage);
	});
	server.begin();
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);
	
	/*|normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()|*/
	
	/*|===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===|*/
	/*|===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===||===Add all void setup() code===|*/
	
	MOTORS_Setup();
	LASER_Setup();
	pinMode(led, OUTPUT);
	Serial.begin(9600);

	while(LASER_Get(1, Range, 0) == 0 && LASER_Get(2, Range, 0) == 0 && LASER_Get(3, Range, 0) == 0){
		MOTORS_Go(-255/2*-1, 255/2*-1);

		//if(millis() - cas_zaznam == 2000){
			//cas_zaznam = millis();
			//Range = Range + 100;
		//}

		while(stop == 0){
			MOTORS_Go(0, 0);
		}
	}
	Global_Code_Pos = "Setup end";
	
	
	
	
	/*|normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()||normal void setup()|*/
	
	xTaskCreatePinnedToCore(	/*This command should be at the end of void setup()*/
		CodeForTask1, 	/*Task Function*/
		"Task_1", 		/*Name*/
		3500, 			/*Stack size*/
		NULL, 			/*Parameter*/
		0, 				/*Priority*/
		&Task1, 		/*Task handle*/
		0);				/*Core*/
}
/*|void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()||void detup()|*/



void loop() {	/*|void loop() core 1||void loop() core 1||void loop() core 1||void loop() core 1||void loop() core 1||void loop() core 1||void loop() core 1||void loop() core 1|*/
	
	/*|===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===|*/
	/*|===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===||===Add all void loop() code===|*/
	
	Global_Code_Pos = "New loop";
	
	switch(SumecMode){

	case 1:
		Global_CurrentMode = "Normal";
		Global_Code_Pos = "mode 1";
		if(LINE_Get(1, hodnota_cary, 0) == 0 && LINE_Get(2, hodnota_cary, 0) == 0){
			
			//třídící proměná
			laser_number = 0;
			
			//třídění laserů pomocí proměné

			Serial.println(LASER_Get(1, Range, 0));

			if(cas_zaznam == 0){
				if(LASER_Get(3, Range, 0) == 1 ){	 // přední laser
					laser_number = laser_number + 1;
					//Serial.println(laser_number);
					digitalWrite(led, HIGH);
				}


				if(LASER_Get(2, Range, 0) == 1 ){	 // levý laser
					laser_number = laser_number + 3;
					//Serial.println(laser_number);
					digitalWrite(led, HIGH);
				}

				
				if(LASER_Get(1, Range, 0) == 1 ){	 // pravý laser
					laser_number = laser_number + 5;
					//Serial.println(laser_number);
					digitalWrite(led, HIGH);
				}

				if(laser_number == 0 ){
					digitalWrite(led, LOW);
				}
			}

			//Serial.println(laser_number);

			// rozpohybování Sumce pomocí proměné "laser_number" vzniklé po třídění	
			switch(laser_number){

				case 0:
					MOTORS_Go(255*-1, 255*-1);
					Global_Code_Pos = "No Enemy";
				case 1:
					MOTORS_Go(255*-1, 255*-1);
					Global_Code_Pos = "Enemy found! (fwd)";
					delay(30); 
				break;

				case 3:
					MOTORS_Go(-255/2*-1, 255/2*-1);
					Global_Code_Pos = "Enemy found! (left)";
				break;

				case 5:
					MOTORS_Go(255/2*-1, -255/2*-1);
					Global_Code_Pos = "Enemy found! (right)";
				break;

				case 4:
					MOTORS_Go(255*-1, 100*-1);
					Global_Code_Pos = "Enemy found! (fwd + left)";
				break;

				case 6:
					MOTORS_Go(100*-1, 255*-1);
					Global_Code_Pos = "Enemy found! (fwd + right)";
				break;

				case 9:
					MOTORS_Go(255*-1, 255*-1);
					Global_Code_Pos = "Enemy found! (all)";
				break;
			}
			
			// možnost zastavení programu pomocí stop proměné
			while(stop == 0){
				MOTORS_Go(0, 0);
				
			}

			if(cas_zaznam > 0){
			delay(1);
			cas_zaznam = cas_zaznam - 1;
			}


		}


		//dotek bílé čáry levým senzorem
		if(LINE_Get(1, hodnota_cary, 0) == 1){
			MOTORS_Go(255/2*-1, -255/2*-1);
			delay(500);
			cas_zaznam = 10;	 
		}
		//dotek bílé čáry pravým senzorem
		if(LINE_Get(2, hodnota_cary, 0) == 1){
			MOTORS_Go(-255/2*-1, 255/2*-1);
			delay(500);
			cas_zaznam = 10;	 
		}
		break;
		
	case 0:
		Global_CurrentMode = "Stopped";
		delay(10);
		break;
		
	case 2:
		Global_CurrentMode = "Debug";
		
	
	}	/*end of sumec mode switch*/
	
	
	Global_Line_Thre = String(hodnota_cary);
	Global_Dist_Range = String(Range);
	
	
	
	if(Global_RunButtonPressed==1) {	//if the run button was pressed somewhere in the past after th last button resert
		Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");	//print what core sent a serial print - "needed" before every serial print - good for debug
		Serial.println(Global_RunButtonPressed);	//print that the button was pressed and later detected by this core
		
		Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");	//print what core sent a serial print - "needed" before every serial print - good for debug
		Serial.println(Global_ModeSelectVar);	//print what mode was set
		
		Global_RunButtonPressed = 0;	//reset the button
		
		SumecMode = Global_ModeSelectVar;
	}

/*end of code*/


	/*	examlple of the run button use
	if(Global_RunButtonPressed==1) {	//if the run button was pressed somewhere in the past after th last button resert
		Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");	//print what core sent a serial print - "needed" before every serial print - good for debug
		Serial.println(Global_RunButtonPressed);	//print that the button was pressed and later detected by this core
		
		Serial.print("Core ["); Serial.print(xPortGetCoreID()); Serial.print("]: ");	//print what core sent a serial print - "needed" before every serial print - good for debug
		Serial.println(Global_ModeSelectVar);	//print what mode was set
		
		Global_RunButtonPressed = 0;	//reset the button
		
		//do something, ex. change the mode using the command goto [label];
	}*/
	
	/*	examples of changing the mode displayed on the webserver - isnt limited to "Stop", "Normal" and "Debug" - is used just for displaying what mode is sumec in
	Global_CurrentMode = "Stop";
	Global_CurrentMode = "Normal";
	Global_CurrentMode = "Debug";
	*/
	
	/*	examples of setting the values shown on the webserver
	//LASER_DistLeft LASER_DistRight LASER_DistFront	//these might be usefull
	Global_Dist_Range = String(random(100, 501));	//used for displaying what threshold is used in measuring the distance - replace random with a function - KEEP THE String()
	Global_Dist_Left = String(random(10, 501));		//used for displaying the distance optained from the left laser sensor - replace random with a function - KEEP THE String()
	Global_Dist_Front = String(random(10, 501));	///used for displaying the distance optained from the front laser sensor - replace random with a function - KEEP THE String()
	Global_Dist_Right = String(random(10, 501));	///used for displaying the distance optained from the right laser sensor - replace random with a function - KEEP THE String()
	
	//analogRead(32) analogRead(33) analogRead(34) analogRead(35)	//these might be usefull
	Global_Line_Thre = String(random(100, 5001));	//used for displaying what threshold is used in sensing the line - replace random with a function - KEEP THE String()
	Global_Line_FL = String(random(10, 5001));		//used for displaying what value the front left line sensor - replace random with a function - KEEP THE String()
	Global_Line_FR = String(random(10, 5001));		//used for displaying what value the front right line sensor - replace random with a function - KEEP THE String()
	Global_Line_BL = String(random(10, 5001));		//used for displaying what value the back left line sensor - replace random with a function - KEEP THE String()
	Global_Line_BR = String(random(10, 5001));		//used for displaying what value the back right line sensor - replace random with a function - KEEP THE String()
	*/
	
	/*	examples of setting the display value for "code pos." - just for debug - not limited to these values - set to a string explaining what is sumec supposed to be doing
	Global_Code_Pos = "Enemy found!";
	Global_Code_Pos = "Enemy lost";
	Global_Code_Pos = "Rotating left";
	Global_Code_Pos = "Rotating right";
	Global_Code_Pos = "Found the edge, backing up";
	*/
	
}