# Contributing

*English below · [Español](#en-español)*

Issues and pull requests are welcome. There are only two house rules, and they
are the reason this thing works at all.

## 1. Measure first

A change justified by "should be faster" is not justified.

```bash
solver --bench --save before
#  ... your change ...
solver --bench --vs before
```

Negative results are worth writing down with their numbers, not deleting. Half
the comments in the engine exist because something reasonable was tried and
measured and turned out to be wrong; that record is what stops the next person
trying it again.

## 2. Every fix brings a check, and the check must be proven

New behaviour, fixed bug, changed rule — it gets an assertion in
`src/check.hpp`. Then **break the thing on purpose** and watch the new check go
red. Put it back.

A check that does not catch its own mutation is worthless, and worse than
worthless, because it buys confidence that is not there. Several checks in here
were tightened after failing exactly that test: one looked for
`new MutationObserver` anywhere in the page and passed happily with a `false &&`
in front of it; one counted folded notes instead of requiring every long note to
be folded; one asserted that the help *mentions* `echo` rather than running
`echo`.

Say in the commit message what you measured and what you broke to prove the
check works.

## Before you open the PR

```bash
solver --check          # must be green: 1,480+ assertions, exit code 0
```

It builds without a single warning (`-Wall -Wextra`, `/W4`). Keep it that way —
the warning that catches a stray `%` in a `printf` is not decoration.

## Style

- Comments in Spanish, identifiers and messages in English. That is the existing
  split; it is not up for a vote, just for consistency.
- Comments are **ASCII**: accents live in user-facing strings, not in code
  comments.
- Any string the user sees needs both languages: `M("es", "en")` in the engine,
  and an entry in the `EN` table in `webui_page.hpp` for the page. The suite
  checks that nothing is left untranslated.
- Header-only. One translation unit. No new dependencies — that is a feature,
  not an oversight.

---

## En español

Los issues y los pull requests son bienvenidos. Solo hay dos reglas de la casa, y
son la razón de que esto funcione.

### 1. Medir primero

Un cambio justificado con "debería ir más rápido" no está justificado.

```bash
solver --bench --save antes
#  ... tu cambio ...
solver --bench --vs antes
```

Los resultados negativos se escriben con sus números en vez de borrarse. La mitad
de los comentarios del motor están ahí porque algo razonable se probó, se midió y
resultó ser falso; ese registro es lo que evita que el siguiente lo intente otra
vez.

### 2. Cada arreglo trae una comprobación, y la comprobación hay que demostrarla

Comportamiento nuevo, fallo arreglado, regla cambiada: lleva un aserto en
`src/check.hpp`. Luego **rompe a propósito lo que vigila** y mira la comprobación
nueva ponerse roja. Vuelve a ponerlo.

Una comprobación que no caza su propia mutación no vale nada, y es peor que nada,
porque compra una confianza que no existe. Varias de las que hay se apretaron
después de fallar justo esa prueba: una buscaba `new MutationObserver` en
cualquier sitio de la página y pasaba contenta con un `false &&` delante; otra
contaba notas plegadas en vez de exigir que todas las largas lo estuvieran; otra
miraba si el help *mencionaba* `echo` en vez de ejecutar `echo`.

Escribe en el mensaje del commit qué midiste y qué rompiste para demostrar que la
comprobación funciona.

### Antes de abrir el PR

```bash
solver --check          # tiene que estar verde: más de 1.480 asertos, salida 0
```

Compila sin un solo aviso (`-Wall -Wextra`, `/W4`). Que siga así: el aviso que
caza un `%` suelto dentro de un `printf` no es decoración.

### Estilo

- Comentarios en español, identificadores y mensajes en inglés. Es el reparto que
  ya hay; no se vota, solo se respeta.
- Los comentarios son **ASCII**: los acentos van en los textos que ve el usuario,
  no en los comentarios del código.
- Todo texto que vea el usuario necesita los dos idiomas: `M("es", "en")` en el
  motor, y una entrada en la tabla `EN` de `webui_page.hpp` para la página. La
  batería comprueba que no quede nada sin traducir.
- Todo en cabeceras. Una sola unidad de traducción. Sin dependencias nuevas: eso
  es una característica, no un descuido.
