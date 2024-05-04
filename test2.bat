@echo off
rem Sprawdź czy istnieje interpreter Pythona w środowisku
where python >nul 2>nul
if %errorlevel% neq 0 (
    echo Błąd: Interpreter Pythona nie został znaleziony. Upewnij się, że Python jest zainstalowany.
    exit /b 1
)

rem Uruchom polecenie Pythona z odpowiednimi argumentami
python -m pip install .
python test.py -t AdjacencyList is_bipartite