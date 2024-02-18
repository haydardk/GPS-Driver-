
#define GPS_BAUD_RATE 9600
#define HARD_UART_INST uart1

#define PIO_RX_PIN 3

void setup_gps(void);
void read_gps(uint8_t buffer[100]);
void GGA_parcelization(uint8_t buffer[100]);
void RMC_parcelization(uint8_t buffer[100]);
// ÖRNEK BİR GPS MESAJ DİZİMİ : 
//$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47 
/* $GPGGA: Başlık (Header)
123519: Saat bilgisi (12:35:19 UTC)
4807.038: Enlem (48 derece 07.038 dakika)
N: Kuzey yönü
01131.000: Boylam (11 derece 31.000 dakika)
E: Doğu yönü
1: GPS sinyali alındı
08: Kullanılan GPS uydu sayısı
0.9: HDOP (yatay doğruluk)
545.4,M: Deniz seviyesinden yükseklik
46.9,M: Geoid yüksekliği
*47: Checksum (kontrol toplamı) */

// ÖRNEK BİR RMC DİZİNİ :
/*$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
Saat: 12:35:19 UTC
Veri geçerli (A)
Enlem: 48 derece 07.038 dakika kuzey
Boylam: 11 derece 31.000 dakika doğu
Hız: 022.4 knots
Gerçek rotasyon: 084.4 derece
Tarih: 23 Mart 1994 */

