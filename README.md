# CollageCraft - Aplikacja do edycji zdjęć i tworzenia kolaży w języku C++

## Główne funkcje programu
* **Zarządzanie płótnem:** Swobodne układanie, przesuwanie, skalowanie i obracanie dodanych grafik.
* **System warstw:** Kontrola nad hierarchią elementów z podglądem i wyróżnianiem aktywnych warstw na liście bocznej.
* **Edycja nieniszcząca:** System pozwalający na jednoczesne nakładanie wielu efektów (jasność, kontrast, skala szarości, sepia, negatyw) bez modyfikowania oryginalnego pliku.
* **System masek:** Możliwość wycinania fragmentów zdjęć (owal, prostokąt, romb) z wykorzystaniem biblioteki OpenCV.
* **Galeria naklejek:** Wbudowany system naklejek, wkompilowany w plik wykonywalny przy użyciu Qt Resource System (`.qrc`).
* **Eksport:** Renderowanie całej sceny i zapis gotowego kolażu do plików PNG lub JPG.

## Technologie
* **C++**
* **Qt 6** (Qt Widgets, QGraphicsScene, CMake)
* **OpenCV** (Core, Imgproc)

## Skróty klawiszowe
Po zaznaczeniu obiektu na płótnie, aplikacja obsługuje następujące skróty:
* `+` / `=` : Powiększ zdjęcie
* `-` : Pomniejsz zdjęcie
* `R` : Obróć w prawo
* `L` : Obróć w lewo
* `W` : Przesuń warstwę o 1 poziom wyżej
* `S` : Przesuń warstwę o 1 poziom niżej
* `E` : Przenieś na sam wierzch
* `Q` : Przenieś na sam spód
