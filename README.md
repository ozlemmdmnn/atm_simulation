# 🏧 ATM Simülasyonu (C ile Modüler ATM Projesi)

Saf C dilinde, dış kütüphane bağımlılığı olmadan yazılmış, dosya tabanlı veri
kalıcılığına sahip, modüler mimarili bir ATM simülasyonu. Bellek güvenliği
(AddressSanitizer/UBSan ile test edilmiştir), güvenli girdi işleme ve
gerçekçi bankacılık kuralları (günlük limit, kart bloke, faiz simülasyonu,
QR ile çekim) içerir.

## ✨ Özellikler

### Temel Özellikler
- 👤 Hesap oluşturma ve PIN ile güvenli giriş
- 💰 Para yatırma / çekme
- 🔁 Hesaplar arası havale / EFT
- 📊 Bakiye görüntüleme
- 🔑 PIN (şifre) değiştirme
- 🧾 Son işlemler (işlem geçmişi) görüntüleme
- 🛠️ Şifre korumalı Admin Paneli (tüm hesapları listeleme, hesap silme, kilit açma)

### İleri Seviye Özellikler
- 🚦 **Günlük para çekme limiti** (varsayılan: 5.000 TL, tarih değişince otomatik sıfırlanır)
- 🔒 **Kart bloke sistemi**: 3 hatalı PIN girişinde hesap otomatik kilitlenir, yalnızca admin açabilir
- 📈 **Vadeli hesap / faiz simülasyonu**: girilen ay sayısına göre bileşik faiz projeksiyonu (gerçek bakiyeyi etkilemez)
- 📱 **QR ile para çekme simülasyonu**: terminalde ASCII QR kodu çizilir, rastgele doğrulama kodu ile "telefondan onay" akışı simüle edilir

## 🧱 Mimari

Proje tek bir `main.c` dosyasına sıkıştırılmamıştır; sorumluluklar ayrı
modüllere bölünmüştür (Single Responsibility Principle):

```
atm_simulation/
├── include/
│   └── atm.h            # Tüm struct'lar, sabitler ve fonksiyon prototipleri
├── src/
│   ├── main.c            # Program giriş noktası, menü akışı
│   ├── auth.c             # Hesap oluşturma, giriş, PIN değiştirme
│   ├── account.c          # Yatırma, çekme, havale, faiz, QR simülasyonu
│   ├── transaction.c      # İşlem geçmişi (dairesel tampon) yönetimi
│   ├── admin.c            # Admin paneli (listele / sil / kilit aç)
│   ├── file_utils.c       # Binary dosya okuma/yazma (persistence)
│   └── utils.c            # Güvenli girdi okuma, hashleme, tarih yardımcıları
├── data/                  # Çalışma zamanında oluşan veri dosyaları (git'e dahil değil)
├── Makefile
└── README.md
```

### Tasarım Kararları

| Konu | Yaklaşım | Neden |
|---|---|---|
| Bellek yönetimi | `malloc`/`free` **kullanılmaz**; tüm hesaplar sabit boyutlu `Bank` struct'ında (statik ömürlü) tutulur | Bellek sızıntısı riskini kökten ortadan kaldırır, `Account` struct'ı doğrudan `fwrite`/`fread` ile diske yazılabilir |
| Girdi okuma | `scanf("%d")` yerine `fgets` + `strtol`/`strtod` | Harf girildiğinde sonsuz döngüye girme / tampon taşması riskini önler |
| PIN saklama | Düz metin yerine basit hash (djb2) ile saklanır | Dosyada PIN'in doğrudan okunabilir olmasını engeller (bkz. Güvenlik notu) |
| İşlem geçmişi | Her hesapta sabit boyutlu (`MAX_HISTORY=10`) dairesel tampon | Sabit ve öngörülebilir bellek kullanımı, dinamik liste gerektirmez |
| Hesap silme | Soft-delete (`is_active = 0`) | Hesap numarası tutarlılığı ve denetim izi (audit trail) korunur |

> ⚠️ **Güvenlik Notu**: Bu proje bir **portfolyo/eğitim** çalışmasıdır. PIN
> hash'lemesi için kullanılan djb2 algoritması kriptografik olarak güvenli
> değildir ve tuzlama (salting) içermez. Gerçek bir bankacılık sisteminde
> `bcrypt`, `Argon2` gibi algoritmalar ve TLS üzerinden iletişim
> kullanılmalıdır.

## 🚀 Kurulum ve Çalıştırma

### Gereksinimler
- GCC (veya C11 destekleyen herhangi bir derleyici)
- `make`
- POSIX uyumlu bir sistem (Linux/macOS) — PIN gizleme (`termios`) için. Windows'ta
  WSL veya MinGW ile de çalışır; termios bulunmazsa PIN görünür şekilde okunur.

### Derleme
```bash
make
```

### Çalıştırma
```bash
make run
# veya
./bin/atm_sim
```

### Debug / Bellek Testi (AddressSanitizer + UBSan)
```bash
make debug
./bin/atm_sim
```

### Temizlik
```bash
make clean        # derleme çıktılarını siler
make clean-data   # kayıtlı hesap verilerini de siler (DİKKAT: geri alınamaz)
```

## 🔑 Admin Paneli

Ana menüden `3` seçilerek erişilir. Varsayılan şifre: `admin123`
(bkz. `include/atm.h` → `ADMIN_PASSWORD`, dilerseniz değiştirebilirsiniz).

## 📸 Örnek Kullanım Akışı

```
1. Ana menüden "Yeni Hesap Oluştur" ile hesap açın, hesap numaranızı not edin.
2. "Giriş Yap" ile hesap numaranız ve PIN'inizle oturum açın.
3. Para yatırın, çekin, havale gönderin veya QR ile çekim yapın.
4. "İşlem Geçmişini Görüntüle" ile son işlemlerinizi kontrol edin.
```

## 🗂️ Veri Kalıcılığı

Hesap verileri `data/accounts.dat` dosyasında **binary** formatta saklanır
(struct'lar doğrudan `fwrite`/`fread` ile yazılır/okunur). Admin işlemleri
ise okunabilir bir denetim (audit) günlüğü olarak `data/admin_actions.log`
dosyasına metin biçiminde eklenir.

## 📄 Lisans

Bu proje eğitim ve portfolyo amaçlı olarak MIT lisansı ile paylaşılabilir.
