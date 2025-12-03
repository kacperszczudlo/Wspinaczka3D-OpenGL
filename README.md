# Wspinaczka 3D - OpenGL

Projekt gry zręcznościowej 3D stworzony w języku C++ z wykorzystaniem biblioteki graficznej OpenGL. Gracz steruje postacią, której celem jest wspinaczka na szczyt po platformach, unikając upadku w przepaść.

## 📌 O projekcie

Główną inspiracją dla projektu były mechaniki znane z gier takich jak *Icy Tower* czy *Chained Together*, przeniesione w trójwymiarowe środowisko. Projekt skupia się na implementacji silnika graficznego od podstaw, fizyce ruchu oraz obsłudze modeli 3D i oświetlenia.

### Kluczowe funkcjonalności (zrealizowane):
- **Silnik renderujący**: Własny potok renderowania oparty na shaderach (GLSL).
- **Kamera TPP**: Kamera trzecioosobowa z systemem kolizji (zapobieganie przenikaniu przez ściany).
- **Fizyka gracza**: System poruszania się, skakania oraz grawitacji.
- **Mechanika Sprintu**: Możliwość chwilowego przyspieszenia ruchu (Boost).
- **Obsługa modeli 3D**: Importowanie zewnętrznych modeli oraz nakładanie tekstur.
- **Stany gry**: Zaimplementowany ekran startowy oraz pętla gry.

## 🛠 Technologie

Projekt został zrealizowany przy użyciu następujących technologii i bibliotek:

*   **Język**: C++
*   **API Graficzne**: OpenGL 3.3+
*   **GLFW**: Obsługa okna, kontekstu OpenGL oraz wejścia (klawiatura/mysz).
*   **GLAD**: Ładowanie wskaźników do funkcji OpenGL.
*   **GLM**: Biblioteka matematyczna (wektory, macierze, przekształcenia).
*   **STB Image** (lub inna użyta): Obsługa ładowania tekstur.

## 🎮 Sterowanie

| Klawisz | Akcja |
| :---: | :--- |
| **W, A, S, D** | Poruszanie się postacią |
| **Mysz** | Obrót kamery wokół postaci |
| **Spacja** | Skok |
| **Shift** | Sprint (Przyspieszenie) |

## 🚀 Instalacja i Uruchomienie

1. **Sklonuj repozytorium:**
   ```bash
   git clone https://github.com/kacperszczudlo/Wspinaczka3D-OpenGL.git
