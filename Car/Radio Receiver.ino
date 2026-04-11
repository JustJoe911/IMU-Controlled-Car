/*connection :
vcc::3.3v
gnd
SPI communication:: MOSI , SCK ,MISO (for uno /nano ):: 11,13,12
CSN ,CE ::any pins 
*/

/*it is important to know the type and length of the data
it can send any type but it has to be of fixed size of max 32 bytes and doesn't change 
for example int32 instead of int  */

/*it sends data at an address of 5 bytes having the same address at both the receiver and transmitter*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE 7
#define CNS 8

RF24 radio(CE, CNS); 
const byte address[6] = "00001";


void setup() {

  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address); //in case of reading 
  /*in case of writing 
  radio.openWritingPipe(address);
  //it can read and write at the same time but at diff addresses
  */
  radio.setPALevel(RF24_PA_MAX);
  /*power levels :
  RF24_PA_MIN -> very close
  RF24_PA_LOW -> same room 
  RF24_PA_HIGH -> diff rooms
  RF24_PA_MAX ->long dist
  */
  radio.startListening(); //in case of reading 
  /*in case of writing :
  radio.stopListening();
  */

}

void loop() {

//in case of reading 
  if (radio.available()) {
      char text[32] = "";
      radio.read(&text, sizeof(text));
      Serial.println(text);
    }

/*in case of writing :
  const char text[] = "Hello World";
  radio.write(&text, sizeof(text));
  delay(1000);
*/
}


