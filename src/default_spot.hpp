#pragma once
// =============================================================================
//  El spot con el que arranca el programa.
//
//  Antes arrancaba con un board y los dos rangos VACIOS, y un cartel que decia
//  "define los rangos de OOP y de IP". Para quien ya sabe son treinta segundos
//  de escribir; para quien acaba de bajarse el .exe es una pantalla que no hace
//  nada y ninguna pista de por donde se empieza. La primera impresion de un
//  solver no puede ser un formulario en blanco.
//
//  Va aqui dentro y no en saves/ a proposito: quien se baja el .exe se baja UN
//  fichero, sin carpetas al lado.
//
//  Los rangos, el board y el stack son los de SRP-IP-25BB, un BUvsBB a 25
//  ciegas. Los TAMANOS van recortados a proposito:
//
//    la config entera   230.417 nodos   2,91 GB   ~5 min a converger
//    esta                43.271 nodos   0,53 GB   ~1,7 min a 0,23% del bote
//
//  Abrir el programa cuesta 19 MB con cualquiera de las dos -- los buferes no se
//  reservan hasta que se solvea, medido -- asi que la diferencia se paga entera
//  en el primer clic en Solve: medio giga y minuto y medio, o tres gigas y cinco
//  minutos. Para estrenar el programa, lo primero.
//
//  Con un tamano por calle y subidas a 3x sigue siendo un arbol de poker de
//  verdad, no un juguete sin subidas, y la estrategia se ve formarse desde el
//  primer segundo. Quien quiera el spot completo lo tiene a un clic en
//  saves/configs.
// =============================================================================

inline const char* DEFAULT_SPOT = R"DEFSPOT(
# poker solver config
board Ah9h4h
oop K5s:0.82,KQo:0.65,JTs:0.51,T9s:0.13,T8s:0.93,T7s:0.33,A8o:0.58,K7o:0.46,K6o:0.85,A5o:0.16,A4o:0.06,A3o:0.23,KQs:0.85,K8o:0.95,Q8o:0.99,J8o:0.95,A7o:0.79,Q7o:0.95,J7o:0.87,T7o:0.97,76s:0.45,A6o:0.8,Q6o:0.95,K5o:0.91,Q5o:0.95,K4o:0.95,Q4o:0.91,K3o:0.89,K2o:0.92,A8s:1,A7s:1,A6s:1,A5s:1,A4s:1,A3s:1,A2s:1,KJs:1,KTs:1,K9s:1,K8s:1,K4s:1,K3s:1,K2s:1,QJs:1,QTs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,KJo:1,QJo:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:1,J2s:1,KTo:1,QTo:1,JTo:1,T6s:1,T5s:1,T4s:1,T3s:1,T2s:1,K9o:1,Q9o:1,J9o:1,T9o:1,97s:1,96s:1,95s:1,94s:1,93s:1,92s:1,T8o:1,98o:1,86s:1,85s:1,84s:1,83s:1,82s:1,97o:1,87o:1,75s:1,74s:1,73s:1,72s:1,96o:1,86o:1,76o:1,65s:1,64s:1,63s:1,62s:1,75o:1,65o:1,54s:1,53s:1,52s:1,64o:1,54o:1,43s:1,42s:1,32s:1
ip A4s:1,KQo:1,QTs:1,KJo:1,A9o:1,A8o:1,33:1,22:1,AA:1,AKs:1,AQs:1,AJs:1,ATs:1,A9s:1,A8s:1,A7s:1,A6s:1,A5s:1,A3s:1,A2s:1,AKo:1,KK:1,KQs:1,KJs:1,KTs:1,K9s:1,K8s:1,K7s:1,K6s:1,K5s:1,K4s:1,K3s:1,K2s:1,AQo:1,QQ:1,QJs:1,Q9s:1,Q8s:1,Q7s:1,Q6s:1,Q5s:1,Q4s:1,Q3s:1,Q2s:1,AJo:1,QJo:1,JJ:1,JTs:1,J9s:1,J8s:1,J7s:1,J6s:1,J5s:1,J4s:1,J3s:0.46,ATo:1,KTo:1,QTo:1,JTo:1,TT:1,T9s:1,T8s:1,T7s:1,T6s:1,T5s:1,T4s:0.88,K9o:1,Q9o:1,J9o:1,T9o:1,99:1,98s:1,97s:1,96s:1,95s:1,K8o:1,Q8o:1,J8o:1,T8o:1,98o:1,88:1,87s:1,86s:1,85s:1,A7o:1,K7o:1,87o:0.35,77:1,76s:1,75s:1,74s:0.89,A6o:1,K6o:0.22,66:1,65s:1,64s:1,A5o:1,55:1,54s:1,A4o:1,44:1,A3o:1,A2o:0.24000000000000002,97o:0.05,53s:0.17
pot 55
stack 220
alpha 1.5
beta 0
gamma 2
rake 0,0
allinpct 0.67
iters 3000
accuracy 1 1
threads 0
iso 1
street flop oop bets 30% raises 3x donks none allin 0
street flop ip bets 30% raises 3x allin 0 no3bet 0
street turn oop bets 66% raises 3x donks none allin 0
street turn ip bets 66% raises 3x allin 0 no3bet 0
street river oop bets 66% raises 3x donks none allin 0
street river ip bets 66% raises 3x allin 0 no3bet 0
)DEFSPOT";
