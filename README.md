# 🌡️ STM32 Akıllı Ortam İzleme ve Kontrol Sistemi

**Hazırlayan:** Furkan Yatır
**Öğrenci Numarası:** 25221303001  
**Ders:** Gömülü Sistemler 
**Üniversite:** İstanbul Topkapı Üniversitesi  

---

Bu proje; ADC, I2C, SPI, PWM ve UART protokollerini tek bir sistemde birleştiren, sıcaklık ve hareket verilerine göre iklimlendirme kontrolü yapan bir gömülü sistem projesidir.

---

## 📊 Sistem Mimarisi ve Blok Diyagramı

![Sistem Şeması](diyagram.png)

> [!IMPORTANT]
> **[🌐 İnteraktif Sistem Diyagramını Görüntüle](https://htmlpreview.github.io/?https://github.com/furkan-yatr/STM32-Akilli-Ortam-Izleme-Sistemi_Odev5/blob/main/stm32_blok_diyagrami.html)**
> *(Bileşenlerin üzerine tıklayarak teknik detayları ve örnek HAL kodlarını inceleyebilirsiniz.)*

---

## 🛠️ Donanım ve Bağlantı Haritası

| Bileşen | Protokol | Pin Yapılandırması | Görevi |
| :--- | :--- | :--- | :--- |
| **NTC Sensör** | ADC | `PA0 (ADC1_IN0)` | Hassas Sıcaklık Ölçümü |
| **MPU6050** | I2C | `PB6 (SCL) / PB7 (SDA)` | Hareket ve İvme Takibi |
| **OLED Ekran** | SPI | `PA4-PA7 (SPI1)` | Veri Görselleştirme |
| **Fan (DC Motor)** | PWM | `PB4 (TIM3_CH1)` | Kademeli Hız Kontrolü |
| **PC Terminal** | UART | `PA2 (TX) / PA3 (RX)` | Veri Loglama |

---

## ⚠️ Mühendislik Notu: CubeIDE ve Wokwi Farklılıkları
Teknik Not: CubeIDE ve Wokwi Arasındaki FarklılıklarProjenin geliştirme sürecinde, hedef donanım (STM32F407VG) ile simülasyon ortamı (Wokwi) arasındaki yapısal farklardan dolayı iki ayrı kod kümesi oluşturulmuştur. Simülasyonda karşılaşılan kısıtlamalar ve uygulanan mühendislik çözümleri aşağıdadır:  İşlemci Portu (MCU): Ödev isterlerine göre teorik tasarım STM32F407VG için yapılmıştır. Ancak Wokwi'nin bu kartı tam desteklememesi nedeniyle simülasyon, donanımsal olarak benzer mimariye sahip Nucleo-L031 üzerine kurulmuştur.  Kesme (Interrupt) Yönetimi: CubeIDE kodunda sistem modu EXTI (Dış Kesme) ve periyodik okumalar Timer kesmeleri ile yönetilmektedir. Simülasyon ortamında yaşanabilecek "Multiple Definition" hatalarını önlemek için Wokwi kodunda HAL_GetTick() tabanlı, işlemciyi bloke etmeyen asenkron bir yapı tercih edilmiştir.  Analog Veri İşleme (ADC): Wokwi'deki NTC sensörünün karakteristik yapısı gereği, standart doğrusal hesaplamalar yerine math.h kütüphanesi kullanılarak logaritmik BETA (3950) formülü entegre edilmiş; böylece hassas sıcaklık okumaları elde edilmiştir.  UART Veri Formatı: Wokwi terminalinin float yazdırma kısıtlaması, sıcaklık verisinin tam sayı ve ondalık kısımlara bölünerek gönderilmesiyle (integer parsing) aşılmıştır.  

Simülasyon ortamının (Wokwi) teknik kısıtlamaları nedeniyle, simülasyon kodunda (`src_wokwi`) şu optimizasyonlar yapılmıştır:

1. **İşlemci Portu:** F407VG desteği kısıtlı olduğundan simülasyon **Nucleo-L031** üzerine kurulmuş, ancak kodlar taşınabilir HAL mimarisiyle yazılmıştır.
2. **Kesme Yönetimi:** Donanımsal kesme çakışmalarını önlemek için `HAL_GetTick()` tabanlı asenkron yapı tercih edilmiştir.
3. **NTC Matematiği:** Simülasyondaki parazitli okumaları gidermek için `math.h` kütüphanesi ile logaritmik **BETA (3950)** formülü entegre edilmiştir.

---

## 🔗 Hızlı Bağlantılar
* **Wokwi Canlı Simülasyon:** [(https://wokwi.com/projects/367244067477216257)]
* **Teorik Kodlar (F407VG):** [./src_cubeide/main.c](./src_cubeide/main.c)
* **Çalışan Simülasyon Kodları:** [./src_wokwi/main.c](./src_wokwi/main.c)
