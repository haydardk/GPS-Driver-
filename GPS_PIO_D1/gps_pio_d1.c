#include "stdint.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "string.h"

#include "gps_pio_d1.h"
#include "gps_pio_d1.pio" 

char GGA_Buffer[100];
char RMC_Buffer[100];
int hour;         // GGA VERİERİ
float latitude;
char NELatitude;
float longitude;
char WElongitude;
int quantity_gps;  
float height ; 
float velocity;  // RMC VERİLERİ
float rotation;
int date;
void setup_gps(){
    setup_default_uart();
    printf("Starting PIO GPS UART RX : \n");
    // UART kurulumu
    uart_init(HARD_UART_INST, GPS_BAUD_RATE);
    // PIO kurulumu , sm=state machine 
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &gps_pio_d1_program);
    gps_pio_d1_program_init(pio, sm, offset, PIO_RX_PIN, GPS_BAUD_RATE); 
}
void read_gps(){
    char temp_buf[5];
    for (int i=0; i<5 ; i++){
        temp_buf[i]=gps_pio_d1_program_getc(pio,sm);
    }
    if(strcmp(temp_buf,"GPGGA")==0){
        for(int i=0 ; i<100 ; i++){
        GGA_Buffer[i]=gps_pio_d1_program_getc(pio,sm);
        if(strcmp(GGA_Buffer[i],"*")==0){
            i=100;
            }
        }
    }else if (strcmp(temp_buf,"GPRMC")==0){
        for(int i=0 ; i<100 ; i++){
        RMC_Buffer[i]=gps_pio_d1_program_getc(pio,sm);
        if(strcmp(RMC_Buffer[i],"*")==0){
            i=100;
            }
        }
    }
    else {
        read_gps();
    }  
}

//  ***   PARSELLEME FONKSİYONLARI  ***  //     

void GGA_parcelization(uint8_t buffer[100]){ // GGA_parcelization(GGA_Buffer)
    int comma[14];
    int indx=-1; // Buffer'daki virgüllerin indexleri belirleniyor
    for (int i=0;i<100;i++){  
        if (strcmp(buffer[i],",")==0){
            indx++;
            comma[indx]=i;
        }
    }
    // Saat verisi parselleniyor
    char temp_hour[6]; 
    for (int i=comma[0]+1;i<comma[1];i++){
        temp_hour[i-(comma[0]+1)]=buffer[i];
    }
    hour=atoi(temp_hour);
    // Enlem verisi parselleniyor  
    char temp_latitude[8];
    for (int i=comma[1]+1;i<comma[2];i++){
            temp_latitude[i-(comma[1]+1)]=buffer[i];
    }
    latitude=atof(temp_latitude);
    NELatitude = buffer[(comma[2]+1)];
    // Boylam verisi parselleniyor
    char temp_longitude[9];
    for (int i=comma[3]+1;i<comma[4];i++){
            temp_longitude[i-(comma[3]+1)]=buffer[i];
    }
    longitude=atof(temp_longitude);
    WElongitude = buffer[(comma[4]+1)];
    // GPS sayısı parselleniyor
    char temp_qgps[2];
    for (int i=comma[6]+1;i<comma[7];i++){
        temp_qgps[i-(comma[6]+1)]=buffer[i];
    }
    quantity_gps=atoi(temp_qgps);
    // Yerden yükseklik parselleniyor
    char temp_altitude[6]; //RAKIM
    char temp_geoid[5];  //Geoid 
    for (int i=comma[8]+1;i<comma[9];i++){
        temp_altitude[i-(comma[8]+1)]=buffer[i];    
    }
    float temp_alt = atof(temp_altitude);
    for (int i=comma[10]+1;i<comma[11];i++){
       temp_geoid[i-(comma[10]+1)]=buffer[i];
    }
    float temp_geo = atof(temp_geoid);
    height = temp_alt - temp_geo ; // Yerden yükseklik hesaplandı.
    // GGA PARSELLEME İŞLEMİ BİTTİ
}

void RMC_parcelization(uint8_t buffer[100]){ // RMC_parcelization(RMC_Buffer)
    int comma[11];
    int indx=-1;
    for(int i=0;i<100;i++){  // Buffer'daki virgüllerin indexleri belirleniyor.
        if(strcmp(buffer[i],",")==0){
            indx++
            comma[indx]=i;
        }
    }
    // Hız verisi parselleniyor 
    char temp_velo[5];
    for (int i=comma[6]+1;i<comma[7];i++){
        temp_velo[i-(comma[6]+1)]=buffer[i];
    }
    float velo_knot = atof(temp_velo);
    velocity = 1.852*velo_knot // Hız knot biriminden km/h'e çevrildi
    // Rotasyon verisi parselleniyor
    char temp_rota[5];
    for (int i=comma[7]+1;i<comma[8];i++){
        temp_rota[i-(comma[7]+1)]= buffer[i];
    }
    rotation = atof(temp_rota);
    // Tarih verisi parselleniyor 
    char temp_date[6];
    for (int i=comma[8]+1;i<comma[9];i++){
        temp_date[i-(comma[8]+1)] = buffer[i];
    }
    date = atoi(temp_date);
    // RMC PARSELLEME İŞLEMİ BİTTİ 
}
