# Opis projektu: Tramwaj wodny

## 1. Temat projektu
Symulacja dzilania tramwaju wodnego kursujacego w sezonie letnim na trasie Krakow Wawel - Tyniec.
Projekt obejmuje modelowanie zachowania pasazerow, kapitana statku oraz dyspozytora, z uwzglednieniem ograniczen pojemnosci.
Projekt realizuje symulacje wspolbiezna oparta o wiele procesow komunikujacych sie za pomoca mechanizmow IPC systemu Linux.

## 2. Parametry systemu i oznaczenia
- N - maksymalna liczba pasazerow na statku (N > 0)
- M - maksymalna liczba rowerow na statku (M < N)
- K - pojemnosc mostku laczacego statek z ladem (K < N)
- T1 - czas oczekiwania na wyplyniecie
- T2 - czas trwania rejsu
- R - maksymalna liczba rejsow w ciagu dnia
- sygnal1 - polecenie od dyspozytora do wczesniejszego wyplyniecia
- sygnal2 - polecenie zakonczenia dalszych rejsow
Wszystkie parametry wejsciowe sa walidowane przed uruchomieniem symulacji. W przypadku blednych wartosci program zglasza blad i konczy dzialanie.

## 3.Opis zadan

### Dyspozytor
- Wysylanie sygnalow sterujacych w trakcie trwania symulacji
- Wysylanie sygnalow sterujacych:
  - sygnal1 - wczesniejsze wyplyniecie
  - sygnal2 - przerwanie pracy statku
- Kontrolowanie zakonczenia symulacji

### Kapitan statku
- Kontrola warunkow przed odplynieciem:
  - czy na mostku nie ma pasazerow
  - czy liczba pasazerow na pokladzie <= N
  - czy liczba rowerow <= M
- Decyduje o odplynieciu (po czasie T1 lub na sygnal1)
- Moze rowniez otrzymac polecenie przerwania dalszych rejsow (sygnal2):
  - jesli podczas zaladunku - statek nie wyplywa, pasazerowie schodza
  - jesli podczas rejsu - dokancza biezacy rejs
- Rejs trwa czas T2
- Statek wykonuje maksymalnie R rejsow w ciagu dnia

### Pasazer
- Ustawia sie w kolejce
- Probuje wejsc na statek przez mostek o pojemnosci K:
  - bez roweru, zajmuje 1 miejsce
  - z rowerem, zajmuje 2 miejsca
- Na poklad moze wejsc maksymalnie N pasazerow oraz M < N rowerow
- Jesli statek ma odplynac i nie zmiescil sie na pokladzie opuszcza mostek,
- Po wejsciu na statek oczekuje doplyniecia do celu,
- Po doplynieciu do Tynca opuszcza statek, ruch na mostku odbywa sie tylko w jedna strone

## 4. Testy symulacji

### Test 1 - Podstawowy przeplyw
Cel: Sprawdzenie poprawnego kursowania tramwaju
-> Parametry: N=10 M=3 K=5 T1=5 T2=3 R=2
- Wejscie pasazerow bez rowerow
- Przestrzeganie limitu N
- Prawidlowe odplyniecie po czasie T1
- Poprawny rejs i powrot

### Test 2 - Ograniczenia statku
Cel: Test pojemnosci statku
-> Parametry: N=6 M=2 K=5 T1=15 T2=5 R=1
- Proba wejscia wiekszej liczby pasazerow niz N
- Proba przewiezienia liczby rowerow > M

### Test 3 - Przedwczesne odplyniecie
Cel:  Test reakcji na sygnal1
-> Parametry: N=20 M=10 K=10 T1=240 T2=5 R=1
- Wyslanie sygnalu1 podczas zaladunku
- Natychmiastowe przygotowanie do odplyniecia
- Prawidlowe obsluzenie pasazerow na moscie

### Test 4 - Przerwanie rejsow
Cel: Test zakonczenia pracy
-> Parametry: N=20 M=10 K=10 T1=10 T2=10 R=50
- Wyslanie sygnalu2 podczas roznych faz cyklu
- Sprawdzenie bezpiecznego zakonczenia przy pelnym statku
- Weryfikacja nierozpoczynania nowych rejsow

### Test 5 - Graniczne warunki
Cel: Test w warunkach wiekszego obciazenia
-> Parametry: N=51 M=50 K=50 T1=2 T2=2 R=10
- Maksymalna liczba rowerow 
- Jednoczesne wejscie wielu pasazerow
- Osiagniecie limitu R rejsow


## 5. Repozytorium GitHub

Link do repozytorium: https://github.com/Monia895/tramwaj_wodny-
