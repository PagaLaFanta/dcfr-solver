# DCFR Solver

[![build](https://github.com/danidealmeria-alt/dcfr-solver/actions/workflows/build.yml/badge.svg)](https://github.com/danidealmeria-alt/dcfr-solver/actions/workflows/build.yml)
[![release](https://img.shields.io/github/v/release/danidealmeria-alt/dcfr-solver?include_prereleases)](https://github.com/danidealmeria-alt/dcfr-solver/releases)
[![license](https://img.shields.io/badge/license-GPLv3-blue)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-informational)](#compilarlo-tú)

Un solver de póker postflop para No Limit Hold'em. Mazo completo de 52 cartas,
rangos completos de 1.326 combos, flop / turn / river, Discounted CFR. Corre **en
tu propio ordenador** y se maneja desde el navegador.

Sin dependencias: C++17 y la librería estándar. Un binario, sin instalador, sin
servidor, sin cuenta, sin conexión.

**Doble clic y se abre en el navegador.** Desde una terminal, `solver --gui` hace
lo mismo, y `solver` a secas te da la consola de texto.

> 🇬🇧 **[This README in English](README.md)** · la interfaz habla los dos
> idiomas — el engranaje de arriba a la derecha los cambia al momento.

---

## Índice

- [Qué es](#qué-es)
- [Empezar](#empezar)
- [Arranque de cinco minutos](#arranque-de-cinco-minutos)
- [Qué hace, y qué no](#qué-hace-y-qué-no)
- [Cómo funciona](#cómo-funciona)
  - [El modelo](#el-modelo)
  - [Rangos y una sola indexación](#rangos-y-una-sola-indexación)
  - [El árbol de apuestas](#el-árbol-de-apuestas)
  - [Las reglas de construcción](#las-reglas-de-construcción)
  - [Nodos de azar e isomorfismo de palos](#nodos-de-azar-e-isomorfismo-de-palos)
  - [Discounted CFR](#discounted-cfr)
  - [Memoria](#memoria)
  - [Hilos](#hilos)
  - [Showdown](#showdown)
  - [Equity](#equity)
  - [Explotabilidad y objetivo de precisión](#explotabilidad-y-objetivo-de-precisión)
  - [Rake](#rake)
  - [Nodelocking](#nodelocking)
  - [Manos hechas y proyectos](#manos-hechas-y-proyectos)
- [La interfaz](#la-interfaz)
- [Jugar el árbol](#jugar-el-árbol)
- [Resolver muchos boards de noche](#resolver-muchos-boards-de-noche)
- [Rendimiento](#rendimiento)
- [Que esté bien](#que-esté-bien)
- [Referencia](#referencia)
- [Mapa del código](#mapa-del-código)
- [Compilarlo tú](#compilarlo-tú)
- [Limitaciones](#limitaciones)
- [Contribuir](#contribuir)
- [Licencia](#licencia)

---

## Qué es

Un solver coge un spot — un board, dos rangos, un bote, un stack y unos tamaños
de apuesta legales — y calcula una estrategia que no se puede explotar mucho: qué
fracción del tiempo cada mano debe apostar, pasar, pagar, subir o retirarse en
cada nodo del árbol.

Este es un solo programa autocontenido. El motor son unas 27.000 líneas de C++17
en cabeceras; la interfaz es una página web que el programa se sirve a sí mismo
por un socket en `127.0.0.1`. Nada sale de tu máquina y no hay que instalar nada
para que funcione.

**No** es un solver de preflop y no juega. Le das un flop, un turn o un river ya
repartido.

---

## Empezar

### Usarlo

1. Baja el binario de tu sistema de la [página de Releases](../../releases).
2. Ponlo en una carpeta suya: crea un `saves/` al lado la primera vez que
   guardes algo, así que `Descargas` es mala casa.
3. **Windows**: doble clic. Se abre `http://127.0.0.1:8777` en tu navegador. La
   primera vez Windows avisará de que el programa no está firmado (la pantalla
   azul de SmartScreen): *Más información* → *Ejecutar de todas formas*. Firmar
   un ejecutable cuesta dinero todos los años y este proyecto no lo tiene. Si
   prefieres no fiarte de un binario, compílalo tú: son treinta segundos.
4. **Linux / macOS**: `chmod +x solver-linux-x64 && ./solver-linux-x64 --gui`.

El programa arranca con un spot puesto — A♥9♥4♥, bote 55, stack 220 — así que
puedes darle a **Resolver** y tener una estrategia delante en unos diez segundos.

### Compilarlo

Necesitas un compilador de C++17. Esa es toda la lista de dependencias.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

o a mano, en Windows con MinGW:

```bash
g++ -std=c++17 -O3 -march=native -Wall -Isrc -static -o solver src/main.cpp -lws2_32
```

En [Compilarlo tú](#compilarlo-tú) está qué hace cada flag y por qué `-static` no
es opcional en Windows.

---

## Arranque de cinco minutos

1. **BOARD** — pincha *Elegir flop* y coge tres cartas, o escribe `Ah 9h 4h`.
   Tres cartas resuelven desde el flop, cuatro desde el turn, cinco desde el
   river.
2. **RANGOS** — pinta la rejilla 13×13, o pega texto: `AA,KK,AKs,A8o:0.5`. La
   rejilla y el texto son lo mismo por dos ventanas; lo que pintes sale escrito
   para pegarlo donde quieras. Pasa el ratón por una casilla y te dice el peso
   que lleva.
3. **MONTAJE DEL ÁRBOL** — bote inicial, stack efectivo, y los tamaños de
   apuesta y de subida de cada jugador en cada calle. Los tamaños van en **por
   ciento del bote** (`50` es medio bote) y se truncan a **fichas enteras**, como
   en una mesa de verdad.
4. **Precisión deseada (% del bote)** — deja el `1` con la casilla marcada. Eso es "resuelve
   hasta que sea explotable por menos del 1% del bote, y para", que es una
   instrucción mucho mejor que un número de iteraciones.
5. **Montar árbol** — te dice cuánta memoria va a ocupar *antes* de pedirla, y
   qué recortar si te pasas del límite.
6. **Resolver**.
7. Navega el árbol. Pincha una mano para ver sus combos; pincha una familia de
   mano hecha para ver solo esas.
8. **Nodelock**: arrastra la barra de una familia — "las dobles parejas apuestan
   el 90%" — y vuelve a resolver. El resto del árbol se readapta alrededor de lo
   que fijaste, y cada lock lleva su ↺ para deshacerlo.

---

## Qué hace, y qué no

**Sí:**

- Resuelve **flop, turn o river** con rangos completos.
- Monta árboles con tamaños de apuesta y de subida por jugador y por calle,
  **donk bets**, ramas de **all-in** puro, y un "don't 3-bet" para IP.
- **Nodelocking** en cualquier nodo, combo a combo, con el resto del árbol
  resolviéndose alrededor.
- Para en un **objetivo de precisión** en vez de un número de iteraciones
  adivinado.
- **Rake** con tope, sobre el bote igualado.
- Guarda y carga **rangos** (los dos lados a la vez, porque un rango es un
  spot), **configs** (el montaje) y **árboles** (la solución entera, retomable).
- Te deja **jugar el árbol resuelto** contra la solución, con el consejo
  encendido o apagado, una nota al final y el nodo de salida que elijas.
- Resuelve **una lista de boards sin estar delante**: te escribe un script y lo
  dejas corriendo de noche.
- Habla **español o inglés**, cambiado al momento desde el engranaje, y se
  acuerda de la elección en tu navegador.

**No:**

- No juega ni resuelve **preflop**.
- No trae rangos de preflop ni solver de rangos de apertura.
- No hace botes multiway. Dos jugadores.
- No hace ICM, ni modo entrenamiento, ni nada multijugador.

---

## Cómo funciona

### El modelo

Dos jugadores, un board, sin preflop. La longitud del board elige la profundidad
del juego:

| board | juego |
|---|---|
| 5 cartas | solo river |
| 4 cartas | turn, y luego river |
| 3 cartas | flop, luego turn, luego river |

Todo lo que hay por debajo de la calle inicial es **azar y otra ronda de
apuestas**: un solve de flop contiene 49 cartas de turn, cada una con 48 de
river, cada una con su árbol de apuestas. Esa apertura — 49 × 48 = 2.352 runouts
ordenados, 1.176 parejas turn-river distintas — es donde se va toda la memoria y
todo el tiempo, y casi toda la ingeniería de aquí dentro es sobre ella.

### Rangos y una sola indexación

Un rango es un **vector de pesos sobre los combos que sobreviven al board base**,
y un cero significa simplemente "no está en mi rango". Los dos jugadores, todos
los nodos y todos los runouts usan esa misma indexación, así que nada del núcleo
de CFR tiene que traducir entre las listas de combos de dos jugadores.

En un board de tres cartas hay 1.176 combos (49 sobre 2). Cuando un nodo de azar
reparte una carta, los combos que la usan pasan a tener alcance cero de ahí para
abajo: no se quitan, dejan de existir en la aritmética.

Sintaxis de rangos:

```
AA  KK  AKs  AKo  AK          clases
QQ+  AJs+  KTo+               abiertas
55-88  T9s-76s  A5s-A2s       intervalos
AsKd                          un combo exacto
lo que sea:0.5                peso parcial
random / all                  todo
```

El diez es `T` o `10`, valen los dos. Lo que produce la rejilla es exactamente
esta sintaxis, así que va y viene con cualquier otra herramienta que la hable.

### El árbol de apuestas

El árbol de apuestas de una calle es **idéntico para todos los runouts que llegan
a ella** — mismo bote, mismos tamaños, misma forma — así que se construye una vez
como plantilla y la memoria de los conjuntos de información se indexa por
`(nodo de la plantilla, instancia de runout)`. Materializar 2.352 copias del
subárbol del river costaría cientos de megas solo en metadatos de nodos, antes de
guardar un solo regret.

Un **contexto** es una ronda de apuestas alcanzada por una línea de
continuaciones:

```
ctx 0        la calle inicial
ctx 1..n     la siguiente, una por continuación de ctx 0
...
```

y dentro de un contexto, una instancia es un runout:

```
instancia(hijo) = instancia(padre) * deckN + hueco_de_carta
```

así que el índice de instancia de cualquier nodo sale directo de las cartas
repartidas, sin tabla que consultar.

Los tipos de nodo son `DECISION`, `CONT` (el nodo de azar que pasa a la calle
siguiente), `SHOWDOWN` y `FOLD`. Las acciones son `Fold`, `Check`, `Call`, `Bet`,
`Raise`, escritas `F X C B R` con el tamaño pegado donde lo hay: `B33`, `R48`.

### Las reglas de construcción

Estas son las reglas que deciden qué contiene el árbol de verdad. Es la parte que
más gente quiere comprobar, así que están todas juntas aquí y todas cubiertas por
la batería.

- **Los tamaños de apuesta son por ciento del bote.** `50` es medio bote, `300`
  son tres botes.
- **Los de subida son `Nx`**: N veces lo que hay que pagar, encima de lo que ya
  llevas puesto. Sobre una apuesta de 100, `3x` son 300; si vuelven a subir, 700,
  y luego 1.500. `min` es la subida legal más pequeña y sale igual que `2x`. Es
  la cuenta estándar.
- **Todo se trunca a fichas enteras.** El 25% de un bote de 250 son 62,5 y el
  árbol construye 62. Truncar en vez de redondear hace que un tamaño nunca
  apueste *más* de lo que pediste. El all-in no se toca: si tienes 250,5 detrás,
  el all-in son 250,5. Un efecto secundario que conviene saber: un tamaño que
  caiga por debajo de una ficha no existe — con un bote de 20, el más pequeño que
  cabe es el 5%.
- **Umbral de all-in.** Una apuesta o subida que comprometa más del `allinpct`
  del **stack efectivo inicial** se convierte en all-in limpio. Por defecto 0,67:
  con dos tercios del stack en el medio ya estás comprometido, así que el tercio
  que queda detrás no compra ninguna decisión, solo ramas. `1` desactiva la
  regla; `0` convierte toda apuesta en all-in.
- **Los tamaños casi iguales se fusionan.** Dos tamaños a menos del 10% del mayor
  son la misma apuesta con pasos extra; gana el pequeño y la rama no se duplica.
  Esa constante se midió, no se eligió: con subidas `0.5,3x` sobre una apuesta de
  12 en un bote de 20 las dos caen en 34 y en 36, y el árbol pasa de 164 nodos a
  112 en cuanto se fusionan. Por debajo de 0,10 el casi-duplicado sobrevive y
  cuesta un tercio del árbol para nada; por encima de 0,20 empieza a comerse
  subidas que sí son distintas.
- **Las donk bets** son OOP liderando contra el agresor de la calle anterior, con
  sus propios tamaños. No existen en la calle en la que empieza el solve, porque
  nadie fue agresivo antes.
- **`no3bet`** impide a IP hacer la *tercera* acción agresiva de la calle (IP
  apuesta, OOP sube, IP para). Es un interruptor solo de IP.
- **Cota de profundidad.** No hay tope de apuestas y subidas de cara al usuario y
  no hace falta: cada subida es al menos tan grande como la que contesta, así que
  el stack se acaba, y el umbral de all-in corta la cadena antes todavía. Medido
  sobre un bote de 20, lo más profundo que llega una ronda son 3 niveles a 100bb
  y 10 a 2000bb. La cota dura de 24 niveles está por el tamaño que no termina en
  ningún sentido útil: una apuesta de una milésima del bote, subida al mínimo
  hasta el techo.
- **Cota de ramificación.** `MAX_ACTIONS = 8` por nodo. Si pides más, la
  construcción se rechaza nombrando el nodo, en vez de tirar una acción por lo
  bajo.

### Nodos de azar e isomorfismo de palos

En un nodo de azar,

```
v(h) = (1/D) · Σ  v(h | c)      para cada c repartible que no esté en h
```

con `D = 52 − |board| − 2`, y todos los combos que usan `c` con alcance cero por
debajo.

Dos cartas de runout son **estratégicamente idénticas** cuando alguna permutación
de los palos manda el board sobre sí mismo y una carta sobre la otra. Solo se
resuelve y se guarda una carta por órbita; las demás se leen permutando. Las
permutaciones que fijan un board son las que permutan palos con conjuntos de
rangos idénticos, así que la ganancia viene de los palos *ausentes* del board:

| flop | palos libres | grupo | ahorro |
|---|---|---|---|
| monótono (A♥9♥4♥) | 3 | 6 | ~6× |
| two-tone | 2 | 2 | ~2× |
| arcoíris | 1 | 1 | ninguno |

La trampa es que solo vale si **los rangos son invariantes bajo ese mismo grupo**
— y eso se comprueba, no se supone. Un rango con `AhKh` pero sin `AsKs` rompe la
simetría y el colapso se deja caer para ese grupo.

Está siempre activo (`set iso off` existe para probar). Lo que varía es cuánto
hay que ganar, y por eso la interfaz dice "palos colapsados ×6" en un board y
nada en otro.

### Discounted CFR

El solver es **Discounted CFR** (Brown y Sandholm, 2019) con los
hiperparámetros por defecto del paper:

| parámetro | valor | qué descuenta |
|---|---|---|
| α | 1,5 | el regret acumulado positivo |
| β | 0,0 | el regret acumulado negativo |
| γ | 2,0 | la estrategia media |

Detalles de implementación que importan para la velocidad y la memoria:

- **Solo dos buffers**: regret acumulado y estrategia acumulada, los dos en
  `float`. La estrategia actual se saca por regret matching al vuelo y la media
  se normaliza cuando se pide, así que ninguna de las dos cuesta un tercer
  buffer.
- **Actualizaciones alternas**: la iteración *t* recorre para un solo jugador. Es
  lo que usa el paper, parte por dos el coste de cada iteración, y quita la
  necesidad de congelar una estrategia actual compartida entre dos recorridos.
- **El bloque de un nodo lleva una entrada por acción y por combo que puede tener
  su dueño**, no por combo del board. Un board ofrece 1.176 y un rango de verdad
  tiene un par de cientos: eso solo vale como 6×.
- **Descuento diferido.** Descontar cada entrada al final de cada iteración
  significaría barrer el buffer entero — gigas — por una aritmética que solo
  importa donde el recorrido llega de verdad. En su lugar cada bloque lleva un
  sello de cuántos descuentos ha absorbido, y el recorrido lo pone al día cuando
  pasa por ahí. La suma de estrategias no se barre nunca: su descuento es un solo
  escalar aplicado al incremento.

### Memoria

Esto es lo que sorprende a todo el mundo. Un flop con rangos amplios, dos tamaños
de apuesta, subidas y all-in son fácilmente **2 o 3 GB**, y árboles más grandes se
van mucho más arriba. Es la naturaleza del problema, no un defecto.

El tamaño es exactamente:

```
bytes = 2 × 4 × Σ contextos [ instancias × Σ nodos de decisión (acciones × combos vivos del dueño) ]
```

El programa calcula eso **antes de pedir nada** y te lo enseña. El límite sale
por defecto del **75% de la RAM física** de la máquina — 24 GB en una de 32, 6 en
una de 8 — así que normalmente no hay nada que configurar. Si te pasas, la
construcción se rechaza con el número y una sugerencia de qué recortar.

Pedir un árbol que no cabe antes significaba que la reserva o petaba en un sitio
poco útil o, en Windows con un pagefile grande, tenía éxito calladita y dejaba la
máquina arrastrándose — que es peor, porque no hay nada que leer ni nada que
cancelar.

### Hilos

Un **pool de hilos persistente**, creado una vez. Crear hilos por sección
paralela costaba más que el trabajo en los árboles pequeños: una iteración de
turn se abre en cuatro puntos, así que con 16 hilos eran 64 creaciones para 2,4 ms
de trabajo.

La apertura de los nodos de azar es donde se paraleliza: los subárboles de un
mismo nodo de azar tocan memoria disjunta, así que no hay ningún cerrojo en el
camino caliente. El trabajo se reparte con un contador atómico, porque una
apertura desigual (un flop monótono tiene órbitas de tamaños muy distintos)
dejaría hilos parados. El hilo que llama participa como trabajador 0, así que
`threads = N` usa N núcleos contando el que pidió.

`set threads 0` es "todos los núcleos". Bájalo para dejar alguno libre para lo
que estés haciendo.

### Showdown

La fuerza de la mano depende de las cinco cartas finales, así que se precalcula
una vez por runout: la puntuación de cada combo y los combos ordenados por ella.
El valor de showdown de un rango entero contra otro es entonces **un barrido
O(N)** sobre ese orden, en vez de una comparación O(N²) de todas las parejas.

### Equity

La equity se calcula **por nodo**, contra el rango con el que el rival llega ahí
de verdad — que cambia con cada acción. La misma mano en el mismo board vale
números distintos en nodos distintos, y todos son correctos.

En `Ah9h4h`, AA vale el 85% contra el rango de partida, menos contra un rango que
pagó una apuesta, y menos todavía contra uno que subió. Si un número parece
raro, lo primero es mirar en qué nodo estás.

El cálculo base está verificado por tres caminos independientes: la
`compute_equity` del motor, una enumeración por fuerza bruta de todos los turns y
rivers (`tools/eqcheck.cpp`), y un solver de fuera.

### Explotabilidad y objetivo de precisión

El solver dice **por cuánto es explotable** la estrategia actual, en porcentaje
del bote. Más bajo es más cerca del equilibrio.

El número que se enseña es la **media de las dos mejores respuestas**, que es la
convención que usa todo solver publicado, así que un 0,5% aquí es un 0,5% en
cualquier parte. Por dentro el motor calcula la *suma*; el factor de dos se
aplica al enseñarlo y en el objetivo de precisión. Si comparas números crudos de
consola entre dos herramientas, ese factor es lo primero que hay que mirar.

**La explotabilidad no baja de forma monótona.** Es así en CFR en general y no es
un fallo. Medido aquí en un flop, pasó de 0,0042 a 1.810 iteraciones a 0,0618 a
1.880 — quince veces peor por hacer más trabajo — y tardó otras dos mil en
recuperarse. La estrategia media pondera más las iteraciones recientes (eso es γ),
así que sigue de cerca a una estrategia actual que todavía se está moviendo. Por
eso un objetivo de precisión es mejor criterio de parada que un contador de
vueltas, y el tope de iteraciones se entiende mejor como red de seguridad que
como plan.

Hay también un tope de **tiempo de reloj** (`set timeout`), que es lo que hace
que una lista de veinte boards de noche termine de verdad: sin él, un board que
no alcanza el objetivo se come las horas de los demás.

### Rake

Desactivado por defecto, porque un solve sin rake es el modelo correcto para un
torneo y para teoría pura. Para cash no es una funcionalidad que falte, sino una
equivocada: el rake empeora los calls marginales y mueve los umbrales de
continuación, y los botes pequeños lo notan más.

`set rake <pct>[,<cap>]` coge un porcentaje del bote **igualado** con un techo
opcional en fichas. Igualado importa: una apuesta no pagada vuelve a quien la
hizo antes de que la casa coja nada, exactamente como en una mesa.

### Nodelocking

Un lock fija lo que hace una parte del rango en un nodo, y el resto del árbol se
resuelve alrededor.

Dos reglas, las dos medidas:

- **Hacia arriba se recalcula todo.** Congelar un river cambia lo que vale
  apostar el turn, y el turn se entera. Bloqueando un river de 48, la apuesta del
  turn se mueve 0,019 de media; bloqueando los 48, 0,190 — diez veces más, con 48
  veces más futuro tocado.
- **Hacia abajo no se hereda nada.** Bloquear el turn deja los rivers libres.

Y la regla con la que todo el mundo se tropieza: **un lock pertenece al nodo
exacto donde estás, con la carta repartida dentro.** Bloquear el 3♠ del river no
bloquea el 3♥. Colócate primero en el runout que te interesa (`cd 3s`, o
pinchando la carta en el árbol).

En la interfaz puedes bloquear arrastrando la barra de una familia ("las dobles
parejas apuestan el 90%") o pintando combos concretos en la rejilla, con el peso
que elijas. Los locks se aplican en el siguiente solve, y cada uno se deshace por
separado.

### Manos hechas y proyectos

El reparto agrupa un rango en familias de mano hecha y familias de proyecto — y
son **dos listas independientes**, no una lista cruzada. Una mano que es set *y*
proyecto de color sale en las dos; no hay familia "set + flush_draw", porque eso
no es algo que nadie estudie.

Las familias son las de siempre, escritas como se escriben siempre, y la
escritura la comprueba la batería letra por letra (hay bastantes herramientas por
ahí que no se ponen de acuerdo entre `2nd_pair` y `2nd-pair` como para que valga
la pena fijarlo).

Las filas salen del **board**, no de tu rango: una familia posible en este board
se lista aunque no tengas ninguna. En A-K-6 eso significa "set, 0.0 combos", que
dice algo — aquí hay sets y tú no llevas ninguno — mientras que esconder las
vacías hace que "imposible" y "no tengo" se vean igual.

---

## La interfaz

### La web

El programa sirve una sola página HTML autocontenida por un socket en
`127.0.0.1`, desde un **servidor HTTP escrito a mano** — sin framework, sin CDN,
sin un solo recurso externo. La página son unas 4.000 líneas de HTML, CSS y
JavaScript a pelo, compiladas dentro del binario como una cadena.

Tiene: las rejillas 13×13 de rangos, el navegador del árbol, la rejilla de
estrategia coloreada por acción, la equity y el EV pintados dentro de las
casillas, el reparto de manos hechas y proyectos con barras arrastrables, el
diálogo de nodelock con pintado de combos por palos, los paneles de guardar y
cargar, y un engranaje con todas las opciones avanzadas — límite de memoria,
hilos, tope de iteraciones, tiempo, umbral de all-in, rake — más el cambio de
idioma.

Los dos idiomas viven en la misma página: el marcado está en español y una tabla
lo lleva a inglés, con un `MutationObserver` que traduce lo que aparece después.
Los mensajes del propio motor son bilingües también, elegidos en cada petición
por lo que pide la página, así que un error no llega nunca en el idioma
equivocado.

### La consola

Todo lo que hace la interfaz lo hace la consola, y algunas cosas solo las hace
ella (`lines`, `freqs`, `br`, `csv`). `solver` sin argumentos te da un prompt;
`help` lista los comandos; ver [Referencia](#referencia).

### Lo que escribe en disco

Al lado del binario, creado en el primer uso:

```
saves/
  ranges/NOMBRE.rng     los dos rangos, OOP e IP, en texto
  configs/NOMBRE.cfg    el montaje: board, rangos, tamaños, bote, stack (unos KB)
  trees/NOMBRE.tree     montaje + solución completa, retomable (cientos de MB o GB)
```

Son ficheros normales. Los `.rng` y los `.cfg` son texto que puedes leer y editar
con el bloc de notas. Los siete rangos de fábrica que trae el binario se escriben
en `saves/ranges/` **solo si esa carpeta no tiene ningún rango**, así que los
tuyos no se tocan y uno que borres no vuelve.

---

## Jugar el árbol

Mirar una estrategia y jugarla no son lo mismo. Delante de la rejilla todo parece
obvio; con una mano concreta en la mano, un bote y alguien que acaba de subir, ya
no.

**Jugar** te reparte una mano de tu rango, te sienta en el spot, y la solución
juega el otro asiento.

```
              DCFR Bot   IP   stack 204
                 [] []
                   16                      <- lo que acaba de apostar
         A♥  9♥  4♥  Q♣  [ ]     bote 87
                   0
                 A♠ K♦
              Hero   OOP · top_pair   stack 204

           [ Fold ]  [ Call 16 ]  [ Raise 48 ]
```

- **DCFR Bot juega la solución.** En cada nodo suyo tira un dado con las
  frecuencias de *su* mano. No juega para castigarte —no sabe lo que tienes—, y
  por eso una mano suelta no dice nada y cien lo dicen todo.
- **El consejo, cuando lo quieras.** La frecuencia y el EV de cada acción, para
  tu mano exacta, sacados del solve: los mismos números que pinta la rejilla.
  Apágalo y vas solo; apagado no sale del motor, así que no hay nada que espiar
  en la respuesta.
- **Se puntúa por EV, no por frecuencia.** Con una mano que el solver apuesta el
  70% de las veces, pasar **no** es un error si pasar vale lo mismo. Las acciones
  que se mezclan se mezclan *porque* valen lo mismo, y puntuar contra la
  frecuencia castigaría lo que la teoría llama indiferente, que es como se
  aprenden supersticiones. Cada decisión se puntúa como `EV(la mejor) − EV(la
  tuya)` para la mano que llevabas, en fichas. Eso es lo que cuesta dinero.
- **El repaso.** Al terminar la mano, una fila por decisión: qué hiciste, qué era
  lo mejor y cuánto costó.
- **Empezar donde quieras.** De fábrica la mano empieza donde empieza el solve.
  *Empezar en…* te baja por el árbol para entrenar un spot concreto —"el turn
  después de apostar y que me paguen"— una y otra vez. Ahí la mano se reparte con
  el rango que **llega a ese nodo**, no con el de partida, así que te tocan las
  manos que de verdad se juegan ahí.
- **La semilla.** Cada mano enseña la suya, y *Repetir esta mano* la vuelve a
  repartir: mismas cartas, mismo runout, mismo dado. Es la única forma de volver
  a la mano que destrozaste y ver cuánto valía la otra línea.

Tu nota es el EV que dejas por decisión, en porcentaje del bote. Por debajo del
0,5% es impecable; por encima del 5% hay una fuga que merece la pena buscar.

---

## Resolver muchos boards de noche

Un flop grande tarda minutos. Veinte tardan lo que tardan, y no vas a estar
delante. El botón **Varios boards…**, al lado de *Resolver*, te escribe un
script; el script lo lanzas tú desde una terminal:

```bash
solver --script mi-lista.txt
```

De principio a fin:

1. **Monta el spot** en pantalla como siempre — un board de ejemplo, los rangos,
   los tamaños, el bote y el stack. Eso es lo que va a usar *cada* board de la
   lista.
2. Abre **Varios boards…** y pega la lista, uno por línea. *Flops al azar* te la
   llena: ninguno repetido, y ninguno que sea otro con los palos cambiados de
   nombre — esos tienen la misma solución y costarían el doble para nada.
3. Di **cuándo parar** en cada board: por precisión, por tiempo, o por los dos.
   Gana el primero que llegue. El tope de tiempo es el que hace que la lista
   quepa en una noche.
4. **Generar**. Tu spot se guarda como config con el nombre que pongas y el
   script lo carga en su primera línea, así el fichero queda corto y legible y el
   spot es algo que puedes volver a abrir.
5. **Descargar**, y a correr. Cada board resuelto se guarda como un árbol en
   `saves/trees`. Por la mañana los abres desde **Guardar → Árboles guardados**.

El script es texto plano y son comandos de la consola, así que se puede editar:

```
load config mi-spot
set accuracy 0.5
set stopacc on
set timeout 600

echo === 1/2  Ah9h4h
board Ah9h4h
solve
save tree Ah9h4h

echo === 2/2  Kd7c2s
board Kd7c2s
solve
save tree Kd7c2s

echo === terminada la lista de 2 boards
```

`echo` pone la hora del reloj, que es justo el punto: por la mañana el log son mil
líneas y lo que hace falta saber es por qué board iba y a qué hora. Si la línea
final no está, es que no terminó — sin contar nada.

**El disco.** Un árbol de flop ocupa cientos de megas o gigas — el panel de
guardar te dice cuánto exactamente — y veinte boards son veinte veces eso. Si
solo quieres medir cuánto tarda la lista, genera el script sin guardar árboles.

---

## Rendimiento

Medido en la máquina del autor (Windows, MinGW, `-O3 -march=native`). Ejecuta
`solver --bench` para tener los números de la tuya en vez de creerte estos.

| spot | tiempo |
|---|---|
| river, dos tamaños | segundos |
| turn, dos tamaños | decenas de segundos |
| flop monótono, árbol por defecto | ~12 s hasta el 0,66% del bote |
| flop grande, rangos amplios | minutos |

`--bench` puede además `--save` una referencia y `--vs` compararse contra ella,
que es como se demuestra que un cambio no ha costado velocidad.

---

## Que esté bien

### La batería

```bash
solver --check
```

**Más de 1.480 asertos**, y devuelve distinto de cero si algo se movió. No es un
fichero de tests puesto al final: es el sitio donde están escritas las reglas del
motor. Comprueba el póker (la evaluación de manos contra una enumeración por
fuerza bruta, los nombres de las familias letra por letra, la equity por tres
caminos), las matemáticas (suma cero en cada nodo, las cotas de la mejor
respuesta, la convención de explotabilidad), el árbol (número de nodos, la
escalera de subidas, el truncado, la fusión, la supresión del donk), la fontanería
(guardar y cargar de ida y vuelta, el parseo de configs, todos los comandos de la
consola en una sesión recién abierta) y la interfaz (que cada texto en español
tiene el suyo en inglés, que la página escribe sus números por un solo
formateador, que el observador que traduce lo que aparece tarde está puesto de
verdad).

Los asertos se llaman como frases, así que un fallo se lee como lo que se rompió:

```
FAIL  and one deleted by hand never comes back
        con seis guardados se rellenaria el septimo
```

### Mutaciones

Una comprobación que pasa sin vigilar nada es peor que no tenerla, porque compra
una confianza que no existe. Así que cada comprobación de este proyecto se
**verifica rompiendo a propósito lo que vigila** y viendo que se pone roja. Varias
de las que hay se apretaron después de fallar esa prueba: una buscaba
`new MutationObserver` en cualquier sitio de la página y pasaba contenta con un
`false &&` delante; otra contaba notas plegadas en vez de exigir que todas las
largas lo estuvieran; otra miraba si el help mencionaba `echo` en vez de ejecutar
`echo`.

Los mensajes de commit dejan escrito qué se midió y qué se rompió para demostrar
que la comprobación funciona.

### Validación de fuera

El motor se pone además al lado de un solver comercial independiente en el mismo
spot, con el mismo árbol y los mismos rangos, y se comparan los números:

| | referencia | este solver |
|---|---|---|
| valor de juego, river | 50,396 | 50,3958 |
| valor de juego, turn con dos tamaños | 55,150 | 55,1494 |
| valor de juego, flop a river | 52,913 | 52,9137 |
| IP paga una apuesta de 33 | 0,8138 | 0,8138 |
| OOP resube sobre una subida | 0,1158 | 0,1154 |

Los árboles se comparan **nodo a nodo** — mismo número de nodos, mismas acciones
en cada uno, incluida la supresión del donk — porque dos solvers que construyen
árboles distintos difieren por razones que no tienen nada que ver con resolver.

Ese ejercicio encontró dos fallos reales del motor que ninguna comprobación
interna podía cazar, que es todo el argumento para hacerlo.

---

## Referencia

### Línea de comandos

```
solver --gui [puerto]   la interfaz en el navegador (por defecto 8777)
                        --no-open no abre el navegador
solver                  consola de texto interactiva
solver --script FICH    ejecuta comandos de consola de un fichero y sale
solver --bench [board]  tiempos de un spot estándar
                        --save NOMBRE guarda referencia, --vs NOMBRE compara
solver --check          comprobaciones; código de salida 1 si algo se movió
solver --help
```

Los comandos también se pueden mandar por tubería: `echo solve | solver`.

### Comandos de la consola

**Spot**

| comando | qué hace |
|---|---|
| `show` | board, rangos, tamaños, tamaño del árbol, locks |
| `board <cartas>` | 3 cartas = solve de flop, 4 = turn, 5 = river |
| `range oop\|ip <spec>` | pone un rango |
| `set pot\|stack <x>` | la geometría |
| `set bets [oop\|ip] [calle] <pct,..>` | tamaños de apuesta, % del bote |
| `set raises [oop\|ip] [calle] <Nx\|min,..>` | tamaños de subida |
| `set donks [calle] <pct,..>` | OOP liderando contra el agresor |
| `set no3bet [calle] on\|off` | IP no hace la tercera agresiva |
| `set allin [oop\|ip] [calle] on\|off` | rama de all-in puro |
| `set allinpct <f>` | umbral de all-in (0,67) |
| `set rake <pct>[,<cap>]` | corte de la casa sobre el bote igualado |
| `set maxmem <gb>` · `set threads <n>` · `set iso on\|off` | límites |
| `set accuracy <pct>` · `set stopacc on\|off` | objetivo de precisión |
| `set iters <n>` · `set timeout <segs>` | redes de seguridad |
| `build` · `estimate` | reconstruir, o solo decir el tamaño |
| `lines [fich]` · `freqs [n\|all\|fich]` | todas las líneas del árbol |
| `flops [n]` | flops al azar, ninguno isomorfo de otro |
| `echo <texto>` | una línea de log con la hora |

**Resolver, navegar, leer**

| comando | qué hace |
|---|---|
| `solve [n]` · `iterate <n>` · `expl` | ejecutarlo |
| `tree` · `ls` · `pwd` · `up` · `cd <acción\|carta\|/>` | moverse |
| `freq` · `hands [n]` · `combos [n]` · `grid [código]` | leerlo |
| `made` · `br [n]` · `runouts` | reparto, mejor respuesta, cartas |
| `report [fich]` · `csv <fich>` | volcarlo |

**Guardar y bloquear**

| comando | qué hace |
|---|---|
| `save range\|config\|tree <nombre>` · `load config\|tree <nombre>` | persistencia |
| `saves` · `delete config\|tree <nombre>` | gestionarlo |
| `lock <sel> <acc=p,...>` · `unlock [all]` · `locks` | nodelocking |

Selectores de lock: `NUTS`, `VALUE`, `BC`, `AIR`, una familia de mano hecha o de
proyecto (`lock two_pair B=90%`), o cualquier expresión de rango. Las acciones son
`F X C B R`, o el código exacto cuando la calle tiene dos tamaños (`B33=0.9`).

### Límites y valores por defecto

| | |
|---|---|
| `MAX_ACTIONS` | 8 por nodo |
| `MAX_RAISE_LEVELS` | 24 por calle |
| umbral de fusión | 10% |
| umbral de all-in | 0,67 del stack inicial |
| DCFR α / β / γ | 1,5 / 0,0 / 2,0 |
| límite de memoria | 75% de la RAM física |
| puerto | 8777 |

---

## Mapa del código

Todo en cabeceras, así que la compilación es una sola unidad de traducción.

| fichero | líneas | qué vive ahí |
|---|---|---|
| `src/main.cpp` | 211 | entrada, línea de comandos, límite de memoria de la máquina |
| `src/config.hpp` | 114 | cada ajuste y por qué su valor por defecto es el que es |
| `src/msg.hpp` | 45 | el mecanismo de mensajes bilingües |
| `src/cards.hpp` | 627 | cartas, evaluación, familias de mano hecha y proyecto |
| `src/deal.hpp` | 720 | combos, runouts, fuerza por runout, isomorfismo de palos |
| `src/range.hpp` | 285 | la sintaxis de rangos, leerla y escribirla |
| `src/tree.hpp` | 863 | el árbol: contextos, instancias, reglas de construcción |
| `src/solver.hpp` | 1.580 | Discounted CFR, el pool de hilos, la mejor respuesta |
| `src/report.hpp` | 768 | el reparto de la estrategia por clase y por familia |
| `src/session.hpp` | 2.613 | el objeto que manejan la interfaz y la consola |
| `src/console.hpp` | 1.171 | la consola de texto |
| `src/webui.hpp` | 1.477 | el servidor HTTP y los endpoints JSON |
| `src/webui_page.hpp` | 3.996 | la interfaz, página incluida |
| `src/default_spot.hpp` | 53 | el spot con el que arranca |
| `src/default_ranges.hpp` | 53 | los rangos de fábrica |
| `src/bench.hpp` | 390 | `--bench` |
| `src/check.hpp` | 11.568 | `--check` |

Los comentarios están en español; los identificadores, los mensajes y la
documentación, en inglés.

---

## Compilarlo tú

### CMake, en cualquier sitio

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

`-DPORTABLE=ON` quita `-march=native`, que es lo que quieres para un binario que
va a ejecutar otra persona.

### Windows, a mano

```bash
g++ -std=c++17 -O3 -march=native -Wall -Isrc -static -o solver src/main.cpp -lws2_32
```

```powershell
.\build.ps1 -Gui
```

El `-static` no es opcional en Windows: hay otras herramientas por ahí (Git,
Qt…) con una `libstdc++-6.dll` incompatible en el PATH, y sin él el binario
arranca con la que encuentre primero.

El `-Wall` tampoco es decoración: el aviso que trae es el que caza un `%` suelto
dentro de un `printf` sin argumentos, que es como la ayuda de la consola llegó a
imprimir `less than this 25235616201f the pot`. Compila sin un solo aviso; si
aparece uno, es que algo se rompió.

### Linux / macOS, a mano

```bash
g++ -std=c++17 -O3 -march=native -Wall -Isrc -pthread -o solver src/main.cpp
```

### CI

Cada push compila en **Ubuntu, macOS y Windows** y pasa la batería completa en
las tres. Etiquetar una versión publica los binarios de las tres:

```bash
git tag v0.1.0 && git push origin v0.1.0
```

---

## Limitaciones

- **Dos jugadores y postflop.** Sin preflop, sin multiway.
- **La memoria es el límite**, no el tiempo. Los árboles de flop grandes quieren
  más RAM de la que tienen la mayoría de los portátiles; empieza por un turn o un
  river para ver el programa funcionando.
- **La explotabilidad no es monótona en las iteraciones** — usa el objetivo de
  precisión.
- El binario de Windows **no está firmado**, así que SmartScreen avisa la primera
  vez.
- Los de macOS no están firmados ni notarizados, así que Gatekeeper protestará;
  si eso te importa, compílalo allí.
- La interfaz da por hecho una ventana de ordenador. Funciona en un móvil; no
  está diseñada para uno.

---

## Contribuir

Los issues y los pull requests son bienvenidos. Dos reglas de la casa, que son la
razón de que esto funcione:

1. **Medir primero.** Un cambio justificado con "debería ir más rápido" no está
   justificado. `--bench --save` antes, `--bench --vs` después.
2. **Cada arreglo trae una comprobación, y la comprobación hay que demostrarla.**
   Rompe el arreglo a propósito, mira la comprobación nueva ponerse roja, vuelve
   a ponerlo. Una comprobación que no caza su propia mutación no vale nada y hay
   que apretarla.

`solver --check` tiene que estar verde antes de commitear nada.

---

## Más documentación

- **[FAQ](docs/FAQ.md)** -- las preguntas que hace todo el mundo: precisión,
  memoria, por qué se trunca un tamaño, por qué la equity cambia de nodo a
  nodo, qué hace el colapso de palos.
- **Las cabeceras.** Cada regla del motor tiene al lado por qué es esa regla,
  con la medida que lo decidió. Ahí está la documentación de verdad.

---

## Licencia

GNU General Public License v3. Es software libre: puedes usarlo, estudiarlo y
modificarlo. Si distribuyes una versión modificada, tienes que publicar tu código
con la misma licencia.

Sin ninguna garantía. Ver [LICENSE](LICENSE).

Copyright © 2026 PagaLaFanta.
