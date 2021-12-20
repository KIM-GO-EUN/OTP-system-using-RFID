
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10    // spi 통신을 위한 SS(chip select)핀 설정
#define RST_PIN 9    // 리셋 핀 설정
 
MFRC522 rfid(SS_PIN, RST_PIN); // 'rfid' 이름으로 클래스 객체 선언
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte nuidPICC[4];   // 카드 ID들을 저장(비교)하기 위한 배열(변수)선언

int len = 0, i = 0, j = 0; // OTP 생성을 위한 변수
int Feedback = 0, output = 0;

void setup() 
{ 
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("OTP...");
  
  SPI.begin(); // SPI 통신 시작
  rfid.PCD_Init(); // RFID(MFRC522) 초기화 
  // ID값 초기화  
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
// MIFARE 타입의 카드키 종류들만 인식됨을 표시 
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void LFSR_128bit() //8자리 OTP 생성기
{
  
  // 초기화
  len = 128; // 길이
  String binary=""; // 출력할 바이너리 코드 저장 공간
  String Dec="";
  int lfsr[128]; // LFSR값을 저장할 비트
  
  // 랜덤으로 128비트를 채운다.
  for(int i=0; i<len-1; i++)
  {
    lfsr[i]=random(0, 1);
  }
  lfsr[127]=1;  
  int bTemp[128];
  
  for (i = 0; i < len; ++i) 
  {
    bTemp[i] = lfsr[i];
  }
  
  
  int rand = random(128, 200); // 시프트 횟수
  
  lcd.clear();
  lcd.print("wait...");
  Serial.println("계산중...");
  for (int count = 1; count<=rand; ++count) // 128이상 200이하의 루프를 돈다.
  {
    binary="";
    for(int i=2;i<54;i++) // 짝수번째 비트 선별
    {
      if(i%2==0)
      {
        binary+=(String)bTemp[i];
      }
    }
  
    //Serial.print("binary: "); //골라낸 26개의 비트 출력(보안을위해서 삭제가능)
    //Serial.print(binary);
    //Serial.println();
    
    double decimal = 0;
    String bits="";
    int p = 0;
    int index=25; // 26개의 비트
    
    while(true)
    {
      if(index == -2)
      {
        break;
      } 
      
      else 
      {
        bits = binary[index];
        int temp = bits.toInt();
        decimal += (temp * pow(2, p));
        p++;
        index--;
      }
    }
    Dec= String(floor(decimal)); //소수점 제거
    //Serial.print(Dec.toInt());
    //Serial.println("계산중...");
      
    // Shift 연산 [128 39 6 3 2 1 0]
    Feedback = (bTemp[127] + bTemp[38]+ bTemp[5]+ bTemp[2] + bTemp[1] + bTemp[0] + 1) % 2;
    output = bTemp[127];
    for (i = len - 1; i > 0; --i) 
    {
      bTemp[i] = bTemp[i - 1];
    }
  

    // add Feedback
    bTemp[0] = Feedback;
   
    // 검사
    for (i = 0; i < len; ++i) 
    {
      if (bTemp[i] != lfsr[i]) 
      {
        break;
      }
    } 
  }
  Serial.println("------------------------------");
  
  //8자리 OTP생성
  Serial.print("OTP: ");
  String lcdprint="";
  for(int i=0;i<8;i++)
  {
    lcdprint+=Dec[i];
  }

  if(lcdprint[7]=='.')
  {
    lcdprint[7]='0';
  }
  Serial.print("OTP "+lcdprint); //OTP 출력
  Serial.println();
  lcd.clear();
  lcd.print(lcdprint); // OTP lcd에 출력
}

void loop() 
{
  // 새카드 접촉이 있을 때만 다음 단계로 넘어감
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // 카드 읽힘이 제대로 되면 다음으로 넘어감
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  // 현재 접촉 되는 카드 타입을 읽어와 모니터에 표시함
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // MIFARE 방식의 카드인지 확인 루틴
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // 이전 인식된 카드와 다른, 혹은 새카드가 인식되면 
  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) 
  {
    Serial.println(F("새로운 카드"));
    lcd.clear();
    lcd.print("NEW card");

    // 고유아이디(UID) 값을 저장한다.
    for (byte i = 0; i < 4; i++) 
    {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    // 그 UID 값을 16진값으로 출력 한다. 
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    // 그 UID 값을 10진값으로 출력 한다. 
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } 
  
  // 이미 등록된 카드의 경우, 메시지와 함께 OTP 출력.
  else 
  {
    Serial.println(F("등록된 카드 입니다."));
    LFSR_128bit();
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// 16진 값으로 변환 해주는 함수 정의
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
// 10진 값으로 변환 해주는 함수 정의
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
