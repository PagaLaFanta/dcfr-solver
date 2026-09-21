#pragma once
// =============================================================================
//  Los mensajes del motor, en los dos idiomas.
//
//  La interfaz esta entera en español o entera en ingles -- lo elige quien mira,
//  en el engranaje -- pero los errores salian del motor siempre en ingles. Una
//  pantalla en español que de pronto contesta "branching factor 12 exceeds
//  MAX_ACTIONS 8" no esta en español: esta a medias, y justo en el momento en el
//  que alguien necesita entender algo.
//
//  Aqui no hay tabla ni claves. Cada sitio dice sus dos textos donde esta el
//  error, que es donde se lee el codigo y donde se entiende que hace falta
//  decir:
//
//      e = M("no hay ninguna accion asi aqui", "no action like that here");
//
//  Y para los que llevan numeros dentro, se elige el FORMATO y se rellena luego,
//  que es la unica forma de que el numero caiga donde cae en cada idioma:
//
//      std::snprintf(b, sizeof b, M("hacen falta %.2f GB", "it needs %.2f GB"), gb);
//
//  El idioma es del PROCESO, no del sitio que escribe el mensaje: lo pone el
//  servidor con lo que le dice la pagina en cada peticion, y la consola lo deja
//  en ingles al arrancar, que es el idioma en el que esta la consola entera.
//  Dos pestañas abiertas en idiomas distintos se pisarian; es un programa que
//  corre en tu maquina y para ti, asi que se acepta.
// =============================================================================

namespace msg {

// true = ingles, que es como sale de fabrica el programa.
//
// El codigo y los comentarios estan en español porque los escribe quien los
// mantiene, pero esto se publica y quien lo abra puede estar en cualquier
// sitio: la primera pantalla tiene que entenderla todo el mundo. El español
// esta a un clic, en el engranaje, y el navegador se acuerda de la eleccion.
inline bool EN = true;

inline const char* pick(const char* es, const char* en) { return EN ? en : es; }

}  // namespace msg

// Corto a proposito: va dentro de los mensajes, y un nombre largo los parte en
// dos lineas y deja de leerse el texto, que es lo que hay que poder revisar.
#define M(es, en) msg::pick((es), (en))
