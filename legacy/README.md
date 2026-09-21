# The earlier version

*[Español abajo](#versión-anterior)*

`solver_toy13.cpp` is the original single-file solver: the synthetic river game
with 13 loose cards, one per player. The 52-card engine in `../src/` replaces
it, but it is kept because it is a complete, self-contained implementation of
DCFR with nodelocking in about 2,000 lines, and it reads well as a reference.

```bash
g++ -std=c++17 -O3 -static -o solver_toy13 legacy/solver_toy13.cpp
```

---

# Versión anterior

`solver_toy13.cpp` es el solver original de un solo archivo: el juego de river
sintético con 13 cartas sueltas, una por jugador. Lo sustituye el motor de 52
cartas en `../src/`, pero se conserva porque es una implementación completa y
autónoma de DCFR con nodelocking en ~2000 líneas, útil como referencia legible.

```bash
g++ -std=c++17 -O3 -static -o solver_toy13 legacy/solver_toy13.cpp
```
