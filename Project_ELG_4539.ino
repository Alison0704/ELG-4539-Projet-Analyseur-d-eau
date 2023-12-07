// Inclusion des Libraries 
  
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #include <Wire.h>

// Broches des capteurs
  
  #define PIN_CAPTEUR_TEMP A0
  #define PIN_CAPTEUR_PH A1
  #define PIN_CAPTEUR_TDS A2

 // Broches des LEDs
    
    #define ledVerteTemp 2  // temperature
    #define ledRougeTemp 3  // temperature
    #define ledVertepH 4    // pH
    #define ledRougepH 5    // pH
    #define ledVerteTDS 6   // TDS
    #define ledRougeTDS 7   // TDS
    #define buzzer 8        //activation du buzzer active

  //Composant du lED Global 
    #define BLEU 9

// Définir les constants

  //TDS
  #define VREF 5.0 // analog reference voltage(Volt) of the ADC
  #define SCOUNT 30 // sum of sample point

  //pH
  #define CALIBRATION_VALUE_PH 28.34
  #define CALIBRATION_ADJUSTMENT_DIFF -1.12

// Configuration de l'installation pour la temperature
  
  OneWire oneWire(PIN_CAPTEUR_TEMP);
  DallasTemperature sensors(&oneWire);

// Déclaration

    int satisfied_count; 
    // le satisfied_count nous donnera une valeur entre 0 et 3 nous indiquant l'état global de notre analyseur d'eau
    // 3 - 3 parametres satisfaitent notre standard de la qualité de l'eau
    // 2 - 2 parametres satisfaitent notre standard de la qualité de l'eau
    // 1 - 1 parametres satisfaitent notre standard de la qualité de l'eau
    // 0 - 0 parametres satisfaitent notre standard de la qualité de l'eau

  //Pour le TDS

      int analogBuffer[SCOUNT];     //stockage de valeur analogique dans un tableau, lu depuis le ADC
      int analogBufferTemp[SCOUNT]; //une version temporaire
      int analogBufferIndex = 0;    //l'index nous permettant de traverser le tableau
      int copyIndex = 0;            

      float averageVoltage = 0;
      float tdsValue = 0;

  //calibration du pH

    float calibration_value = CALIBRATION_VALUE_PH - CALIBRATION_ADJUSTMENT_DIFF - 1.22-8+2 ;
    int phval = 0; 
    unsigned long int avgval; 
    int buffer_arr[10],temp;
    float ph_act;
  
// Algorithme de filtrage médian - pour le TDS
  int getMedianNum(int bArray[], int iFilterLen){
    int bTab[iFilterLen];
    for (byte i = 0; i<iFilterLen; i++)
    bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++) {
      for (i = 0; i < iFilterLen - j - 1; i++) {
        if (bTab[i] > bTab[i + 1]) {
          bTemp = bTab[i];
          bTab[i] = bTab[i + 1];
          bTab[i + 1] = bTemp;
        }
      }
    }
    if ((iFilterLen & 1) > 0){
      bTemp = bTab[(iFilterLen - 1) / 2];
    }
    else {
      bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    }
    return bTemp;
  }

// SECTION CONFIGURATION
  void setup() 
    {
      Serial.begin(115200); //initialisation du port serie
      Wire.begin();
      pinMode(ledVerteTemp, OUTPUT);
      pinMode(ledRougeTemp, OUTPUT);
      pinMode(ledVertepH, OUTPUT);
      pinMode(ledRougepH, OUTPUT);
      pinMode(ledVerteTDS, OUTPUT);
      pinMode(ledRougeTDS, OUTPUT);  
      pinMode(buzzer, OUTPUT);
      pinMode(BLEU, OUTPUT);

      pinMode(PIN_CAPTEUR_TEMP, INPUT);
      pinMode(PIN_CAPTEUR_PH, INPUT);
      pinMode(PIN_CAPTEUR_TDS, INPUT);

      sensors.begin();
    }
// SECTION PRINCIPALE DE PROGRAMMATION ACTIVE

  void loop() {
  int satisfied_count_final;
  //initialisation des valeurs numerique des paramètres 
  float temperature = 0;
  float pH = 0;
  float tds= 0;

  // Lecture des valeurs des capteurs
  temperature = lireTemperature();
  pH = lirePH();
  tds = lireTDS(lireTemperature());

  // Mise à jour des LEDs pour chaque capteur
  satisfied_count_final = miseAJourLEDs(temperature,pH,tds);

  //Identification des LED qui sont verts - Pour avoir l'état global de la qualité de l'eau
  if(satisfied_count_final == 3)
  {
    digitalWrite(buzzer, HIGH);
    digitalWrite(BLEU,HIGH);
  }
  else
  {  
    digitalWrite(buzzer, LOW);
    digitalWrite(BLEU,LOW);
  }
  }
  
// Fonctions - Lecture des capteur
  //Temperature
    float lireTemperature() 
      {
        // Remplacer par votre logique de lecture de température
          sensors.requestTemperatures(); // Send the command to get temperatures
          float tempC = sensors.getTempCByIndex(0);

          // Check if reading was successful
          if(tempC != DEVICE_DISCONNECTED_C) 
        {
          return tempC;
        } 
          else
        {
          tempC = -273.15;
        }
      }
  //pH
    float lirePH() 
    {
      //Stockage the 10 valeurs d'échantillon afin de faire la moyenne après
      for(int i=0;i<10;i++) 
      { 
        buffer_arr[i]=analogRead(A1);
        delay(30);
      }
      //Faire le tri des valeurs d'échantillon pour facilité la calculation de la moyenne après
      for(int i=0;i<9;i++)
      {
        for(int j=i+1;j<10;j++)
        {
          if(buffer_arr[i]>buffer_arr[j])
            {
              temp=buffer_arr[i];
              buffer_arr[i]=buffer_arr[j];
              buffer_arr[j]=temp;
            }
        }
      }
      //la calculation de la moyenne
      avgval=0;
      for(int i=2;i<8;i++)
      {
        avgval+=buffer_arr[i];
      }
      float volt=(float)avgval* 5.0/1024/6;       //convertit l'analogique en millivolt
      ph_act =  -5.70 * volt + calibration_value;    //convertit le millivolt en valeur de pH
    
      return ph_act;
      }
  //TDS  
    float lireTDS(float temperature)
      {
      static unsigned long analogSampleTimepoint = millis();
        if(millis()-analogSampleTimepoint > 40U) //toutes les 40 millisecondes, lisez la valeur analogique de l'ADC 
        {     
          analogSampleTimepoint = millis();
          analogBuffer[analogBufferIndex] = analogRead(PIN_CAPTEUR_TDS);    //lire la valeur analogique et la stocker dans le tampon
          analogBufferIndex++;
          if(analogBufferIndex == SCOUNT)
          { 
            //Réinitialisation du tableau pour ne pas dépasser 30 valeurs
            analogBufferIndex = 0;
          }
        }   
      
      static unsigned long printTimepoint = millis();
      if(millis()-printTimepoint > 800U)
      {
        printTimepoint = millis();
        for(copyIndex=0; copyIndex<SCOUNT; copyIndex++)
        {
          analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
          
          // lecture de la valeur analogique plus stable grâce à l'algorithme de filtrage médian et convertit en valeur de tension
          averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
          
          //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
          float compensationCoefficient = 1.0+0.02*(temperature-25.0);
          //temperature compensation
          float compensationVoltage=averageVoltage/compensationCoefficient;
          
          float prevtdsValue = tdsValue;
          
          //convertit la valeur de tension en valeur tds
          tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
        
        }
      }
      return tdsValue;
      }
// Mise à jour des signaux LED
  int miseAJourLEDs(float temperature, float pH, float tds) 
    {
    // Mise à jour de la LED pour la température
    int satisfied_count = 0;
    //Serial.println("------------------------");
    if (temperature >= 10 && temperature <= 25) {
      digitalWrite(ledVerteTemp, HIGH);
      digitalWrite(ledRougeTemp, LOW);
      //Serial.print("Good Temp: ");
      Serial.print(temperature,2);
      Serial.print(",");
      satisfied_count += 1;
    } else { 
      digitalWrite(ledVerteTemp, LOW);
      digitalWrite(ledRougeTemp, HIGH);
      //Serial.print("Bad Temp: ");
      Serial.print(temperature,2);
      Serial.print(",");
    }
    
    // Mise à jour de la LED pour le pH
    if (pH >= 6.5 && pH <= 8.5) {
      digitalWrite(ledVertepH , HIGH);
      digitalWrite(ledRougepH, LOW);
      //Serial.print("Good pH: ");
      Serial.print(pH);
      Serial.print(",");
      satisfied_count += 1;
    }
    else {
      digitalWrite(ledVertepH, LOW);
      digitalWrite(ledRougepH, HIGH);
      //Serial.print("Bad pH: ");
      Serial.print(pH);
      Serial.print(",");

    }

    // Mise à jour de la LED pour le TDS
    if (tds >= 50 && tds <= 250) {
      digitalWrite(ledVerteTDS, HIGH);
      digitalWrite(ledRougeTDS, LOW);
      //Serial.print("Good TDS: ");
      Serial.print(tds,2);
      Serial.print(",");
      satisfied_count += 1;
    } else {
      digitalWrite(ledVerteTDS, LOW);
      digitalWrite(ledRougeTDS, HIGH);
      //Serial.print("Bad TDS: ");
      Serial.print(tds,2);
      Serial.print(",");
    }
    Serial.print(satisfied_count);
    Serial.print(",");
    Serial.println();
    return satisfied_count;
    }