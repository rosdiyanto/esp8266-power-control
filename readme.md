# ESP8266 Power Control

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Proyek ini adalah sistem kontrol daya berbasis ESP8266 yang memungkinkan Anda untuk menghidupkan dan mematikan perangkat dari jarak jauh melalui antarmuka web. Sistem ini dilengkapi dengan logging aktivitas, pembaruan status secara *real-time* melalui WebSocket, dan manajemen koneksi WiFi yang andal.

## Fitur Utama

-   **Kontrol Web**: Antarmuka web sederhana untuk menekan tombol "power" secara virtual.
-   **Status Real-time**: Status perangkat (ON/OFF) diperbarui secara instan di antarmuka web menggunakan WebSocket.
-   **Logging Komprehensif**: Semua aktivitas penting (boot, koneksi WiFi, perubahan status, aksi pengguna) dicatat ke dalam file `log.txt` di dalam sistem file LittleFS.
-   **Log Viewer**: Endpoint `/log` memungkinkan Anda melihat seluruh isi file log langsung dari browser.
-   **Manajemen Log**: Log akan otomatis dihapus dan dibuat ulang jika ukurannya melebihi 100KB untuk mencegah memori penuh.
-   **Manajemen Koneksi WiFi**: Perangkat akan secara otomatis mencoba menyambung kembali ke jaringan WiFi jika koneksi terputus, dengan interval waktu yang meningkat secara bertahap.
-   **Indikator LED**: LED bawaan (`LED_BUILTIN`) digunakan sebagai indikator status koneksi WiFi (Menyala jika terhubung, Mati jika tidak).

## Perangkat Keras yang Dibutuhkan

-   **Papan ESP8266**: Contohnya NodeMCU atau Wemos D1 Mini.
-   **Modul Relay**: Dihubungkan ke pin `D1` (GPIO 5) untuk mengontrol daya ke perangkat eksternal.
-   **Sensor Status**: Sebuah saklar atau sensor yang memberikan sinyal LOW saat perangkat "ON". Dihubungkan ke pin `D2` (GPIO 4).

### Pinout

| Pin ESP8266 | Fungsi                 | Keterangan                               |
| :---------- | :--------------------- | :--------------------------------------- |
| `D1` (GPIO 5) | **POWER_PIN**          | Output untuk mengontrol relay (HIGH-press, LOW-release). |
| `D2` (GPIO 4) | **STATUS_PIN**         | Input untuk membaca status perangkat (LOW = ON). |
| `LED_BUILTIN` | **LED_PIN**            | Indikator status koneksi WiFi (LOW = Connected). |

## Kebutuhan Software & Library

1.  **Arduino IDE**: Pastikan Anda sudah menginstal Arduino IDE.
2.  **ESP8266 Core**: Ikuti panduan ini untuk menambahkan board ESP8266 ke Arduino IDE Anda.
3.  **Library yang Dibutuhkan**:
    -   `ESP8266WebServer` (biasanya sudah termasuk dalam core)
    -   `ESP8266WiFi` (biasanya sudah termasuk dalam core)
    -   `WebSockets` oleh Markus Sattler (Install melalui Library Manager)
    -   `LittleFS` (biasanya sudah termasuk dalam core)
4.  **ESP8266 LittleFS Data Upload Tool**: Anda perlu menginstal plugin ini untuk mengunggah file ke memori flash ESP8266. Ikuti panduan instalasi dari repositori GitHub plugin.

## Instalasi dan Setup

1.  **Konfigurasi Kode**:
    -   Buka file `PowerControl.ino` di Arduino IDE.
    -   Ubah `ssid` dan `password` agar sesuai dengan jaringan WiFi Anda.

    ```cpp
    const char *ssid = "NamaWiFiAnda";
    const char *password = "PasswordWiFiAnda";
    ```

2.  **Siapkan File untuk LittleFS**:
    -   Buat folder bernama `data` di dalam direktori yang sama dengan file `.ino` Anda.
    -   Letakkan file antarmuka web Anda di dalam folder `data`, misalnya `index.html` dan `favicon.ico`.

3.  **Upload File ke LittleFS**:
    -   Di Arduino IDE, pilih board dan port yang benar.
    -   Klik `Tools` -> `ESP8266 Sketch Data Upload`. Ini akan mengunggah semua file dari folder `data` ke LittleFS di ESP8266.

4.  **Upload Kode Utama**:
    -   Klik tombol `Upload` untuk meng-flash program utama ke ESP8266 Anda.

5.  **Akses Perangkat**:
    -   Buka Serial Monitor untuk melihat alamat IP yang didapat oleh ESP8266.
    -   Buka browser dan navigasikan ke alamat IP tersebut untuk mengakses antarmuka kontrol.

## Dokumentasi API & WebSocket

### Endpoint HTTP

| Metode | Endpoint      | Deskripsi                                                              |
| :----- | :------------ | :--------------------------------------------------------------------- |
| `GET`  | `/`           | Menampilkan halaman utama (`index.html`).                              |
| `GET`  | `/press`      | Mensimulasikan penekanan tombol daya (mengirim sinyal HIGH).             |
| `GET`  | `/release`    | Mensimulasikan pelepasan tombol daya (mengirim sinyal LOW).              |
| `GET`  | `/status`     | Mengembalikan status perangkat saat ini dalam format JSON (`{"status":"ON"}` atau `{"status":"OFF"}`). |
| `GET`  | `/log`        | Menampilkan seluruh isi file `log.txt` sebagai `text/plain`.           |
| `GET`  | `/clearlog`   | Menghapus dan membuat ulang file `log.txt`.                            |
| `GET`  | `/favicon.ico`| Menyajikan ikon untuk tab browser.                                     |

### Event WebSocket

Server WebSocket berjalan di port `81`.

**Pesan dari Server ke Client:**

-   **Status Perangkat**: Dikirim saat status berubah atau saat klien baru terhubung.
    ```json
    {"status":"ON"}
    {"status":"OFF"}
    ```
-   **Pesan Log**: Dikirim setiap kali ada log baru yang dibuat.
    ```json
    {"log":"12345 | WIFI OK: 192.168.1.10"}
    ```
-   **Perintah Hapus Log**: Dikirim ke semua klien saat log dihapus.
    ```json
    {"clear":true}
    ```

## Lisensi

Proyek ini dilisensikan di bawah Lisensi MIT. Lihat file `LICENSE` untuk detailnya.
