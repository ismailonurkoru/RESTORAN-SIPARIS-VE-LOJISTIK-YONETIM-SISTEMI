#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Performans testi için sabitler ve global değişkenler
#define HASH_SIZE 10007
#define MAX_MUTFAK_KAPASITESI 100
#define DURAK_SAYISI 5 //Restoran ve 4 mahalle arasındaki ağ için

int global_fis_id = 1000; //Her hesap kapatıldıpında artacak fiş numarası


// 1. VERİ KATMANI VE HASH TABLE (ÜRÜNLER İÇİN)
typedef struct Urun {
    int id;
    char isim[50];
    float fiyat;
    int stok;
    struct Urun *next;
} Urun;

Urun *urun_tablosu[HASH_SIZE];

int hash_fonksiyonu(int id) { return id % HASH_SIZE; }

void urun_ekle(int id, const char *isim, float fiyat, int stok) {
    int index = hash_fonksiyonu(id);
    Urun *yeni_urun = (Urun *) malloc(sizeof(Urun));
    yeni_urun->id = id;
    strcpy(yeni_urun->isim, isim);
    yeni_urun->fiyat = fiyat;
    yeni_urun->stok = stok;
    yeni_urun->next = urun_tablosu[index]; //Yeni ürünü listenin başına ekliyoruz.
    urun_tablosu[index] = yeni_urun;
}

Urun *urun_bul(int id) {
    int index = hash_fonksiyonu(id);
    Urun *current = urun_tablosu[index];
    while (current != NULL) {
        if (current->id == id) return current;
        current = current->next;
    }
    return NULL;
}

// 2. AĞAÇ YAPISI (MENÜ HİYERARŞİSİ İÇİN)
typedef struct MenuNode {
    char isim[100];
    struct MenuNode *first_child; //ilk alt kategori
    struct MenuNode *next_sibling; //aynı kategorideki diğer kategori
} MenuNode;

MenuNode *dugum_olustur(const char *isim) {
    MenuNode *yeni_dugum = (MenuNode *) malloc(sizeof(MenuNode));
    strcpy(yeni_dugum->isim, isim);
    yeni_dugum->first_child = NULL;
    yeni_dugum->next_sibling = NULL;
    return yeni_dugum;
}

void alt_dugum_ekle(MenuNode *ebeveyn, MenuNode *cocuk) {
    if (ebeveyn->first_child == NULL) ebeveyn->first_child = cocuk;
    else {
        MenuNode *temp = ebeveyn->first_child;
        while (temp->next_sibling != NULL) temp = temp->next_sibling;
        temp->next_sibling = cocuk;
    }
}

// Menüyü girintili bir şekilde yazdırıyoruz.
void menuyu_yazdir(MenuNode *root, int derinlik) {
    if (root == NULL) return;
    for (int i = 0; i < derinlik; i++) printf("   ");
    if (derinlik > 0) printf("|-- ");
    printf("%s\n", root->isim);
    menuyu_yazdir(root->first_child, derinlik + 1);
    menuyu_yazdir(root->next_sibling, derinlik);
}

// 3. BAĞLI LİSTE (MASA SİPARİŞLERİ İÇİN)
// Masalardaki sipariş sayıs belirsiz olduğu için Linked List kullandık.
typedef struct SiparisKalemi {
    int urun_id;
    int adet;
    struct SiparisKalemi *next;
} SiparisKalemi;

void masaya_siparis_ekle(SiparisKalemi **head, int urun_id, int adet) {
    Urun *urun = urun_bul(urun_id);
    if (urun == NULL) {
        printf("HATA: %d ID'li urun bulunamadi!\n", urun_id);
        return;
    }

    SiparisKalemi *yeni = (SiparisKalemi *) malloc(sizeof(SiparisKalemi));
    yeni->urun_id = urun_id;
    yeni->adet = adet;
    yeni->next = NULL;

    if (*head == NULL) *head = yeni;
    else {
        SiparisKalemi *temp = *head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = yeni;
    }
    printf(">> Masa siparisine eklendi: %d adet %s\n", adet, urun->isim);
}

void masa_siparisi_yazdir(int masa_no, SiparisKalemi *head) {
    printf("\n--- MASA %d AKTIF SIPARISLER ---\n", masa_no);
    if (head == NULL) {
        printf("Masa su an bos.\n\n");
        return;
    }

    SiparisKalemi *temp = head;
    float toplam_tutar = 0;
    while (temp != NULL) {
        Urun *u = urun_bul(temp->urun_id);
        if (u != NULL) {
            printf("- %s (Adet: %d, Fiyat: %.2f TL)\n", u->isim, temp->adet, u->fiyat * temp->adet);
            toplam_tutar += u->fiyat * temp->adet;
        }
        temp = temp->next;
    }
    printf("--------------------------------\nToplam Tutar: %.2f TL\n\n", toplam_tutar);
}

// 4. YIĞIN / STACK (GERİ AL - UNDO SİSTEMİ İÇİN)
// LIFO mantığı ile yapılan hatalı sipariş girişlerini geri alabilmek için Stack kullandık.
typedef struct IslemGecmisi {
    int masa_no;
    int urun_id;
    int adet;
    struct IslemGecmisi *next;
} IslemGecmisi;

IslemGecmisi *stack_top = NULL;

void islem_gecmisine_ekle(int masa_no, int urun_id, int adet) {
    IslemGecmisi *yeni_islem = (IslemGecmisi *) malloc(sizeof(IslemGecmisi));
    yeni_islem->masa_no = masa_no;
    yeni_islem->urun_id = urun_id;
    yeni_islem->adet = adet;
    yeni_islem->next = stack_top; // Push işlemi
    stack_top = yeni_islem;
}

void son_islemi_geri_al(SiparisKalemi **aktif_masalar) {
    if (stack_top == NULL) {
        printf("\n>> [HATA] Geri alinacak bir islem bulunamadi (Stack bos)!\n");
        return;
    }

    IslemGecmisi *son_islem = stack_top; // Pop işlemi başlangıcı
    stack_top = stack_top->next;

    SiparisKalemi **head = &aktif_masalar[son_islem->masa_no];
    SiparisKalemi *temp = *head;
    SiparisKalemi *prev = NULL;

    while (temp != NULL) {
        if (temp->urun_id == son_islem->urun_id) {
            temp->adet -= son_islem->adet;
            if (temp->adet <= 0) {
                if (prev == NULL) *head = temp->next;
                else prev->next = temp->next;
                free(temp); //Belleği serbest bırak.
            }
            break;
        }
        prev = temp;
        temp = temp->next;
    }

    Urun *u = urun_bul(son_islem->urun_id);
    printf("\n>> [GERI AL / UNDO] Masa %d'den %d adet %s siparisi iptal edildi!\n",
           son_islem->masa_no, son_islem->adet, u ? u->isim : "Urun");

    free(son_islem);
}


// 5. ÖNCELİKLİ KUYRUK (MUTFAK İŞ AKIŞI - MAX HEAP)
typedef struct {
    int masa_no;
    char siparis_detayi[100];
    int oncelik; // 1: Normal, 2: Acil
} MutfakSiparisi;

MutfakSiparisi mutfak_kuyrugu[MAX_MUTFAK_KAPASITESI];
int kuyruk_boyutu = 0;

void swap(MutfakSiparisi *a, MutfakSiparisi *b) {
    MutfakSiparisi t = *a;
    *a = *b;
    *b = t;
}

void mutfaga_ilet(int masa_no, const char *detay, int oncelik) {
    if (kuyruk_boyutu >= MAX_MUTFAK_KAPASITESI) return;
    int i = kuyruk_boyutu++;
    mutfak_kuyrugu[i].masa_no = masa_no;
    strcpy(mutfak_kuyrugu[i].siparis_detayi, detay);
    mutfak_kuyrugu[i].oncelik = oncelik;

    // Heapify Up: Öncelikli elemanı yukarı taşıyoruz.
    while (i != 0 && mutfak_kuyrugu[(i - 1) / 2].oncelik < mutfak_kuyrugu[i].oncelik) {
        swap(&mutfak_kuyrugu[i], &mutfak_kuyrugu[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
    printf(">> Mutfaga Iletildi! Masa: %d | Oncelik: %d\n", masa_no, oncelik);
}

void mutfaktan_siparisi_cikar() {
    if (kuyruk_boyutu <= 0) {
        printf(">> Mutfakta bekleyen siparis yok.\n");
        return;
    }
    MutfakSiparisi hazirlanan = mutfak_kuyrugu[0];
    mutfak_kuyrugu[0] = mutfak_kuyrugu[--kuyruk_boyutu];

    // Heapify Down: Kök değişince Heap özelliğini tekrar sağlıyoruz.
    int i = 0;
    while (1) {
        int sol = 2 * i + 1, sag = 2 * i + 2, en_buyuk = i;
        if (sol < kuyruk_boyutu && mutfak_kuyrugu[sol].oncelik > mutfak_kuyrugu[en_buyuk].oncelik) en_buyuk = sol;
        if (sag < kuyruk_boyutu && mutfak_kuyrugu[sag].oncelik > mutfak_kuyrugu[en_buyuk].oncelik) en_buyuk = sag;
        if (en_buyuk != i) {
            swap(&mutfak_kuyrugu[i], &mutfak_kuyrugu[en_buyuk]);
            i = en_buyuk;
        } else break;
    }
    printf(">> HAZIRLANAN SIPARIS: Masa %d | Oncelik: %s | Detay: %s\n",
           hazirlanan.masa_no,
           (hazirlanan.oncelik == 2) ? "ACIL" : "NORMAL",
           hazirlanan.siparis_detayi);
}

// 6. İKİLİ ARAMA AĞACI (GEÇMİŞ FİŞ ARŞİVİ - BST)
typedef struct FisNode {
    int fis_id;
    int masa_no;
    float toplam_tutar;
    struct FisNode *left, *right;
} FisNode;

FisNode *fis_ekle(FisNode *root, int fis_id, int masa_no, float tutar) {
    if (root == NULL) {
        FisNode *yeni = (FisNode *) malloc(sizeof(FisNode));
        yeni->fis_id = fis_id;
        yeni->masa_no = masa_no;
        yeni->toplam_tutar = tutar;
        yeni->left = yeni->right = NULL;
        return yeni;
    }
    if (fis_id < root->fis_id) root->left = fis_ekle(root->left, fis_id, masa_no, tutar);
    else if (fis_id > root->fis_id) root->right = fis_ekle(root->right, fis_id, masa_no, tutar);
    return root;
}

FisNode *fis_ara(FisNode *root, int fis_id) {
    if (root == NULL || root->fis_id == fis_id) return root;
    if (fis_id < root->fis_id) return fis_ara(root->left, fis_id);
    return fis_ara(root->right, fis_id);
}

// ARSIV DOSYASINA KAYDETME
void dosyaya_fis_kaydet(int fis_id, int masa_no, float tutar) {
    FILE *dosya = fopen("arsiv.txt", "a"); // 'a' modu: dosyanın sonuna ekler
    if (dosya != NULL) {
        fprintf(dosya, "%d %d %.2f\n", fis_id, masa_no, tutar);
        fclose(dosya);
    }
}

// Masayı kapatırken bellekteki sipariş listesini temizleyip fiş kesiyoruz.
FisNode *masayi_kapat_ve_fis_kes(int masa_no, SiparisKalemi **head, FisNode *arsiv_root) {
    if (*head == NULL) {
        printf(">> Masa %d zaten bos. Kapatilacak bir hesap yok!\n", masa_no);
        return arsiv_root;
    }

    SiparisKalemi *temp = *head;
    float toplam = 0;

    while (temp != NULL) {
        Urun *u = urun_bul(temp->urun_id);
        if (u != NULL) toplam += u->fiyat * temp->adet;
        SiparisKalemi *silinecek = temp;
        temp = temp->next;
        free(silinecek);
    }

    *head = NULL;

    int yeni_fis_id = global_fis_id++;
    arsiv_root = fis_ekle(arsiv_root, yeni_fis_id, masa_no, toplam);

    // DOSYAYA YAZMA
    dosyaya_fis_kaydet(yeni_fis_id, masa_no, toplam);

    printf("\n=================================\n");
    printf("   MASA %d HESABI KAPATILDI\n", masa_no);
    printf("   Fis No      : %d (Dosyaya kaydedildi)\n", yeni_fis_id);
    printf("   Odenen Tutar: %.2f TL\n", toplam);
    printf("=================================\n\n");

    return arsiv_root;
}

// ARSIV DOSYASINDAN OKUMA
FisNode *dosyadan_arsiv_yukle(FisNode *root) {
    FILE *dosya = fopen("arsiv.txt", "r");
    if (dosya == NULL) return root;

    int id, masa;
    float tutar;
    while (fscanf(dosya, "%d %d %f", &id, &masa, &tutar) != EOF) {
        root = fis_ekle(root, id, masa, tutar);
        // ID cakismasini onlemek icin global_fis_id'yi guncelle
        if (id >= global_fis_id) global_fis_id = id + 1;
    }
    fclose(dosya);
    return root;
}

// 7. PERFORMANS TESTİ (ARRAY VS HASH TABLE - Kapsamlı Analiz)
void performans_testi() {
    printf("\n===============================================================================\n");
    printf("   PERFORMANS TESTI: ARAMA VE EKLEME MALIYETLERI (Array vs Hash Table) \n");
    printf("===============================================================================\n");

    // Yönergeye uygun olarak küçük, orta ve büyük veri setleri
    int veri_setleri[] = {1000, 10000, 100000};
    int test_tekrari = 1000; // Süreyi daha net ölçebilmek için arama döngü sayısı

    // Sonuçların tablo olarak gösterilmesi
    printf("%-15s | %-25s | %-25s\n", "Veri Boyutu", "Dizi (Array) Suresi", "Hash Table Suresi");
    printf("-------------------------------------------------------------------------------\n");

    for (int v = 0; v < 3; v++) {
        int test_boyutu = veri_setleri[v];
        int *dizi = (int *) malloc(test_boyutu * sizeof(int));

        // --- 1. EKLEME (INSERTION) TESTİ ---
        clock_t basla_ekle_dizi = clock();
        for (int i = 0; i < test_boyutu; i++) {
            dizi[i] = 5000 + i; // Diziye indeks ile ekleme O(1)
        }
        clock_t bitis_ekle_dizi = clock();
        double sure_ekle_dizi = ((double) (bitis_ekle_dizi - basla_ekle_dizi)) / CLOCKS_PER_SEC;

        clock_t basla_ekle_hash = clock();
        for (int i = 0; i < test_boyutu; i++) {
            urun_ekle(5000 + i, "Test Urunu", 10.0, 5); // Hash hesaplama ve malloc maliyeti var
        }
        clock_t bitis_ekle_hash = clock();
        double sure_ekle_hash = ((double) (bitis_ekle_hash - basla_ekle_hash)) / CLOCKS_PER_SEC;


        // --- 2. ARAMA (SEARCH) TESTİ (Worst Case - En son eleman aranıyor) ---
        int aranan_id = 5000 + test_boyutu - 1;

        clock_t basla_ara_dizi = clock();
        for (int t = 0; t < test_tekrari; t++) {
            for (int i = 0; i < test_boyutu; i++) {
                if (dizi[i] == aranan_id) break; // Linear Search O(n)
            }
        }
        clock_t bitis_ara_dizi = clock();
        double sure_ara_dizi = ((double) (bitis_ara_dizi - basla_ara_dizi)) / CLOCKS_PER_SEC;

        clock_t basla_ara_hash = clock();
        for (int t = 0; t < test_tekrari; t++) {
            urun_bul(aranan_id); // Hash Table Search O(1)
        }
        clock_t bitis_ara_hash = clock();
        double sure_ara_hash = ((double) (bitis_ara_hash - basla_ara_hash)) / CLOCKS_PER_SEC;

        // Tablo formatında yazdırma
        printf("%-15d | Ekleme: %-15.6f sn | Ekleme: %-15.6f sn\n", test_boyutu, sure_ekle_dizi, sure_ekle_hash);
        printf("%-15s | Arama:  %-15.6f sn | Arama:  %-15.6f sn\n", "", sure_ara_dizi, sure_ara_hash);
        printf("-------------------------------------------------------------------------------\n");

        free(dizi); // Bellek sızıntısını önlemek için
    }

    // Yönergedeki "Sonuçların veri yapısı seçimi ile ilişkisinin açıklanması" maddesi için rapor
    printf("\n>>> PERFORMANS ANALIZI VE VERI YAPISI DEGERLENDIRMESI <<<\n");
    printf("- Arama Islemi: Dizi uzerinde dogrusal arama (Linear Search) yapildigi icin\n");
    printf("  veri boyutu arttikca sure O(n) karmasikligiyla dogrusal olarak artmistir.\n");
    printf("  Hash Table ise arama isleminde O(1) maliyetiyle boyut artisindan etkilenmemistir.\n\n");

    printf("- Ekleme Islemi: Diziye (Array) indeks uzerinden dogrudan atama yapmak O(1) hizindadir.\n");
    printf("  Hash Table'da ise her urun icin Hashing fonksiyonu calistirma ve Bagli Listeye\n");
    printf("  (Linked List) yeni dugum (malloc) ekleme maliyeti nedeniyle ekleme suresi daha uzundur.\n\n");

    printf("SONUC: Sistemde urunlerin menuye eklenmesi gunde birkac kez yapilirken, siparis\n");
    printf("alimi sirasinda 'arama' islemi binlerce kez yapilmaktadir. Bu yuzden ekleme\n");
    printf("maliyetinden feragat edilerek, arama hizini maksimize eden Hash Table tercih edilmistir.\n");
    printf("===============================================================================\n");
}

// 8. STANDART KUYRUK (MÜŞTERİ BEKLEME SIRASI - FIFO)
// İlk gelen masaya ilk oturur.
typedef struct BeklemeNode {
    char musteri_ismi[50];
    struct BeklemeNode *next;
} BeklemeNode;

typedef struct {
    BeklemeNode *front, *rear;
} BeklemeKuyrugu;

void kuyruk_baslat(BeklemeKuyrugu *q) {
    q->front = q->rear = NULL;
}

void siraya_musteri_ekle(BeklemeKuyrugu *q, const char *isim) {
    BeklemeNode *yeni = (BeklemeNode *) malloc(sizeof(BeklemeNode));
    strcpy(yeni->musteri_ismi, isim);
    yeni->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = yeni;
        printf(">> %s siraya alindi (Sira basi).\n", isim);
        return;
    }
    q->rear->next = yeni;
    q->rear = yeni;
    printf(">> %s siraya alindi.\n", isim);
}

void siradan_musteri_cikar(BeklemeKuyrugu *q) {
    if (q->front == NULL) {
        printf(">> Bekleyen musteri yok.\n");
        return;
    }
    BeklemeNode *temp = q->front;
    printf(">> SIRADAKI MUSTERI MASAYA ALINDI: %s\n", temp->musteri_ismi);
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
}

// 9. GRAPH (TESLİMAT ROTASI - ADJACENCY MATRIX)
// 0: Restoran, 1-4: Teslimat Noktalari (Mahalleler)
int teslimat_matrisi[DURAK_SAYISI][DURAK_SAYISI] = {
    {0, 5, 10, 0, 0},   // 0: Restoran
    {5, 0, 3, 11, 0},   // 1: Mahalle A
    {10, 3, 0, 2, 15},  // 2: Mahalle B
    {0, 11, 2, 0, 1},   // 3: Mahalle C
    {0, 0, 15, 1, 0}    // 4: Mahalle D
};

void teslimat_rotasi_sorgula(int hedef) {
    printf("\n--- TESLIMAT ROTASI ANALIZI (Graph - Adjacency Matrix) ---\n");
    if (hedef < 1 || hedef >= DURAK_SAYISI) {
        printf("Gecersiz teslimat noktasi (Lutfen 1-4 arasi seciniz)!\n");
        return;
    }

    printf("Restoran (Merkez) ile Mahalle %d arasindaki direkt mesafe: ", hedef);
    if (teslimat_matrisi[0][hedef] == 0) {
        printf("Direkt yol yok, aktarma gerekli.\n");
    } else {
        printf("%d km\n", teslimat_matrisi[0][hedef]);
    }

    printf("Hedef Mahallenin komsu baglantilari:\n");
    for (int i = 0; i < DURAK_SAYISI; i++) {
        if (teslimat_matrisi[hedef][i] != 0) {
            printf("- Nokta %d ile baglanti var (Mesafe: %d km)\n", i, teslimat_matrisi[hedef][i]);
        }
    }
}

// BAŞLANGIÇ VERİLERİNİ YÜKLEME
MenuNode *sistemi_baslat() {
    for (int i = 0; i < HASH_SIZE; i++) urun_tablosu[i] = NULL;

    urun_ekle(201, "Mercimek Corbasi", 40.0, 100);
    urun_ekle(202, "Ezogelin Corbasi", 45.0, 80);
    urun_ekle(101, "Izgara Kofte", 150.0, 50);
    urun_ekle(102, "Adana Kebap", 180.0, 30);
    urun_ekle(103, "Tavuk Sis", 120.0, 40);
    urun_ekle(104, "Iskender Kebap", 200.0, 25);
    urun_ekle(401, "Firin Sutlac", 60.0, 25);
    urun_ekle(402, "Kunefe", 90.0, 15);
    urun_ekle(403, "Kazandibi", 70.0, 20);
    urun_ekle(301, "Kutu Kola", 25.0, 200);
    urun_ekle(302, "Ev Yapimi Ayran", 20.0, 150);
    urun_ekle(303, "Salgam Suyu", 25.0, 100);
    urun_ekle(304, "Su (Pet Sise)", 10.0, 500);

    MenuNode *ana_menu = dugum_olustur("RESTORAN MENUSU");
    MenuNode *corbalar = dugum_olustur("Corbalar");
    MenuNode *ana_yemekler = dugum_olustur("Ana Yemekler");
    MenuNode *tatlilar = dugum_olustur("Tatlilar");
    MenuNode *icecekler = dugum_olustur("Icecekler");

    alt_dugum_ekle(ana_menu, corbalar);
    alt_dugum_ekle(ana_menu, ana_yemekler);
    alt_dugum_ekle(ana_menu, tatlilar);
    alt_dugum_ekle(ana_menu, icecekler);

    alt_dugum_ekle(corbalar, dugum_olustur("Mercimek Corbasi (ID: 201) - 40.00 TL"));
    alt_dugum_ekle(corbalar, dugum_olustur("Ezogelin Corbasi (ID: 202) - 45.00 TL"));
    alt_dugum_ekle(ana_yemekler, dugum_olustur("Izgara Kofte (ID: 101) - 150.00 TL"));
    alt_dugum_ekle(ana_yemekler, dugum_olustur("Adana Kebap (ID: 102) - 180.00 TL"));
    alt_dugum_ekle(ana_yemekler, dugum_olustur("Tavuk Sis (ID: 103) - 120.00 TL"));
    alt_dugum_ekle(ana_yemekler, dugum_olustur("Iskender Kebap (ID: 104) - 200.00 TL"));
    alt_dugum_ekle(tatlilar, dugum_olustur("Firin Sutlac (ID: 401) - 60.00 TL"));
    alt_dugum_ekle(tatlilar, dugum_olustur("Kunefe (ID: 402) - 90.00 TL"));
    alt_dugum_ekle(tatlilar, dugum_olustur("Kazandibi (ID: 403) - 70.00 TL"));
    alt_dugum_ekle(icecekler, dugum_olustur("Kutu Kola (ID: 301) - 25.00 TL"));
    alt_dugum_ekle(icecekler, dugum_olustur("Ev Yapimi Ayran (ID: 302) - 20.00 TL"));
    alt_dugum_ekle(icecekler, dugum_olustur("Salgam Suyu (ID: 303) - 25.00 TL"));
    alt_dugum_ekle(icecekler, dugum_olustur("Su (Pet Sise) (ID: 304) - 10.00 TL"));

    return ana_menu;
}

// ANA FONKSİYON VE KULLANICI ARAYÜZÜ
int main() {
    MenuNode *menu = sistemi_baslat();
    SiparisKalemi *aktif_masalar[21] = {NULL}; // 20 masa için
    FisNode *root_arsiv = NULL;

    // DOSYADAN ARSIVI YUKLE
    root_arsiv = dosyadan_arsiv_yukle(root_arsiv);
    if(root_arsiv != NULL) printf(">> Eski fisler basariyla yuklendi.\n");

    // Verilerin silinmemesi için kuyruğu döngünün dışında tanımla ve başlat:
    BeklemeKuyrugu kapi_sirasi;
    kuyruk_baslat(&kapi_sirasi);

    int secim;
    while (1) {
        printf("\n   RESTORAN OTOMASYON SISTEMI\n");
        printf("=====================================\n");
        printf("1. Menuyu Goruntule (Tree)\n");
        printf("2. Masaya Siparis Ekle (Linked List & Stack)\n");
        printf("3. Masa Hesabini Goruntule\n");
        printf("4. Masayi Kapat ve Fis Kes (LinkedList -> BST & Dosya)\n");
        printf("5. Siparisi Mutfaga Ilet (Priority Queue)\n");
        printf("6. Mutfaktan Siparis Cikar (Max-Heap)\n");
        printf("7. Gecmis Fis Sorgula (BST)\n");
        printf("8. Performans Testi Baslat (Array vs Hash)\n");
        printf("9. Son Girilen Siparisi Geri Al (Stack)\n");
        printf("10. Musteriyi Bekleme Sirasina Al (Queue - Enqueue)\n");
        printf("11. Bekleyen Musteriyi Masaya Al (Queue - Dequeue)\n");
        printf("12. Teslimat Rotasi Sorgula (Graph)\n");
        printf("0. Cikis\n");
        printf("Seciminiz: ");

        if (scanf("%d", &secim) != 1) break;

        switch (secim) {
            case 0:
                printf("Sistem kapatiliyor...\n");
                return 0;
            case 1:
                printf("\n");
                menuyu_yazdir(menu, 0);
                break;
            case 2: {
                int masa, id, adet, devam;
                printf("Masa No (1-20): ");
                scanf("%d", &masa);

                if (masa > 0 && masa <= 20) {
                    do {
                        printf("\n--- MASA %d SIPARIS GIRISI ---\n", masa);
                        printf("Urun ID: ");
                        scanf("%d", &id);
                        printf("Adet: ");
                        scanf("%d", &adet);

                        Urun *u = urun_bul(id);
                        if (u != NULL) {
                            masaya_siparis_ekle(&aktif_masalar[masa], id, adet);
                            islem_gecmisine_ekle(masa, id, adet); // Stack'e ekle
                        } else {
                            printf("[HATA] Gecersiz Urun ID!\n");
                        }

                        printf("\nBu masaya baska urun eklemek ister misiniz? (Evet: 1 / Hayir: 0): ");
                        scanf("%d", &devam);

                    } while (devam == 1); // Kullanıcı 1 dediği sürece bu masa ekranında kalır

                    printf(">> Masa %d siparis girisleri tamamlandi. Ana menuye donuluyor...\n", masa);
                } else {
                    printf("[HATA] Gecersiz Masa No!\n");
                }
                break;
            }
            case 3: {
                int masa;
                printf("Masa No (1-20): ");
                scanf("%d", &masa);
                if (masa > 0 && masa <= 20) masa_siparisi_yazdir(masa, aktif_masalar[masa]);
                break;
            }
            case 4: {
                int masa;
                printf("Kapatilacak Masa No: ");
                scanf("%d", &masa);
                if (masa > 0 && masa <= 20)
                    root_arsiv = masayi_kapat_ve_fis_kes(masa, &aktif_masalar[masa], root_arsiv);
                break;
            }
            case 5: {
                int masa, oncelik;
                char detay[100];
                printf("Masa No: ");
                scanf("%d", &masa);
                printf("Oncelik (1:Normal, 2:Acil): ");
                scanf("%d", &oncelik);
                getchar(); // buffer temizle
                printf("Detay: ");
                fgets(detay, 100, stdin);
                detay[strcspn(detay, "\n")] = 0;
                mutfaga_ilet(masa, detay, oncelik);
                break;
            }
            case 6:
                printf("\n");
                mutfaktan_siparisi_cikar();
                break;
            case 7: {
                int aranan_fis;
                printf("Sorgulanacak Fis ID: ");
                scanf("%d", &aranan_fis);
                FisNode *f = fis_ara(root_arsiv, aranan_fis);
                if (f != NULL) printf("\n[FIS BULUNDU] Fis No: %d | Masa: %d | Tutar: %.2f TL\n", f->fis_id, f->masa_no,
                                      f->toplam_tutar);
                else printf("\n[HATA] Fis bulunamadi!\n");
                break;
            }
            case 8:
                performans_testi();
                break;
            case 9:
                son_islemi_geri_al(aktif_masalar);
                break;
            case 10: {
                char isim[50];
                printf("Musteri ismi: ");
                scanf("%s", isim);
                siraya_musteri_ekle(&kapi_sirasi, isim);
                break;
            }
            case 11:
                siradan_musteri_cikar(&kapi_sirasi);
                break;
            case 12: {
                int mahalle;
                printf("Teslimat yapilacak Mahalle No (1-4): ");
                scanf("%d", &mahalle);
                teslimat_rotasi_sorgula(mahalle);
                break;
            }
            default:
                printf("Gecersiz secim!\n");
                break;
        }
    }
    return 0;
}
