# Versión anterior

`solver_toy13.cpp` es el solver original de un solo archivo: el juego de river
sintético con 13 cartas sueltas, una por jugador. Lo sustituye el motor de 52
cartas en `../src/`, pero se conserva porque es una implementación completa y
autónoma de DCFR con nodelocking en ~2000 líneas, útil como referencia legible.

```bash
g++ -std=c++17 -O3 -static -o solver_toy13 legacy/solver_toy13.cpp
```
