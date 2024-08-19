#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"  // float ve int dönüşümleri için gerekli

// UART ayarları
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define BUFFER_SIZE 300
uint8_t GPS_Buffer[BUFFER_SIZE];

// Global değişkenler
char g_time[10], g_status, g_ns, g_ew, g_date[7];
float g_latitude, g_longitude, g_speed, g_hdop, g_altitude;
int g_fix_quality, g_num_satellites;
float accuracy;
float filtered_lat, filtered_long, filtered_speed, filtered_alti;

// *** KALMAN FİLTRESİ FONKSİYONLARI ******

typedef struct {
    double g_latitude;
    double g_longitude;
    double g_altitude;
    float g_speed;
    long timeStamp;
    float variance;
} KalmanFilter;

void KalmanFilter_init(KalmanFilter* filter) {
    filter->variance = -1;
}

void KalmanFilter_setState(KalmanFilter* filter, double latitude, double longitude, double altitude, float speed, long timeStamp, float accuracy) {
    filter->g_latitude = latitude;
    filter->g_longitude = longitude;
    filter->g_altitude = altitude;
    filter->g_speed = speed; // Hız verisi de eklenir
    filter->timeStamp = timeStamp;
    filter->variance = accuracy * accuracy;
}

void KalmanFilter_process(KalmanFilter* filter, float g_speed, double newLatitude, double newLongitude, double newAltitude, long newTimeStamp, float newAccuracy) {
    if (filter->variance < 0) {
        KalmanFilter_setState(filter, newLatitude, newLongitude, newAltitude, g_speed, newTimeStamp, newAccuracy);
    } else {
        long duration = newTimeStamp - filter->timeStamp;
        if (duration > 0) {
            filter->variance += duration * g_speed * g_speed / 1000;
            filter->timeStamp = newTimeStamp;
        }
        
        float k = filter->variance / (filter->variance + newAccuracy * newAccuracy);
        
        filter->g_latitude += k * (newLatitude - filter->g_latitude);
        filter->g_longitude += k * (newLongitude - filter->g_longitude);
        filter->g_altitude += k * (newAltitude - filter->g_altitude);
        filter->g_speed += k * (g_speed - filter->g_speed); // Hız verisi de filtreye eklenir

        filter->variance = (1 - k) * filter->variance;

        printf("%f,%f,%f,%f,%f\n",filter->g_longitude,filter->g_latitude,filter->g_altitude,filter->timeStamp,filter->g_speed);
    }
}

// ***** GPS FONKSİYONLARI *******

void parse_GNRMC(uint8_t *sentence) {
    char lat[11], lon[12];
    
    sscanf(sentence, "$GNRMC,%9[^,],%c,%10[^,],%c,%11[^,],%c,%f,,%6[^,]",
           g_time, &g_status, lat, &g_ns, lon, &g_ew, &g_speed, g_date);
    
    // Latitude ve Longitude'u doğru şekilde float'a çevirme
    int lat_deg = (int)(atof(lat) / 100);
    float lat_min = atof(lat) - (lat_deg * 100);
    g_latitude = lat_deg + (lat_min / 60.0);

    int lon_deg = (int)(atof(lon) / 100);
    float lon_min = atof(lon) - (lon_deg * 100);
    g_longitude = lon_deg + (lon_min / 60.0);

    // Kuzey/Güney ve Doğu/Batı koordinatlarına göre işaretleme
    if (g_ns == 'S') g_latitude = -g_latitude;
    if (g_ew == 'W') g_longitude = -g_longitude;

    g_speed = g_speed * 1.852; // Knotları km/saat'e çevirme
}

void parse_GNGGA(uint8_t *sentence) {
    char lat[11], lon[12];
    
    sscanf(sentence, "$GNGGA,%9[^,],%10[^,],%c,%11[^,],%c,%d,%d,%f,%f",
           g_time, lat, &g_ns, lon, &g_ew, &g_fix_quality, &g_num_satellites, &g_hdop, &g_altitude);
    
    // Latitude ve Longitude'u doğru şekilde float'a çevirme
    int lat_deg = (int)(atof(lat) / 100);
    float lat_min = atof(lat) - (lat_deg * 100);
    g_latitude = lat_deg + (lat_min / 60.0);

    int lon_deg = (int)(atof(lon) / 100);
    float lon_min = atof(lon) - (lon_deg * 100);
    g_longitude = lon_deg + (lon_min / 60.0);

    // Kuzey/Güney ve Doğu/Batı koordinatlarına göre işaretleme
    if (g_ns == 'S') g_latitude = -g_latitude;
    if (g_ew == 'W') g_longitude = -g_longitude;
}

void parse_GPS_data(uint8_t *buffer) {
    uint8_t *line = strtok(buffer, "\n");
    while (line != NULL) {
        if (strncmp(line, "$GNRMC", 6) == 0) {
            parse_GNRMC(line);
        } else if (strncmp(line, "$GNGGA", 6) == 0) {
            parse_GNGGA(line);
        }
        line = strtok(NULL, "\n");
    }
}

// GPS zamanını long veri tipine dönüştürme fonksiyonu
long convert_gps_time_to_long(const char* time_str) {
    char hours[3] = {time_str[0], time_str[1], '\0'};
    char minutes[3] = {time_str[2], time_str[3], '\0'};
    char seconds[3] = {time_str[4], time_str[5], '\0'};

    long time_in_seconds = atol(hours) * 3600 + atol(minutes) * 60 + atol(seconds);
    return time_in_seconds;
}

int main() {
    // GPIO ayarları
    stdio_init_all();
    
    // UART ayarları
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // UART formatını ayarlama (8 bit data, 1 stop bit, no parity)
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    KalmanFilter filter;
    KalmanFilter_init(&filter);
    accuracy = 8.0; 

    sleep_ms(10000);

    while (true) {
        uart_read_blocking(UART_ID, GPS_Buffer, BUFFER_SIZE);
        parse_GPS_data(GPS_Buffer);

        // GPS zamanını long veri tipine dönüştür
        long gps_time_long = convert_gps_time_to_long(g_time);

        // Kalman filtresini işleme
        KalmanFilter_setState(&filter, g_latitude, g_longitude, g_altitude, g_speed, gps_time_long, accuracy);
        KalmanFilter_process(&filter, g_speed, g_latitude, g_longitude, g_altitude, gps_time_long, accuracy);
    }
}