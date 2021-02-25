void Reley_init(){
  
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  
   }
   
// Включаем реле с web страницы /pult.htm
void handle_Reley() {
  int state = HTTP.arg("state").toInt();
  Reley(state);
  Serial.println(state);
 
  }
// Включаем реле
void Reley(int state) {
  switch (state) {
     
    case 3:
       digitalWrite(rele2, LOW);
     break;
    case 2:
       digitalWrite(rele2, HIGH);
     break;
    case 1:
      digitalWrite(rele1, LOW);
      break;
    case 0:
      digitalWrite(rele1, HIGH);
      break;
  }
  
}


