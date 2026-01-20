# Wspinaczka 3D - Projekt Zaliczeniowy OpenGL

## 1. Opis Projektu
"Wspinaczka 3D" to gra zręcznościowa, w której gracz steruje sferycznym obiektem (piłką). Celem gry jest pokonanie toru przeszkód zawieszonego w przestrzeni i dotarcie do strefy końcowej ("Meta").

Główną inspiracją dla projektu były mechaniki znane z gier takich jak *Icy Tower* czy *Chained Together*, przeniesione w trójwymiarowe środowisko. Projekt skupia się na implementacji silnika graficznego od podstaw (własny potok renderowania), fizyce ruchu oraz obsłudze modeli 3D i zaawansowanego oświetlenia.

## 2. Sterowanie
| Klawisz | Akcja |
| :---: | :--- |
| **W / S** | Ruch do przodu / do tyłu |
| **A / D** | Ruch w lewo / w prawo |
| **Mysz** | Rozglądanie się (obrót kamery wokół postaci) |
| **SPACJA** | Skok |
| **LEWY SHIFT** | Sprint (chwilowe przyspieszenie / Boost) |
| **ESC** | Wyjście z gry / Pauza |

## 3. Zaimplementowane Mechaniki i Elementy Graficzne

### Silnik i Grafika:
* **System Oświetlenia:** Model oświetlenia Phong/Blinn-Phong oraz elementy PBR dla tekstur (Albedo, Metallic, Roughness).
* **Cienie:** Dynamiczne cienie generowane przy użyciu techniki Shadow Mapping (Depth Map).
* **Skybox:** Renderowanie sześcianu otoczenia (cubemap) imitującego niebo.
* **Modele 3D:** Obsługa importu modeli (formaty .obj, .fbx) przy użyciu biblioteki Assimp.
* **Kamera TPP:** Kamera trzecioosobowa z systemem kolizji (zapobieganie przenikaniu kamery przez ściany).

### Fizyka i Gameplay:
* **Fizyka AABB:** Detekcja kolizji (Axis-Aligned Bounding Box) dla obiektów statycznych i dynamicznych.
* **Mechanika Ruchu:** System poruszania się, grawitacji oraz mechanika sprintu (Boost).
* **Obiekty Interaktywne:**
    * *Trampoliny:* Wybijają gracza w górę przy kontakcie.
    * *Ruchome Ściany:* Przeszkody przemieszczające się w pętli, które mogą zrzucić gracza.
    * *Szklany Most:* Przezroczyste kafelki, z których wybrane są "fałszywe" i zapadają się pod graczem (mechanika inspirowana "Squid Game").
    * *Wiatr:* Strefy wpływające na wektor ruchu gracza, spychające go w określonym kierunku.

## 4. Wykorzystane Biblioteki i Technologie
Projekt został zrealizowany w języku **C++** przy użyciu **OpenGL 3.3+**.

1.  **GLFW** - Obsługa okna, kontekstu OpenGL oraz wejścia (klawiatura/mysz).
2.  **GLAD** - Ładowanie wskaźników do funkcji OpenGL.
3.  **GLM** - Biblioteka matematyczna (wektory, macierze, przekształcenia).
4.  **Assimp** - Importowanie modeli 3D.
5.  **stb_image** - Wczytywanie tekstur.

## 5. Autorzy
* **Kacper Szczudło**
* **Norbert Armatys**
* **Piotr Cebula**
* **Krzysztof Łyszczarz**
