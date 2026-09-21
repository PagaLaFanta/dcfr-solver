#pragma once
// =============================================================================
//  Los rangos que vienen dentro del programa.
//
//  Quien se baja el .exe se encuentra la lista de rangos vacia, y montar un rango
//  de 25bb a mano es media hora antes de poder resolver nada. Estos son los del
//  autor, sacados de su trabajo y no de una tabla generica: LJ, HJ, CO y BU
//  contra BB y contra BU, a 25 ciegas.
//
//  Se escriben en saves/ranges la primera vez que se abre el programa, y SOLO si
//  esa carpeta no tiene ningun rango: los tuyos no se tocan, y uno que borres no
//  vuelve al arrancar. A partir de ahi son ficheros normales, se editan y se
//  borran como cualquier otro.
//
//  Cada uno lleva LOS DOS lados, OOP e IP, que es como se guardan: un rango es de
//  un spot concreto y solo dice algo con el del otro lado al lado.
// =============================================================================

struct RangoDeFabrica {
    const char* nombre;
    const char* texto;
};

inline const RangoDeFabrica DEFAULT_RANGES[] = {
{ "BUvsBB-25BB",
R"RNG(OOP K5s:0.82,KQo:0.65,JTs:0.51,T9s:0.13,T8s:0.93,T7s:0.33,A8o:0.58,K7o:0.46,K6o:0.85,A5o:0.16,A4o:0.06,A3o:0.23,KQs:0.85,K8o:0.95,Q8o:0.99,J8o:0.95,A7o:0.79,Q7o:0.95,J7o:0.87,T7o:0.97,76s:0.45,A6o:0.8,Q6o:0.95,T6o:0.84,K5o:0.91,Q5o:0.95,J5o:0.8,T5o:0.98,95o:0.99,K4o:0.95,Q4o:0.91,J4o:0.96,T4o:0.91,K3o:0.89,Q3o:0.84,J3o:0.95,T3o:0.98,K2o:0.92,Q2o:0.87,J2o:0.95,T2o:0.99,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,KJs:1,KTs:1,K9s:1,K8s:1,K4s:1,K3s:1,K2s:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,KJo:1,QJo:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:1,J2s:1,KTo:1,QTo:1,JTo:1,T6s:1,T5s:1,T4s:1,T3s:1,T2s:1,K9o:1,Q9o:1,J9o:1,T9o:1,97s:1,96s:1,95s:1,94s:1,93s:1,92s:1,T8o:1,98o:1,86s:1,85s:1,84s:1,83s:1,82s:1,97o:1,87o:1,75s:1,74s:1,73s:1,72s:1,J6o:1,96o:1,86o:1,76o:1,65s:1,64s:1,63s:1,62s:1,85o:1,75o:1,65o:1,54s:1,53s:1,52s:1,94o:1,84o:1,74o:1,64o:1,54o:1,43s:1,42s:1,93o:0.51,73o:1,63o:1,53o:1,43o:1,32s:1,92o:0.16,62o:0.99,52o:1,42o:1,32o:1
IP A4s:1,KQo:1,QTs:1,KJo:1,A9o:1,A8o:1,33:1,22:1,AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,K2s:1,AQo:1,QQ:1,QJs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,AJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:0.46,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,T6s:1,T5s:1,T4s:0.88,K9o:1,Q9o:1,J9o:1,T9o:1,99:1,98s:1,97s:1,96s:1,95s:1,K8o:1,Q8o:1,J8o:1,T8o:1,98o:1,88:1,87s:1,86s:1,85s:1,A7o:1,K7o:1,87o:0.35,77:1,76s:1,75s:1,74s:0.89,A6o:1,K6o:0.22,66:1,65s:1,64s:1,A5o:1,55:1,54s:1,A4o:1,44:1,A3o:1,A2o:0.24000000000000002,97o:0.05,53s:0.17
)RNG" },
{ "COvsBB-25BB",
R"RNG(OOP K6s:0.89,KQo:0.69,KJo:0.57,A8o:0.21,A7o:0.85,A4o:0.28,ATs:0.5,A8s:0.98,KQs:0.98,JTs:0.45,T9s:0.55,T8s:0.96,A9o:0.94,Q9o:0.96,J9o:0.98,K8o:0.87,Q8o:0.98,J8o:0.98,87s:0.58,K7o:0.97,J7o:0.89,T7o:0.97,76s:0.93,A6o:0.84,K6o:0.97,T6o:0.91,K5o:0.95,Q5o:0.93,J5o:0.87,K4o:0.96,Q4o:0.86,J4o:0.94,T4o:0.97,A3o:0.99,K3o:0.96,Q3o:0.9,A2o:0.86,K2o:0.98,Q2o:0.93,A9s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,KJs:1,KTs:1,K9s:1,K5s:1,K4s:1,K3s:1,K2s:1,QJs:1,QTs:1,Q9s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,QJo:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:1,J2s:1,KTo:1,QTo:1,JTo:1,T7s:1,T6s:1,T5s:1,T4s:1,T3s:1,T2s:1,K9o:1,T9o:1,97s:1,96s:1,95s:1,94s:1,93s:1,92s:1,T8o:1,98o:1,86s:1,85s:1,84s:1,83s:1,82s:1,Q7o:1,97o:1,87o:1,75s:1,74s:1,73s:1,72s:1,Q6o:1,J6o:1,96o:1,86o:1,76o:1,65s:1,64s:1,63s:1,62s:1,T5o:1,95o:1,85o:1,75o:1,65o:1,54s:1,53s:1,52s:1,84o:1,74o:1,64o:1,54o:1,43s:1,42s:1,J3o:1,T3o:0.45,73o:0.55,63o:1,53o:1,43o:1,32s:1,J2o:1,52o:1,42o:1,32o:1
IP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:0.23,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,T6s:1,A9o:1,K9o:1,Q9o:0.76,J9o:0.6,T9o:0.88,99:1,98s:1,97s:1,96s:1,A8o:1,88:1,87s:1,86s:1,85s:0.6699999999999999,A7o:1,77:1,76s:1,75s:1,A6o:0.31,66:1,65s:1,A5o:1,55:1,54s:1,44:1,33:1,K2s:0.03,K8o:0.02,64s:0.01,22:0.2
)RNG" },
{ "COvsBU-25BB",
R"RNG(OOP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:0.23,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,T6s:1,A9o:1,K9o:1,Q9o:0.76,J9o:0.6,T9o:0.88,99:1,98s:1,97s:1,96s:1,A8o:1,88:1,87s:1,86s:1,85s:0.6699999999999999,A7o:1,77:1,76s:1,75s:1,A6o:0.31,66:1,65s:1,A5o:1,55:1,54s:1,44:1,33:1,K2s:0.03,K8o:0.02,64s:0.01,22:0.2
IP AA:0.57,AQs:0.77,AJs:1,ATs:1,A9s:1,A8s:0.99,A5s:1,A4s:0.68,A3s:0.46,KK:0.56,KQs:1,KTs:0.61,K9s:0.92,K8s:0.91,KQo:0.95,QQ:0.67,QTs:1,Q9s:0.9,AJo:0.85,KJo:0.94,JJ:0.56,JTs:0.52,J9s:1,ATo:0.79,T9s:1,T8s:0.99,98s:1,87s:0.96,77:0.96,76s:0.95,66:0.92,55:0.98,44:1,33:1,QJo:0.86,K7s:0.79,K6s:0.97,Q8s:0.73,J8s:0.8,97s:0.96,65s:1,22:0.67,A2s:0.44,54s:0.27,86s:0.19,KTo:0.94,A6s:0.84,A7s:0.96
)RNG" },
{ "HJvsBB-25BB",
R"RNG(OOP AJs:0.19,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,KQs:1,KJs:1,KTs:1,K9s:0.66,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,K2s:1,KQo:0.75,QTs:1,Q9s:0.64,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,KJo:1,QJo:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:1,J2s:1,ATo:1,KTo:0.73,QTo:1,JTo:1,T9s:0.56,T8s:1,T7s:1,T6s:1,T5s:1,T4s:1,T3s:1,T2s:1,A9o:0.5,K9o:0.81,Q9o:0.94,J9o:1,T9o:1,98s:0.8,97s:1,96s:1,95s:1,94s:1,93s:1,92s:1,K8o:0.99,Q8o:0.96,J8o:0.96,T8o:1,98o:1,87s:0.96,86s:1,85s:1,84s:1,83s:1,82s:1,K7o:0.98,Q7o:1,J7o:0.93,T7o:1,97o:1,87o:1,76s:1,75s:1,74s:1,73s:1,72s:1,K6o:0.97,Q6o:0.92,J6o:1,T6o:1,96o:1,86o:1,76o:1,65s:1,64s:1,63s:1,62s:1,K5o:0.97,Q5o:0.86,J5o:0.97,85o:1,75o:1,65o:1,54s:1,53s:1,52s:1,A4o:1,K4o:1,Q4o:0.92,74o:1,64o:1,54o:1,43s:1,42s:1,A3o:0.94,Q3o:0.97,63o:1,53o:1,43o:1,32s:1,A2o:1,K2o:0.97,Q2o:0.98,J4o:1,95o:1,52o:1,QJs:1,A6o:0.79,A5o:0.93,42o:1,J3o:1,K3o:0.91,J2o:0.23,84o:0.17,A7o:0.66
IP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,A9o:1,99:1,98s:1,97s:1,96s:0.28,A8o:1,88:1,87s:1,86s:1,77:1,76s:1,75s:0.32,66:1,65s:1,55:1,44:1,33:0.13,A7o:0.27,K9o:0.2,Q5s:0.65,K4s:0.88
)RNG" },
{ "HJvsBU-25BB",
R"RNG(OOP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,A9o:1,99:1,98s:1,97s:1,96s:0.28,A8o:1,88:1,87s:1,86s:1,77:1,76s:1,75s:0.32,66:1,65s:1,55:1,44:1,33:0.13,A7o:0.27,K9o:0.2,Q5s:0.65,K4s:0.88
IP AA:0.57,AQs:0.77,AJs:1,ATs:1,A9s:1,A8s:0.99,A5s:1,A4s:0.68,A3s:0.46,KK:0.56,KQs:1,KTs:0.61,K9s:0.92,K8s:0.91,KQo:0.95,QQ:0.67,QTs:1,Q9s:0.9,AJo:0.85,KJo:0.94,JJ:0.56,JTs:0.52,J9s:1,ATo:0.79,T9s:1,T8s:0.99,98s:1,87s:0.96,77:0.96,76s:0.95,66:0.92,55:0.98,44:1,33:1,QJo:0.86,K7s:0.79,K6s:0.97,Q8s:0.73,J8s:0.8,97s:0.96,65s:1,22:0.67,A2s:0.44,54s:0.27,86s:0.19,KTo:0.94,A6s:0.84,A7s:0.96
)RNG" },
{ "LJvsBB-25BB",
R"RNG(OOP ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,KQs:1,KJs:1,KTs:1,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,K2s:1,KQo:1,QTs:0.83,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,AJo:0.43,KJo:1,QJo:0.97,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:1,J2s:1,ATo:1,KTo:0.95,QTo:0.89,JTo:1,T9s:0.72,T8s:1,T7s:1,T6s:1,T5s:1,T4s:1,T3s:1,T2s:1,A9o:0.89,K9o:0.83,Q9o:0.95,J9o:1,T9o:1,98s:1,97s:1,96s:1,95s:1,94s:1,93s:1,92s:1,K8o:1,Q8o:0.95,J8o:1,T8o:1,98o:1,87s:1,86s:1,85s:1,84s:1,83s:1,82s:1,K7o:0.94,Q7o:0.96,J7o:0.96,T7o:1,97o:1,87o:1,76s:1,75s:1,74s:1,73s:1,72s:1,K6o:0.88,Q6o:0.91,J6o:1,T6o:1,96o:1,86o:1,76o:1,65s:1,64s:1,63s:1,62s:1,K5o:1,Q5o:0.91,J5o:1,85o:1,75o:1,65o:1,55:0.73,54s:1,53s:1,52s:1,A4o:0.93,K4o:1,Q4o:0.99,74o:1,64o:1,54o:1,43s:1,42s:1,A3o:0.91,Q3o:1,63o:1,53o:1,43o:1,33:0.19,32s:1,A2o:1,K2o:0.99,Q2o:1,22:0.99,52o:0.97,QJs:1,A7o:0.77,A6o:0.96,A5o:1,K3o:0.95,K9s:0.34,AJs:0.82,95o:0.11,J4o:0.32,42o:0.04
IP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:0.75,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:0.88,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,ATo:1,KTo:1,QTo:0.75,JTo:0.43,TT:1,T9s:1,T8s:1,A9o:1,99:1,98s:1,97s:1,88:1,87s:1,86s:0.61,77:1,76s:1,66:1,65s:0.33,55:1,A8o:0.17,44:0.61
)RNG" },
{ "LJvsBU-25BB",
R"RNG(OOP AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:0.75,AQo:1,KQo:1,QQ:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:0.88,AJo:1,KJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,ATo:1,KTo:1,QTo:0.75,JTo:0.43,TT:1,T9s:1,T8s:1,A9o:1,99:1,98s:1,97s:1,88:1,87s:1,86s:0.61,77:1,76s:1,66:1,65s:0.33,55:1,A8o:0.17,44:0.61
IP AA:0.57,AQs:0.77,AJs:1,ATs:1,A9s:1,A8s:0.99,A5s:1,A4s:0.68,A3s:0.46,KK:0.56,KQs:1,KTs:0.61,K9s:0.92,K8s:0.91,KQo:0.95,QQ:0.67,QTs:1,Q9s:0.9,AJo:0.85,KJo:0.94,JJ:0.56,JTs:0.52,J9s:1,ATo:0.79,T9s:1,T8s:0.99,98s:1,87s:0.96,77:0.96,76s:0.95,66:0.92,55:0.98,44:1,33:1,QJo:0.86,K7s:0.79,K6s:0.97,Q8s:0.73,J8s:0.8,97s:0.96,65s:1,22:0.67,A2s:0.44,54s:0.27,86s:0.19,KTo:0.94,A6s:0.84,A7s:0.96
)RNG" },
};
