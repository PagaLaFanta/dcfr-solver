# Preguntas frecuentes

## Lo básico

**¿Juega preflop?**

No. Le das un flop, un turn o un river ya repartido, más los rangos con los que
los dos jugadores llegan ahí. No hay solver de rangos de apertura ni rangos
preflop predefinidos.

**¿Cuánta memoria necesito de verdad?**

Depende del árbol, y mucho:

| spot | memoria aproximada |
|---|---|
| river, un tamaño de apuesta | decenas de MB |
| turn, dos tamaños | cientos de MB |
| flop, dos tamaños + subidas + all-in | **2 a 3 GB** |
| flop con muchos tamaños | más de 8 GB |

No es un defecto: un solver de flop replica el subárbol del river a lo largo de
1.176 runouts. La interfaz te dice cuánto va a ocupar **antes** de construirlo, y
si te pasas del límite te dice qué recortar. Baja los tamaños de apuesta antes
que los rangos: cada tamaño extra multiplica.

**¿Funciona en Mac o Linux?**

El código está escrito para los dos (hay rama POSIX además de la de Windows) y
hay Makefile y CMake. Se desarrolla y se prueba a diario en Windows; si lo
compilas en otro sitio y algo falla, es información útil y agradecida.

**¿Necesita internet?**

No. Corre entero en tu máquina y no habla con nadie. La interfaz es una página
web servida por el propio programa en `127.0.0.1`, que es la dirección de tu
ordenador y de nadie más.

---

## Precisión y resultados

**¿Es tan preciso como un solver comercial?**

En todo lo contrastado, sí. Mismo árbol, mismos rangos y mismo spot, los valores
de juego coinciden a 3-4 decimales en ocho spots distintos, y las estrategias
coinciden combo a combo. Ver la sección de validación del README.

Las diferencias que quedan caen siempre en manos **indiferentes** — aquellas
cuyas acciones valen lo mismo dentro de una centésima de ficha —, donde cualquier
mezcla es equilibrio y los dos solvers tienen razón.

**¿Por qué mi apuesta del 33% sale de 33 fichas y no de 33,4?**

Porque en una mesa no se apuesta media ficha. Los tamaños se **truncan a fichas
enteras**, como en cualquier otro solver: un 25% de un bote de 250 son 62,5 y
el árbol
construye 62.

Se trunca y no se redondea por dos razones: es lo que hacen los demás, y así nunca se
apuesta **más** de lo que has pedido.

El **all-in no se toca**: el tope es el stack que tú has puesto. Si tienes 250,5
detrás, el all-in son 250,5.

Un efecto secundario a tener en cuenta: un tamaño que dé **menos de una ficha**
ya no es una apuesta, y el solver lo rechaza diciéndolo en vez de dejarlo caer.
Con un bote de 20, el más pequeño que cabe es el 5%.

**La equity que veo no coincide con la de otro solver.**

Comprueba en qué **nodo** estás. La equity es contra el rango con el que el rival
llega **ahí**, y ese rango cambia con cada acción: el que paga una apuesta llega
con más mano, así que tu equity baja.

En `Ah9h4h`, AA vale:

| dónde | equity |
|---|---|
| contra el rango entero | **85,0%** |
| después de que el rival pase | 86,7% |
| enfrentando su apuesta | **74,4%** |

Los tres números son correctos y son de sitios distintos. El 85% está verificado
por tres caminos independientes: nuestro cálculo, una enumeración por fuerza
bruta de todos los turns y rivers (`tools/eqcheck.cpp`), y un solver comercial,
que da 0,8500.

**Mi explotabilidad no coincide con la de otro solver para la misma solución.**

Ojo con la convención, porque hay un factor de dos. Lo que se enseña fuera es
la **media** de
las dos mejores respuestas; internamente aquí se calcula la **suma**. El número
que ves en la interfaz y el objetivo de precisión ya están convertidos a la
convención de siempre, así que deberían coincidir. Si comparas números crudos de la
consola, divide entre dos.

**¿Por qué la explotabilidad sube a veces si le doy más iteraciones?**

Porque es así, y no es un fallo. La estrategia media pondera las iteraciones
recientes con más peso (el exponente gamma de DCFR), así que sigue de cerca a la
estrategia actual; cuando ésta hace una excursión — por ejemplo, porque una
acción que no usaba se vuelve rentable —, la media va detrás.

Medido aquí: de 0,0042 a 1.810 iteraciones a 0,0618 a 1.880, y otras dos mil
iteraciones en recuperarse. Está comprobado que es eso y no corrupción: el bache
cae en las mismas iteraciones con gamma 1, 2 y 3, y su tamaño crece con gamma.

**La consecuencia práctica**: fija un objetivo de precisión, no un número de
vueltas. Un contador no te dice dónde has caído.

**¿Qué son alpha, beta y gamma?**

Los tres descuentos del Discounted CFR, que es el algoritmo que usa esto. **No se
tocan desde la interfaz a propósito**: están fijos en los valores del artículo
original y cambiarlos solo altera a qué velocidad converge, no cuál es la
respuesta. Se explican aquí porque los vas a ver nombrados en la literatura.

- **alpha (1,5)** — cuánto se descuentan los arrepentimientos **positivos**. Alto
  significa que lo aprendido hace mucho pesa casi igual que lo de ahora.
- **beta (0)** — lo mismo para los **negativos**. En 0 se reducen a la mitad cada
  iteración: una acción que iba mal deja de arrastrar su historia y puede volver
  a probarse si el rival cambia.
- **gamma (2)** — cuánto pesan las iteraciones recientes en la **estrategia
  media**, que es la que se te muestra. Al cuadrado: lo de ahora manda.

Si quieres experimentar con ellos, están en la consola (`set alpha`, `set beta`,
`set gamma`).

---

## Uso

**¿Puedo usar los rangos que ya tengo?**

Sí, es el formato que se usa aquí: manos separadas por comas, las puras sin peso
y las parciales con `:0.5`.

```
AA,KK,QQ,AKs,A8o:0.5,KQ,KJ,K8o:0.5,Q5s:0.5,J9o:0.5
```

Se pega tal cual en la caja *Range text* y la rejilla se pinta sola. Y al revés:
lo que pintes en la rejilla sale escrito en ese mismo formato, listo para pegar
donde quieras o guardarlo donde guardes los demás.

Se aceptan además los atajos de siempre — `22+`, `A2s+`, `KTo+`, `55-88`,
`AsKd` para un combo concreto, `random` para todo — y el diez tanto como `T`
como `10`.

**Si bloqueo un river, ¿vale solo para esa carta?**

Sí: el lock pertenece al **nodo exacto donde estás, con
la carta repartida dentro**. Bloquear el 3s no bloquea el 3h. Colócate en el
runout que te interesa (`cd 3s`, o pinchando la carta en el árbol) antes de
bloquear.

Las dos reglas que lo acompañan, medidas:

- **Hacia arriba se recalcula todo.** Congelar un river cambia lo que vale
  apostar el turn, y el turn se entera. Bloqueando un river de 48 la apuesta del
  turn se mueve 0,019 de media; bloqueando los 48, 0,190 — diez veces más, con
  48 veces más futuro tocado.
- **Hacia abajo no se hereda nada.** Bloquear el turn deja los rivers libres:
  siguen resolviéndose, cada uno contra el rango que le llega del turn congelado.

Un lock sobre un turn o un flop no tiene esta distinción, porque esa decisión se
toma **una sola vez, antes** de que caiga ninguna carta.

**¿Qué es el nodelocking y cómo se usa aquí?**

Es fijar lo que hace una parte del rango en un nodo, para ver cómo se adapta el
resto del árbol. El flujo es el de siempre:

1. Resuelve.
2. Navega hasta el nodo que te interesa y mira la estrategia.
3. Abre **Nodelock**, elige la acción y **pinta** las manos que quieres fijar. Se
   pinta combo a combo y con el peso que elijas, no clase a clase.
4. Vuelve a resolver: lo pintado se queda quieto y el resto se readapta.

**El colapso de palos se apaga o se recorta si el lock nombra cartas concretas**
(`AsKs`), porque entonces los palos dejan de ser intercambiables. La interfaz te
lo dice. Si fijas por rangos (`QQ+`) no pierdes nada.

**¿Por qué a veces dice "suits collapsed x6" y otras x2 o nada?**

(El colapso está siempre activo; no es una opción. Lo que
varía es cuánto se puede aprovechar.)

Dos runouts que solo se diferencian en un intercambio de palos son la misma
decisión, así que se resuelve uno y el otro se lee permutando. Cuánto se ahorra
depende del board: un flop monótono deja tres palos libres (grupo de 6), un
two-tone deja dos (grupo de 2), un arcoíris ninguno.

Y depende de tus rangos: solo se colapsa por las permutaciones que **tus rangos
también sobreviven**. Un rango sin diamantes recorta el grupo.

**¿Puedo guardar un rango para no repintarlo cada vez?**

Sí. Debajo de la rejilla hay un nombre y los botones **Guardar** / **Cargar** /
**Borrar**. Guarda el lado que tengas abierto en las pestañas (OOP o IP).

El rango se guarda **sin el jugador dentro**, a propósito: uno guardado desde OOP
se puede cargar en IP, que es justo lo que quieres al montar el mismo spot desde
el otro lado. Son ficheros de texto diminutos en `saves/ranges/`, así que se
pueden editar con el bloc de notas o pasar a otra persona.

En la consola: `save range <nombre> oop|ip`, `load range <nombre> oop|ip`,
`delete range <nombre>`, y `saves` los lista.

**¿Puedo guardar un árbol resuelto?**

Sí, con el botón *Guardar tree*. Ocupa lo suyo — un flop resuelto son decenas de
megas — porque guarda los arrepentimientos y la estrategia acumulados, que es lo
que hace falta para seguir resolviendo donde lo dejaste. Las configuraciones se
guardan aparte y pesan medio kilobyte.

---

## Jugar contra la solución

**¿Cómo se puntúa el entrenador?**

Por **EV perdido**, no por frecuencia. En cada decisión tuya se mira lo que vale
cada acción **para la mano exacta que llevas** —los mismos números que pinta la
rejilla en modo EV— y se compara la que tomaste con la mejor. La diferencia, en
fichas, es lo que te costó.

Esto importa más de lo que parece. Con una mano que el solver apuesta el 70% de
las veces, **pasar no es un error** si pasar vale lo mismo: las acciones que se
mezclan se mezclan *porque* valen lo mismo. Puntuar contra la frecuencia
castigaría lo que la propia teoría llama indiferente, que es la forma más rápida
de aprender supersticiones.

La nota de la sesión es ese EV perdido por decisión, en porcentaje del bote:
menos del 0,5% es impecable, más del 5% es una fuga.

**El bot me pagó con nada y se llevó el bote. ¿Está roto?**

No. El bot juega la solución: en cada nodo suyo tira un dado con las frecuencias
de **su** mano. No sabe lo que tienes y no juega para castigarte, así que a veces
paga con la peor mano de su rango y liga. El resultado de una mano suelta no dice
nada —por eso el marcador no lo puntúa— y el EV perdido no depende de cómo caigan
las cartas.

**¿Por qué al empezar en un nodo de dentro me tocan otras manos?**

Porque la mano se reparte con el rango que **llega a ese nodo**, no con el de
partida. Si eliges el nodo de después de pagar una apuesta, te tocarán manos que
pagan. Repartir del rango de partida sería entrenar un spot que no se juega
nunca: la mitad de las manos no estarían ahí.

Las cartas que falten para llegar (el turn, el river) se reparten al azar en cada
mano, así que entrenas el nodo y no una carta concreta. Si quieres una carta fija,
resuelve ese board.

**Si cierro y vuelvo, ¿se guarda mi marcador?**

El marcador vive mientras el programa está abierto y se pone a cero con *Empezar
de cero*. No se guarda en disco: es para una sesión de estudio, no un historial.

**¿Puedo repetir la mano que acabo de destrozar?**

Sí, y es lo más útil del entrenador. Cada mano lleva una **semilla** a la vista y
*Repetir esta mano* la vuelve a repartir entera: mismas cartas tuyas, mismas del
bot, mismo runout. Juega la otra línea y compara lo que costó cada una.

## Licencia y contribuciones

**¿Puedo venderlo, o venderlo integrado en otra cosa?**

Está bajo la GPL v3. Puedes usarlo y modificarlo libremente, incluso con fines
comerciales, pero si **distribuyes** una versión modificada tienes que publicar
tu código con la misma licencia. No puedes cerrarlo.

**¿Cómo colaboro?**

Lee primero el README, que está escrito para eso, y los comentarios de las
cabeceras: cada regla del motor tiene al lado por qué es esa y no otra, con la
medida que lo decidió. Ahorra repetir callejones sin salida.

Y una costumbre del proyecto que conviene respetar: **nada se da por mejor sin
medirlo**, y los resultados negativos se escriben con sus números en vez de
borrarse.

`solver --check` tiene que quedarse en verde.
