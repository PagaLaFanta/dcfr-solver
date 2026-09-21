#pragma once
// =============================================================================
//  The single page served by WebUI, embedded in the binary.
// =============================================================================

const char* const WEBUI_PAGE = R"HTMLPAGE(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DCFR Solver</title>
<style>
  :root{
    --bg:#151821; --panel:#1d212c; --panel2:#252a37; --line:#333a4a;
    --fg:#e6e9ef; --dim:#98a0b3; --accent:#4c8bf5;
    /* La paleta de la rejilla, al estilo de la referencia: fondos claros con la
       letra en NEGRO encima. Antes eran tonos oscuros con la letra en gris, y
       sobre el rojo de apostar el nombre de la casilla no se leia.
       Y el fold en AZUL: en gris azulado se confundia con una casilla fuera de
       rango, que es justo lo contrario de lo que quiere decir. */
    --fold:#7fa8d4; --check:#6fbf85; --bet1:#f2ab99; --bet2:#e0806e; --bet3:#cc6352;
    --out:#828a99; --out2:#7d8593;
  }
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--fg);
       font:13px/1.45 "Segoe UI",system-ui,sans-serif}
  h2{font-size:12px;letter-spacing:.08em;text-transform:uppercase;color:var(--dim);
     margin:0 0 8px;font-weight:600}
  header{display:flex;align-items:center;gap:14px;padding:9px 14px;flex-wrap:wrap;
         background:var(--panel);border-bottom:1px solid var(--line)}
  header .title{font-weight:700;font-size:15px}
  /* El engranaje se va al extremo derecho y se lleva el panel con el. */
  header .gearwrap{margin-left:auto;position:relative}
  .gear{background:var(--panel2);border:1px solid var(--line);color:var(--dim);
        border-radius:6px;padding:3px 9px;font-size:15px;line-height:1;cursor:pointer}
  .gear:hover{color:var(--fg);border-color:var(--accent)}
  .gear.on{color:var(--fg);border-color:var(--accent)}
  .opts{position:absolute;top:30px;right:0;z-index:60;width:380px;
        max-width:calc(100vw - 24px);max-height:calc(100vh - 60px);overflow-y:auto;
        background:var(--panel);border:1px solid var(--line);border-radius:8px;
        padding:12px;box-shadow:0 10px 30px rgba(0,0,0,.45)}
  .opts h3{margin:0 0 6px;font-size:11px;letter-spacing:.08em;
           text-transform:uppercase;color:var(--dim);font-weight:600}
  .opts .lang{display:flex;gap:6px;margin-bottom:12px}
  .opts .lang button{flex:1}
  .opts .lang button.act{background:var(--accent);color:#0b0e13;border-color:var(--accent)}
  .tbar button.act{background:var(--accent);color:#0b0e13;border-color:var(--accent)}
  .tbar .teclas{font-size:11px;color:var(--dim);border:1px dashed var(--line);
                border-radius:10px;padding:2px 8px;cursor:help}
  .opts .sep{border-top:1px solid var(--line);margin:10px 0}
  .opts .kv{display:flex;justify-content:space-between;font-size:11px;
            color:var(--dim);margin:3px 0}
  .opts .kv b{color:var(--fg);font-weight:600}
  .board{display:flex;gap:4px}
  .pill{padding:3px 9px;border-radius:99px;font-size:11px;background:var(--panel2);
        color:var(--dim);border:1px solid var(--line)}
  .pill.on{background:#1d3a24;color:#7ee2a8;border-color:#2f6b41}
  .pill.busy{background:#3a2f1d;color:#e8c07e;border-color:#6b552f}
  .pill.err{background:#3a1d1d;color:#e88;border-color:#7a3a3a}
  /* Convergencia. Los escalones no son redondos por gusto: 5% es donde el
     usuario de esto dijo que una solucion ya sirve para aprender de ella, y 2%
     donde deja de haber discusion. Por encima del 20% no hay estrategia, hay
     CFR a medio arrancar -- un arbol parado en 7 iteraciones marcaba 211%. */
  .pill.cv0{background:#14301d;color:#7ee2a8;border-color:#2f6b41}
  .pill.cv1{background:#1d3a24;color:#a8e27e;border-color:#416b2f}
  .pill.cv2{background:#3a361d;color:#e2d27e;border-color:#6b642f}
  .pill.cv3{background:#3a2a1d;color:#e2a87e;border-color:#6b4a2f}
  .pill.cv4{background:#3a1d1d;color:#ff9d9d;border-color:#7a3a3a}
  main{display:flex;gap:12px;padding:12px;align-items:flex-start}
  /* Ancha para que quepan flop, turn y river en fila, pero que encoja en una
     pantalla pequena en vez de desbordarse: las cajas de calle van a 1fr y
     tienen min-width 0, asi que comprimen bien. */
  .col-l{flex:0 1 600px;min-width:340px;max-width:52vw}
  /* Plegar el montaje. Una vez hay solucion, board y rangos ya no se tocan y se
     estan llevando la mitad de la pantalla; la rejilla, que es lo que se mira,
     se queda en la otra mitad. Plegado, la solucion ocupa el ancho entero. */
  main.plegado .col-l{display:none}
  /* Y la rejilla se suelta. Tenia un tope de 600px que con el montaje al lado
     era lo que cabia; plegado, ese tope dejaba el hueco vacio y plegar no
     servia de nada. */
  main.plegado .grid.strat{max-width:860px}
  main.plegado .grid.strat .cell{font-size:17px}
  #setupBtn{padding:3px 10px;font-size:11px;border-radius:99px}
  #goTop,#stopTop{padding:3px 12px;font-size:11px;border-radius:99px}
  tr.pick{cursor:pointer}
  tr.pick:hover td{background:var(--panel2)}
  tr.pick.on td{background:var(--panel2);box-shadow:inset 0 0 0 1px var(--accent)}
  .lockstrip{font-size:11px;color:var(--dim);margin:0 0 8px;display:flex;
    align-items:center;gap:8px;flex-wrap:wrap;min-height:18px}
  .lockstrip.has{color:#ffd479}
  .lockstrip .mono{color:var(--fg)}
  .lockstrip a{color:var(--accent)}
  /* El detalle, cuando se pide: en columna, con tope de alto y su propio
     scroll. Asi ni abierto se come la pantalla. */
  .lockdet{flex-basis:100%;display:flex;flex-direction:column;gap:2px;
    max-height:120px;overflow-y:auto;margin-top:4px;
    padding:4px 6px;background:var(--panel2);border-radius:4px}

  .col-r{flex:1;min-width:0}
  /* Ventana estrecha: una columna debajo de la otra.

     MEDIDO a 375px de ancho: el montaje se planta en sus 340 de minimo y a la
     solucion le quedan veinte pixeles, asi que "resuelve para ver el reparto"
     sale una palabra por linea y la pagina parece rota. Por debajo de 760 --
     que es media pantalla de un portatil, o sea el solver a un lado y Discord
     al otro -- las dos columnas pasan a ocupar el ancho entero. */
  @media (max-width:760px){
    main{flex-wrap:wrap}
    .col-l{flex:1 1 100%;max-width:100%;min-width:0}
    .col-r{flex:1 1 100%}
    /* En una ventana estrecha, las trece columnas del dialogo de nodelock
       dejan casillas de veintipocos pixeles y "AKo" se queda sin la o.
       MEDIDO a 430 de ancho: con 11 cabe, con 14 no.
       La rejilla del montaje no lo necesita: sus casillas son mas anchas
       porque no comparten sitio con el panel de la derecha. */
    #lockDlg .grid .cell{font-size:11px}
    /* Y la del montaje igual. Ojo con la especificidad: `.grid .cell` a secas
       no vale aqui, porque la regla de 13px de mas abajo empata en peso y gana
       por ir despues. Un @media no anade peso ninguno. */
    main .grid .cell{font-size:11px}
  }
  .panel{background:var(--panel);border:1px solid var(--line);border-radius:6px;
         padding:12px;margin-bottom:12px}
  label{display:block;font-size:11px;color:var(--dim);margin:8px 0 3px}
  input[type=text],input[type=number],select{
    width:100%;padding:5px 7px;background:#0e1117;color:var(--fg);
    border:1px solid var(--line);border-radius:4px;font:inherit}
  input:focus,select:focus{outline:none;border-color:var(--accent)}
  .row{display:flex;gap:8px} .row>*{flex:1;min-width:0}
  /* An on/off switch is a box you tick, not a two-item dropdown. Same height as
     the inputs next to it so a mixed row still lines up. */
  .tick{display:flex;align-items:center;gap:7px;padding:6px 0;font-size:12px;
        cursor:pointer;user-select:none}
  .tick input{width:15px;height:15px;margin:0;accent-color:var(--accent);cursor:pointer}
  .tick.off{opacity:.45;cursor:default} .tick.off input{cursor:default}
  button{padding:6px 12px;background:var(--panel2);color:var(--fg);
         border:1px solid var(--line);border-radius:4px;cursor:pointer;font:inherit}
  button:hover{border-color:var(--accent)}
  button.primary{background:var(--accent);border-color:var(--accent);color:#fff;font-weight:600}
  button:disabled{opacity:.45;cursor:not-allowed}
  button.sm{padding:3px 8px;font-size:11px}
  .tabs{display:flex;gap:4px;margin-bottom:8px}
  .tabs button{flex:1;font-size:12px}
  .tabs button.act{background:var(--accent);border-color:var(--accent);color:#fff}

  /* Mas pequena y con la letra mas grande: con la columna a 600px la celda
     salia de unos 45px con texto de 9, o sea casi todo hueco. */
  .grid{display:grid;grid-template-columns:repeat(13,1fr);gap:1px;user-select:none;
        max-width:430px}
  /* Dos rejillas comparten esta clase: la de ESTRATEGIA, con los colores de
     accion claros detras, y la de RANGOS del montaje, que es oscura con las
     casillas elegidas en azul. Les puse la letra negra a las dos de una vez y
     la del montaje se quedo ilegible. Aqui va lo comun; el color de la letra,
     cada una el suyo. */
  .grid .cell{position:relative;aspect-ratio:1;border-radius:2px;background:#0e1117;
              display:flex;align-items:center;justify-content:center;
              font-size:13px;font-weight:600;color:#7b8496;cursor:pointer;overflow:hidden}
  .grid .cell.on{color:#fff}
  .grid .cell .lab{position:relative;z-index:2;text-shadow:0 1px 2px rgba(0,0,0,.9)}
  /* Y la de estrategia, en negro sobre los colores de accion, como en la referencia. El
     halo es claro porque lo de debajo es claro. */
  .grid.strat .cell, .grid.strat .cell.on{color:#12161d}
  .grid.strat .cell .lab{text-shadow:0 1px 2px rgba(255,255,255,.5)}
  .grid .cell .fill{position:absolute;inset:0;z-index:1}
  /* La rejilla de estrategia se lleva el ancho que haya. A 600 se quedaba
     corta en una pantalla normal -- el panel mide casi 800 y sobraban
     doscientos pixeles a la derecha -- y es la rejilla que se mira todo el
     rato, con un nombre y un numero dentro de cada casilla. Plegado el
     montaje sigue habiendo mas, que es para lo que esta el pliegue. */
  .grid.strat{max-width:760px}
  /* La casilla de estrategia, como la pinta la referencia.

     - el nombre ARRIBA A LA IZQUIERDA y no centrado. Centrado, el color
       encogido le caia justo encima y con pesos pequenos no habia quien
       leyera la mano.
     - el fondo, GRIS. Era casi negro (#0e1117), asi que una mano que SI se
       juega pero con poco peso quedaba como un sello de color flotando en un
       agujero negro, y la rejilla entera se veia sucia.

     El color lo pone .fill encima, a lo ancho entero y creciendo desde
     abajo. */
  .grid.strat .cell{font-size:14px;background:var(--out);
                    align-items:flex-start;justify-content:flex-start}
  .grid.strat .cell .lab{padding:2px 0 0 4px}
  /* El numero dentro de la casilla, abajo a la derecha, cuando la rejilla
     pinta equity o EV -- como lo ensena la referencia. El color ya dice "esta
     verde", pero la pregunta que sigue siempre es CUANTO, y estaba a un hover
     de distancia, casilla a casilla. */
  .grid.strat .cell .val{position:absolute;right:3px;bottom:1px;z-index:2;
                         font-size:9px;font-weight:700;line-height:1;
                         font-variant-numeric:tabular-nums;
                         text-shadow:0 1px 2px rgba(255,255,255,.45)}
  main.plegado .grid.strat .cell .val{font-size:11px;right:4px;bottom:3px}
  .grid.strat .cell.sel{outline:2px solid var(--accent);outline-offset:-2px;z-index:3}
  /* The nodelock dialog. Others open one over the tree, and it is the right
     shape for the job: the grid is the control, not a text field beside it. */
  /* Las dos filas que estructuran el reparto por categorias: la cabecera de
     bloque y el total de cada bloque. Son dos bloques porque la referencia tiene dos
     listas independientes, y cada una reparte el rango entero, asi que hay dos
     totales del 100% y eso hay que dejarlo claro de un vistazo. */
  tr.catsub td{padding-top:10px;color:var(--dim);text-transform:uppercase;
               letter-spacing:.08em;font-size:11px;border-bottom:1px solid var(--line)}
  tr.cattot td{border-top:1px solid var(--line);color:var(--dim);font-weight:600}
  tr.vac td{color:var(--dim)}
  tr.catsub:hover td, tr.cattot:hover td{background:none}
  /* La barra de familia: la celda ES el control. Se arrastra de izquierda a
     derecha y lo que marca es lo que esa familia hara con esa accion.

     Antes era un relleno translucido detras de un numero, y no parecia un
     control: parecia una celda con el fondo sucio. Ahora es un medidor con su
     carril, su relleno del color de la accion, y un tirador que aparece al
     pasar por encima justo donde se agarra. */
  td.fambar{position:relative;cursor:ew-resize;user-select:none;padding:3px 4px}
  .ftrack{position:relative;display:block;height:17px;border-radius:3px;
          background:var(--out);overflow:hidden}
  .ftrack .ffill{position:absolute;left:0;top:0;bottom:0;opacity:.62}
  .ftrack .fedge{position:absolute;top:0;bottom:0;width:2px;margin-left:-1px;
                 background:var(--fg);opacity:0}
  .ftrack .fnum{position:absolute;right:5px;top:0;line-height:17px;font-size:11px;
                font-weight:600;font-style:normal;color:var(--fg);
                font-variant-numeric:tabular-nums}
  td.fambar:hover .fedge{opacity:.9}
  td.fambar:hover .ftrack{box-shadow:inset 0 0 0 1px var(--accent)}
  /* Arrastrando: el numero grande va con el raton, porque mirando la celda no
     se lee -- la mano tapa justo esa esquina. */
  body.arrastrando{cursor:ew-resize}
  body.arrastrando tr.pick:hover td{background:none}
  #fambadge{position:fixed;z-index:80;display:none;pointer-events:none;
            padding:3px 8px;border-radius:5px;background:var(--panel2);
            border:1px solid var(--accent);color:var(--fg);font-size:13px;
            font-weight:700;font-variant-numeric:tabular-nums;
            box-shadow:0 6px 18px rgba(0,0,0,.5)}
  /* El peso de la familia, en barra: cual es grande y cual son cuatro combos se
     ve antes en un ancho que en una columna de numeros. Va en proporcion a la
     familia mas grande de la tabla, que si no las de 3% no se distinguen. */
  .wtrack{position:relative;display:block;height:14px;border-radius:3px;
          background:var(--out)}
  .wtrack i{position:absolute;left:0;top:0;bottom:0;border-radius:3px;
            background:var(--accent);opacity:.45}
  .wtrack em{position:absolute;right:4px;top:0;line-height:14px;font-size:10px;
             font-style:normal;color:var(--fg);font-variant-numeric:tabular-nums}
  /* La cabecera dice de que color es cada accion, para no tener que adivinarlo
     mirando la rejilla. */
  .achip{display:inline-block;width:8px;height:8px;border-radius:2px;
         margin-right:5px;vertical-align:0}
  #madeTable td{font-variant-numeric:tabular-nums}
  /* Bloqueada: la barra vuelve al numero del solve que hay en pantalla -- el
     lock no se aplica hasta el siguiente -- y sin marca parece que el arrastre
     no hizo nada. */
  /* La version, pequena y apagada: hace falta para dar soporte, no para
     mirarla. */
  .ver{color:var(--dim);font-size:11px;margin-left:6px;letter-spacing:.02em}
  /* Bloqueada y ya aplicada: azul. Bloqueada y SIN aplicar: ambar, el mismo
     color del boton de Resolver cuando le falta un solve. Son dos estados
     distintos y antes se veian igual. */
  td.fambar.lk .ftrack{box-shadow:inset 3px 0 0 var(--accent)}
  td.fambar.pend .ftrack{box-shadow:inset 3px 0 0 var(--warn,#e0b33a)}
  td.fambar.pend .fnum{color:var(--warn,#e0b33a)}
  /* El boton de resolver cuando hay locks puestos que todavia no lleva lo que
     ves: es el sitio donde se mira, y con la pildora sola no bastaba. */
  button.primary.pend{background:var(--warn,#e0b33a);color:#1a1200;
                      box-shadow:0 0 0 2px rgba(224,179,58,.35)}
  button.undo{background:none;border:0;color:var(--accent);cursor:pointer;
              font-size:13px;padding:0 3px;line-height:1}
  button.undo:hover{color:var(--fg)}
  td.fambar.lk .fnum{font-weight:700}
  #szDlg{position:fixed;inset:0;z-index:70;background:rgba(6,8,12,.72);
         display:none;align-items:flex-start;justify-content:center;overflow:auto;padding:24px}
  #szDlg.on{display:flex}
  #szBox{background:var(--panel);border:1px solid var(--line);border-radius:8px;
         padding:16px;max-width:640px;width:100%;box-shadow:0 18px 60px rgba(0,0,0,.6)}
  #szBox h2{margin:0 0 10px}
  #szBox code, .note code{background:#0e1117;border:1px solid var(--line);
              border-radius:3px;padding:1px 5px;color:var(--accent)}
  #szBox td{vertical-align:top;padding-right:10px}
  #lockDlg{position:fixed;inset:0;z-index:60;background:rgba(6,8,12,.72);
           display:none;align-items:flex-start;justify-content:center;overflow:auto;padding:18px}
  #lockDlg.on{display:flex}
  #lockBox{background:var(--panel);border:1px solid var(--line);border-radius:8px;
           padding:14px;max-width:1080px;width:100%;box-shadow:0 18px 60px rgba(0,0,0,.6)}
  #lockBox h2{margin:0 0 10px}
  .lkacts{display:flex;gap:6px;margin-bottom:10px;flex-wrap:wrap}
  .lkact{flex:1 1 0;min-width:120px;border:2px solid transparent;border-radius:5px;
         padding:7px 9px;cursor:pointer;color:#0d1014;font-weight:600;line-height:1.25}
  .lkact.on{border-color:#fff}
  .lkact small{display:block;font-weight:400;opacity:.85;font-size:11px}
  .lkwrap{display:flex;gap:14px;align-items:flex-start;flex-wrap:wrap}
  .lkleft{flex:1 1 460px;min-width:320px}
  .lkright{flex:1 1 300px;min-width:260px}
  .grid.strat .cell.pick{outline:3px solid #ffe14d;outline-offset:-3px;z-index:4}
  #lockSlider{width:100%}
  .lkbig{font-size:20px;font-weight:600;text-align:center;margin:2px 0 8px}
  .lksuits{display:flex;gap:4px;margin:3px 0}
  .lksuit{width:26px;height:24px;border:1px solid var(--line);border-radius:4px;
          display:flex;align-items:center;justify-content:center;cursor:pointer;
          font-size:14px;background:#0e1117;opacity:.35}
  .lksuit.on{opacity:1;border-color:var(--accent);background:#161b24}
  .lkcmbs{display:flex;flex-wrap:wrap;gap:4px;margin:3px 0 6px;min-height:26px}
  .lkcmb{border:1px solid var(--line);border-radius:4px;padding:3px 6px;cursor:pointer;
         font-size:11px;font-family:ui-monospace,Menlo,Consolas,monospace;background:#0e1117;
         display:flex;align-items:center;gap:5px}
  .lkcmb.on{border-color:#ffe14d;background:#20242c}
  .lkcmb.ed{box-shadow:inset 0 0 0 1px #ffe14d55}
  .lkcmb .step{width:16px;text-align:center;cursor:pointer;border-radius:3px;
               background:#1b212b;user-select:none}
  .lkcmb .step:hover{background:#2a3341;color:#fff}
  .lkcmb .lkpct{min-width:30px;text-align:right;color:var(--dim)}
  .lkcmb .mini{width:26px;height:8px;border-radius:2px;overflow:hidden;display:flex}
  /* Selection has to be visible at a glance and has to show HOW MUCH of a
     class is in, not just that some of it is -- a thin outline on a dark grid
     is invisible, which is what "no se ve" meant. The reference washes the square in
     yellow in proportion to the weight; so does this. */
  .grid.strat .cell .pickwash{position:absolute;inset:0;z-index:2;pointer-events:none;
                              background:#ffe14d}
  .grid.strat .cell.picked{outline:2px solid #ffe14d;outline-offset:-2px;z-index:4}
  /* A pending, uncommitted change to this square. */
  .grid.strat .cell .editmark{position:absolute;top:0;right:0;z-index:3;
       width:0;height:0;border-top:6px solid #ffe14d;border-left:6px solid transparent}
  .grid .cell.lk::after{content:"";position:absolute;top:1px;right:1px;width:4px;height:4px;
                        border-radius:50%;background:#ffd479;z-index:3}
  .legend{display:flex;gap:12px;flex-wrap:wrap;margin:8px 0;font-size:11px;color:var(--dim)}
  .legend i{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:4px;
            vertical-align:-1px}

  /* A line browser: the line runs left to right, and the alternatives
     at the selected decision point hang underneath it. */
  .ptree{display:flex;align-items:flex-start;gap:3px;overflow-x:auto;padding:2px 0 4px}
  .pcol{display:flex;flex-direction:column;gap:3px;flex:none}
  .pcell{padding:4px 10px;border:1px solid #2a2f3d;border-radius:3px;cursor:pointer;
         font:600 12px/1.25 "Segoe UI",system-ui,sans-serif;white-space:nowrap;
         color:#16181d;background:#e9ecf2;text-align:center;min-width:60px}
  /* Una celda que lleva una carta dentro no necesita el relleno de una
     pastilla de texto: la carta ya tiene su forma. */
  .pcell.pcard{padding:2px;min-width:0;background:transparent;border-color:transparent}
  .pcell.pcard.sel{background:transparent;box-shadow:none}
  .pcell.pcard.sel .bcard{outline:2px solid #ffe14d;outline-offset:1px}
  .navboard{display:flex;gap:4px;align-items:center;margin-bottom:8px;min-height:33px}
  .pcell:hover{filter:brightness(1.08)}
  .pcell.sel{background:#ffe14d;box-shadow:inset 0 0 0 2px #b58900}
  .pcell.root{background:#e8a58a}
  .pcell.chk{background:#a8d5a0}
  .pcell.fold{background:#8ab4d8}
  .pcell.bet0{background:#c07f5f}
  .pcell.bet1{background:#e0a184}
  .pcell.bet2{background:#f0c4ae}
  .pcell.chance{background:#fbfbfd;font-family:ui-monospace,Consolas,monospace}
  .pcell.chance.red{color:#d02020}
  .pcell.alt{opacity:.92}
  .pcell.ghost{background:#3a3f4d;color:#c7cddb;border-style:dashed;cursor:default}
  .pcell.ghost:hover{filter:none}
  /* four-colour deck: spades black, hearts red, diamonds blue, clubs green */
  .deck{display:flex;flex-direction:column;gap:3px;margin-top:6px}
  .deckrow{display:grid;grid-template-columns:repeat(13,1fr);gap:3px}
  .pc{position:relative;aspect-ratio:.72;border-radius:4px;background:#f5f7fb;
      border:1px solid #c3cad6;cursor:pointer;display:flex;flex-direction:column;
      align-items:center;justify-content:center;line-height:1;user-select:none;
      font:700 11px/1 "Segoe UI",system-ui,sans-serif;transition:transform .07s}
  .pc .su{font-size:12px;margin-top:1px}
  /* La carta es un rectangulo blanco con el rango y el palo dentro, y lo que se
     lee es el rango y el palo. Con 11px en una carta de 60 casi todo era hueco.
     Acotado a .deck porque en la baraja las cartas se pintan mayores.

     Y `.pc` es SOLO la carta. Nombraba ademas un porcentaje en el dialogo de
     nodelock -- el mismo nombre para dos cosas -- y la regla global se colaba
     en la otra: un span de porcentaje con ese nombre salia como un cuadro
     blanco con forma de carta dentro del boton de accion. El porcentaje del
     dialogo se llama ahora `lkpct`. Esta clase la pone cardFace() y nadie mas
     la escribe a mano, que es lo que vigila la comprobacion. */
  .deck{max-width:470px}
  .deck .pc{font-size:19px}
  .deck .pc .su{font-size:17px;margin-top:2px}
  .pc.s{color:#1b1f27} .pc.h{color:#d92b2b}
  .pc.d{color:#2f6fe0} .pc.c{color:#1a9257}
  .pc:hover{transform:translateY(-2px);border-color:var(--accent);z-index:2}
  .pc.sel{transform:translateY(-3px);border-color:#fff;
          box-shadow:0 0 0 2px var(--accent),0 4px 10px rgba(0,0,0,.45)}
  .pc.dead{opacity:.22;cursor:not-allowed;transform:none;border-color:#c3cad6}
  .pc .pos{position:absolute;top:-6px;right:-4px;min-width:14px;height:14px;
           border-radius:7px;background:var(--accent);color:#fff;font-size:9px;
           display:flex;align-items:center;justify-content:center;padding:0 3px}
  .pc.taken{box-shadow:0 0 0 2px #ffe14d;border-color:#ffe14d}

  .bslots{display:flex;gap:14px;align-items:center;flex-wrap:wrap;margin-bottom:4px}
  .bgroup{display:flex;gap:4px;align-items:center}
  .bgroup .lab{font-size:10px;color:var(--dim);text-transform:uppercase;
               letter-spacing:.07em;margin-right:2px}
  .bcard{width:30px;height:42px;border-radius:4px;background:#f5f7fb;flex:none;
         border:1px solid #c3cad6;display:flex;flex-direction:column;
         align-items:center;justify-content:center;line-height:1;cursor:pointer;
         font:700 13px/1 "Segoe UI",system-ui,sans-serif}
  .bcard .su{font-size:14px;margin-top:1px}
  .bcard.s{color:#1b1f27} .bcard.h{color:#d92b2b}
  .bcard.d{color:#2f6fe0} .bcard.c{color:#1a9257}
  .bcard.empty{background:transparent;border-style:dashed;border-color:var(--line);
               cursor:default}
  .bcard.dealt{border-style:dashed;border-color:var(--accent);
               box-shadow:0 0 0 1px rgba(76,139,245,.35)}
  .bcard.sm{width:24px;height:33px;font-size:11px}
  .bcard.sm .su{font-size:11px}

  table{width:100%;border-collapse:collapse;font-size:12px}
  th,td{padding:4px 7px;text-align:right;border-bottom:1px solid var(--line);
        font-variant-numeric:tabular-nums}
  th{color:var(--dim);font-weight:600;font-size:11px;text-align:right}
  th:first-child,td:first-child{text-align:left}
  tbody tr:hover{background:var(--panel2)}
  .mono{font-family:ui-monospace,Consolas,monospace}
  .note{font-size:11px;color:var(--dim);margin-top:6px}
  /* Las notas largas: una linea, y el resto en un dialogo cuando se pide.

     Explicar esta bien, pero no a costa de que la pantalla parezca un manual:
     seis parrafos de ayuda alrededor de los campos hacen que no se lea ninguno.

     La primera version desplegaba el resto al pasar el raton por encima, y eso
     se dispara solo: mueves el raton para llegar a un campo, el parrafo se abre,
     y lo que hay debajo se mueve. Ahora es un enlace -- azul y con la manita,
     que es como se ve que algo se pincha -- y el detalle sale en un dialogo. */
  .note.nx .det{display:none}
  .note.nx .q{display:inline-block;margin-left:6px;width:14px;height:14px;
              line-height:13px;text-align:center;border:1px solid var(--accent);
              border-radius:50%;color:var(--accent);font-size:10px;font-weight:700;
              font-style:italic;cursor:pointer;vertical-align:1px;user-select:none}
  .note.nx .q:hover{background:var(--accent);color:#0b0e13}
  #startDlg{position:fixed;inset:0;z-index:76;background:rgba(6,8,12,.72);
           display:none;align-items:flex-start;justify-content:center;
           overflow:auto;padding:24px}
  #startDlg.on{display:flex}
  #startBox{background:var(--panel);border:1px solid var(--line);border-radius:8px;
            padding:16px;max-width:620px;width:100%;
            box-shadow:0 18px 60px rgba(0,0,0,.6)}
  #startBox h2{margin:0 0 10px;text-transform:none;letter-spacing:0;
               font-size:13px;color:var(--fg)}
  .startpath{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin:12px 0;
             padding:8px;background:var(--panel2);border:1px solid var(--line);
             border-radius:6px;min-height:20px}
  .startpath b{background:#0f1319;border:1px solid var(--line);border-radius:4px;
               padding:2px 7px;font-weight:600;cursor:pointer}
  .startpath b:hover{border-color:var(--accent)}
  .startpath span{color:var(--dim)}
  .startacts{display:flex;flex-wrap:wrap;gap:8px}
  .startacts button{min-width:96px}
  #noteDlg{position:fixed;inset:0;z-index:75;background:rgba(6,8,12,.72);
           display:none;align-items:flex-start;justify-content:center;
           overflow:auto;padding:24px}
  #noteDlg.on{display:flex}
  #noteBox{background:var(--panel);border:1px solid var(--line);border-radius:8px;
           padding:16px;max-width:560px;width:100%;
           box-shadow:0 18px 60px rgba(0,0,0,.6)}
  #noteBox h2{margin:0 0 10px;text-transform:none;letter-spacing:0;
              font-size:13px;color:var(--fg)}
  #noteBody{font-size:13px;line-height:1.6;color:var(--fg)}
  #scrDlg{position:fixed;inset:0;z-index:72;background:rgba(6,8,12,.72);
          display:none;align-items:flex-start;justify-content:center;
          overflow:auto;padding:20px}
  #scrDlg.on{display:flex}
  #scrBox{background:var(--panel);border:1px solid var(--line);border-radius:8px;
          padding:16px;max-width:860px;width:100%;
          box-shadow:0 18px 60px rgba(0,0,0,.6)}
  #scrBox h2{margin:0 0 10px}
  #scrBox textarea{width:100%;background:var(--panel2);color:var(--fg);
                   border:1px solid var(--line);border-radius:5px;padding:6px 8px;
                   font:12px/1.45 "Segoe UI",system-ui,sans-serif;resize:vertical}
  /* a small four-colour card face for table cells */
  #hoverCard{position:fixed;z-index:60;display:none;pointer-events:none;
    background:var(--panel);border:1px solid var(--line);border-radius:6px;
    padding:8px 10px;box-shadow:0 8px 28px rgba(0,0,0,.55);font-size:11px;
    min-width:250px;max-width:480px}
  #hoverCard h4{margin:0 0 6px;font-size:12px;font-weight:600}
  #hoverCard .hgrid{display:grid;gap:4px}
  #hoverCard .hc{border-radius:3px;padding:4px 5px;min-width:86px;
    color:#12161d;font-size:11px;line-height:1.3;background:var(--line)}
  #hoverCard .hc.out{background:var(--panel2);color:var(--dim);opacity:.55}
  #hoverCard .hc .hr{display:flex;justify-content:space-between;gap:8px;
    font-variant-numeric:tabular-nums}
  #hoverCard .hc .hr b{font-weight:600}
  #hoverCard .hc>span{font-size:13px;font-weight:700;letter-spacing:.3px}
  #hoverCard .lkd{box-shadow:0 0 0 2px #ffd479 inset}
  #hoverCard .hc .s{color:#1b1f27} #hoverCard .hc .h{color:#d92b2b}
  #hoverCard .hc .d{color:#2f6fe0} #hoverCard .hc .c{color:#1a9257}
  #hoverCard .hc.out .s,#hoverCard .hc.out .h,
  #hoverCard .hc.out .d,#hoverCard .hc.out .c{color:var(--dim)}
  .rc{display:inline-block;min-width:26px;padding:1px 4px;border-radius:3px;
      background:#f5f7fb;border:1px solid #c3cad6;cursor:pointer;text-align:center;
      font:700 12px/1.3 "Segoe UI",system-ui,sans-serif}
  .rc:hover{border-color:var(--accent)}
  .rc.s{color:#1b1f27} .rc.h{color:#d92b2b}
  .rc.d{color:#2f6fe0} .rc.c{color:#1a9257}
  tfoot td{border-top:2px solid var(--line);border-bottom:none}
  .err{color:#ff8b8b}
  /* Los botones de accion, de ancho FIJO.
     La barra de proporciones de abajo si crece con la ventana, porque lo suyo
     es ser proporcional; estos no. Una caja que se estira hasta el borde de la
     pantalla porque solo hay dos acciones queda mal y ademas enganna: parece
     que el tamano dice algo. */
  .acts{display:flex;gap:8px;flex-wrap:wrap;margin:8px 0}
  .act-btn{width:184px;padding:8px 6px;border-radius:4px;cursor:pointer;
    text-align:center;border:2px solid transparent;color:#12161d;
    font-size:11px;line-height:1.35;user-select:none}
  .act-btn b{display:block;font-size:13px;letter-spacing:.02em}
  .act-btn .cmb{display:block;opacity:.78}
  /* `abf` y no `pc`: `.pc` es la carta de la baraja, con su aspect-ratio y su
     fondo claro, y un span con ese nombre dentro del boton salia como un
     cuadro blanco del tamano de una carta. Acotar la regla propia no arregla
     nada cuando la que sobra es la global. */
  .act-btn .abf{display:block;font-weight:600}
  .act-btn:hover{border-color:var(--fg)}
  .act-btn.on{border-color:var(--fg);box-shadow:0 0 0 2px var(--bg) inset}
  .act-btn.off{opacity:.38}
  .bar{display:flex;height:20px;border-radius:3px;overflow:hidden;margin:6px 0}
  /* Negro y no blanco: los colores de accion son claros desde que la rejilla
     se pinta al estilo de la referencia, y el blanco encima no se leia. */
  .bar div{display:flex;align-items:center;justify-content:center;font-size:10px;
           color:#12161d;font-weight:600;min-width:0;overflow:hidden;white-space:nowrap}
  .kv{display:flex;gap:16px;flex-wrap:wrap;font-size:12px;color:var(--dim);margin-bottom:8px}
  .kv b{color:var(--fg);font-weight:600}
  .street{border:1px solid var(--line);border-radius:5px;padding:8px;min-width:0}
  /* Flop, turn y river en fila. En vertical la lista era tan alta que para ver
     los tamanos de OOP habia que perder de vista los de IP, que es justo la
     comparacion que uno quiere hacer. */
  .streetrow{display:grid;grid-auto-flow:column;grid-auto-columns:1fr;gap:6px;
             margin-bottom:6px}
  .side{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:var(--accent);
        margin:10px 0 5px;font-weight:600}
  .street h3{margin:0 0 4px;font-size:11px;text-transform:uppercase;letter-spacing:.06em;
             color:var(--accent);font-weight:600}
  .street.off{opacity:.4}
  .pbar{height:9px;background:#0e1117;border:1px solid var(--line);border-radius:5px;
        overflow:hidden;margin-top:9px}
  .pbar div{height:100%;width:0%;border-radius:5px;
            background:linear-gradient(90deg,#4c8bf5,#7ee2a8);transition:width .18s}
  .hud{position:sticky;top:0;z-index:20;background:var(--panel);
       border:1px solid var(--line);border-radius:6px;padding:10px 12px;margin-bottom:12px;
       box-shadow:0 6px 18px rgba(0,0,0,.35)}
  .hudrow{display:flex;gap:10px;align-items:stretch}
  .hudside{flex:1;border-radius:5px;padding:7px 10px;min-width:0}
  .hudside.oop{background:linear-gradient(180deg,#1d3350,#182a41)}
  .hudside.ip{background:linear-gradient(180deg,#153528,#122b21)}
  .hudside .who{font-size:10px;letter-spacing:.09em;text-transform:uppercase;color:var(--dim)}
  .hudside .ev{font:700 22px/1.15 "Segoe UI",system-ui,sans-serif;
               font-variant-numeric:tabular-nums}
  .hudside .sub{font-size:11px;color:var(--dim);font-variant-numeric:tabular-nums}
  .hudside.act{outline:2px solid var(--accent);outline-offset:-2px}
  .split{display:flex;height:7px;border-radius:4px;overflow:hidden;margin:8px 0 6px}
  .split .a{background:#4c8bf5} .split .b{background:#2f9e63}
  .hudmeta{display:flex;gap:14px;flex-wrap:wrap;font-size:11px;color:var(--dim);
           font-variant-numeric:tabular-nums}
  .hudmeta b{color:var(--fg)}
  .up{color:#7ee2a8} .dn{color:#ff8b8b}
  button.stop{background:#7a2f2f;border-color:#a04545;color:#ffe0e0;font-weight:600}
  button.stop:hover{background:#8e3838;border-color:#c05555}

  /* ------------------------------------------------------------------ jugar
     La mesa.

     Aqui no se estudia: se juega. Asi que fuera todo lo que no sea la mano que
     tienes delante -- ni rejillas, ni arbol, ni paneles -- y las cartas
     GRANDES. Una carta de 30 pixeles se lee cuando la buscas; en una decision
     de tres segundos tiene que entrar por los ojos sin buscarla.

     El fieltro es verde oscuro y no verde de tapete: la pantalla es oscura y un
     verde de casino al lado de un panel #1d212c parece un error de CSS. Lo
     justo para que se lea "esto es una mesa" y no canse a las dos horas. */
  .trainer{display:none}
  body.playing .trainer{display:block}
  body.playing main{display:none}
  body.playing header .pill,
  body.playing header .board,
  body.playing #setupBtn,
  body.playing #goTop,
  body.playing #stopTop{display:none}

  .tbar{display:flex;align-items:center;gap:10px;flex-wrap:wrap;
        padding:8px 14px;background:var(--panel);border-bottom:1px solid var(--line)}
  .tbar .sp{margin-left:auto}
  .tbar b{font-size:14px}

  .tgrid{display:grid;grid-template-columns:minmax(0,1fr) 320px;gap:14px;padding:14px}
  @media (max-width:980px){ .tgrid{grid-template-columns:minmax(0,1fr)} }

  /* La mesa.

     Un ovalo con rail, y los dos asientos enfrentados. No es decoracion: en una
     mesa se sabe de quien es cada cosa por DONDE esta -- tus cartas abajo, las
     suyas arriba, lo apostado entre cada uno y el centro -- y eso se lee sin
     pensar. Una lista de numeros ordenados no se lee igual aunque diga lo mismo.

     El rail es oscuro y el pano verde apagado: la pantalla es oscura y un verde
     de casino al lado de un panel gris canta. Lo justo para que se entienda. */
  .felt{position:relative;padding:20px 10px 16px;
        display:flex;flex-direction:column;align-items:center;gap:10px}
  .mesa{position:relative;width:100%;max-width:820px;aspect-ratio:1.32;
        border-radius:50%/40%;
        background:radial-gradient(ellipse at 50% 42%,#20463c 0%,#17322c 55%,#122622 100%);
        border:13px solid #1b1410;
        box-shadow:0 0 0 2px #2c211a, 0 18px 50px #0009, inset 0 0 60px #0006;
        display:flex;flex-direction:column;align-items:center;
        justify-content:space-between;padding:16px 18px;gap:2px}
  /* Una linea de luz por dentro del rail, que es lo que da el relieve. */
  .mesa::before{content:"";position:absolute;inset:0;border-radius:inherit;
                box-shadow:inset 0 0 0 2px #ffffff0d;pointer-events:none}

  /* El centro: bote y cartas comunes. */
  .centro{display:flex;flex-direction:column;align-items:center;gap:8px}

  /* Los asientos, pegados al rail por arriba y por abajo. */
  .silla{display:flex;flex-direction:column;align-items:center;gap:6px;
         width:max-content;max-width:100%}
  .chapa{display:flex;align-items:center;gap:9px;padding:5px 12px 5px 5px;
         border-radius:26px;background:#141a22;border:1px solid var(--line);
         box-shadow:0 4px 14px #0007}
  .chapa .av{width:34px;height:34px;border-radius:50%;flex:none;
             display:flex;align-items:center;justify-content:center;
             font-weight:700;font-size:13px;color:#0b0e13;
             background:linear-gradient(160deg,#9fb6d8,#6d86ab)}
  .silla.abajo .chapa .av{background:linear-gradient(160deg,#9ce8bd,#4fae7d)}
  .chapa .quien{display:flex;flex-direction:column;line-height:1.15}
  .chapa .nick{font-weight:700;font-size:13px}
  .chapa .stk{color:var(--dim);font-size:12px;font-variant-numeric:tabular-nums}
  .chapa .stk b{color:#e8d48a}
  .chapa.act{border-color:var(--accent);box-shadow:0 0 0 1px var(--accent),0 4px 14px #0007}
  .chapa .pos{font-size:10px;letter-spacing:.06em;color:var(--dim);
              border:1px solid var(--line);border-radius:4px;padding:1px 4px}
  /* El boton del dealer, que en heads-up postflop es siempre IP. */
  .dealer{width:20px;height:20px;border-radius:50%;background:#f2f4f8;color:#111;
          font-size:11px;font-weight:700;display:flex;align-items:center;
          justify-content:center;box-shadow:0 2px 6px #0008;flex:none}

  /* Las fichas apostadas, entre el jugador y el centro, que es donde estan en
     una mesa: sin eso no se ve quien ha metido que sin leer el historial. */
  .apuesta{display:flex;align-items:center;gap:7px;min-height:22px}
  .apuesta .pila{display:flex}
  .apuesta .pila i{width:15px;height:15px;border-radius:50%;margin-left:-6px;
                   background:radial-gradient(circle at 35% 30%,#ffe9a8,#e0b33a 60%,#a6801f);
                   border:1px solid #6b5411;box-shadow:0 1px 3px #0009}
  .apuesta .pila i:first-child{margin-left:0}
  .apuesta .amt{font-weight:700;font-variant-numeric:tabular-nums;
                background:#0c1211cc;border-radius:10px;padding:1px 7px}
  .hand{display:flex;gap:6px;min-height:0;align-items:center}

  /* La carta grande. Mismo cuatro colores que el resto del programa: un palo
     no puede ser rojo en una pantalla y azul en otra. */
  .tcard{width:56px;height:80px;border-radius:8px;background:#f7f9fc;
         border:1px solid #c9d2e0;box-shadow:0 3px 10px #0007;
         display:flex;flex-direction:column;align-items:center;justify-content:center;
         font-weight:700;font-size:26px;line-height:1;user-select:none}
  .tcard .su{font-size:21px;margin-top:2px}
  .tcard.s{color:#1b1f27} .tcard.h{color:#d92b2b}
  .tcard.d{color:#2f6fe0} .tcard.c{color:#1a9257}
  /* Boca abajo: un lomo liso, sin dibujos que distraigan. */
  .tcard.back{background:repeating-linear-gradient(45deg,#2a3550,#2a3550 6px,#323e5c 6px,#323e5c 12px);
              border-color:#3d4a6b;color:transparent}
  .tcard.sm{width:44px;height:62px;font-size:20px}
  .tcard.sm .su{font-size:16px}
  /* Las tuyas, mas grandes: son las que miras. Las suyas estan tapadas y no
     hay nada que leer en ellas hasta el showdown. */
  .silla.abajo .tcard{width:62px;height:88px;font-size:29px}
  .silla.abajo .tcard .su{font-size:23px}
  .silla.arriba .tcard{width:46px;height:65px;font-size:21px}
  .silla.arriba .tcard .su{font-size:17px}

  .boardrow{display:flex;gap:7px;align-items:center;justify-content:center;
            padding:8px 10px;border-radius:12px;background:#0e1a1799;
            box-shadow:inset 0 0 0 1px #ffffff0a}
  .boardrow .gap{width:56px;height:80px;border-radius:8px;
                 border:2px dashed #ffffff26;background:#ffffff08}

  .potline{font-size:14px;color:var(--dim);display:flex;align-items:center;gap:8px;
           background:#0c1211aa;border-radius:14px;padding:3px 12px}
  .potline b{font-size:22px;color:var(--fg);font-variant-numeric:tabular-nums}
  .potline .chip{width:14px;height:14px;border-radius:50%;
                 background:radial-gradient(circle at 35% 30%,#ffe9a8,#e0b33a 60%,#a6801f);
                 border:1px solid #6b5411}

  /* El turn y el river caen girando. No es adorno: sin el giro, una carta
     nueva en una fila de cartas no se ve, y lo que acaba de cambiar es
     justo lo unico que hay que mirar. */
  @keyframes cae{
    0%  {transform:rotateY(90deg) translateY(-14px);opacity:0}
    60% {transform:rotateY(-12deg) translateY(0);opacity:1}
    100%{transform:rotateY(0) translateY(0);opacity:1}
  }
  .tcard.nueva{animation:cae .42s ease-out both}
  @media (prefers-reduced-motion:reduce){ .tcard.nueva{animation:none} }

  .tactions{display:flex;gap:10px;flex-wrap:wrap;justify-content:center;
            margin-top:12px;min-height:44px}
  .tactions button{padding:13px 24px;font-size:15px;font-weight:600;border-radius:10px;
                   min-width:124px;background:var(--panel2);border:1px solid var(--line);
                   color:var(--fg);cursor:pointer;transition:transform .08s}
  .tactions button:active{transform:translateY(1px)}
  .tactions button:hover{border-color:var(--accent)}
  .tactions button .amt{display:block;font-weight:400;color:var(--dim);font-size:12px}
  /* La tecla, en la esquina del boton. Asi se aprende sola: no hace falta
     buscarla en ningun sitio, esta donde se mira. */
  .tactions button{position:relative}
  .tactions button .key{position:absolute;top:4px;right:6px;font-size:10px;
                        font-weight:700;opacity:.5;letter-spacing:.04em}
  /* El color dice que es cada accion, el mismo de la rejilla de estrategia. */
  .tactions button.f{background:#2b3c52;border-color:#3f5a78}
  .tactions button.x{background:#25402f;border-color:#3a6a4b}
  .tactions button.c{background:#25402f;border-color:#3a6a4b}
  .tactions button.b,.tactions button.r{background:#4a2b26;border-color:#7a453b}
  /* Con el consejo encendido, cada boton lleva su frecuencia debajo. */
  .tactions .adv{display:block;font-size:11px;color:#cfd6e4;opacity:.85;font-weight:400}

  /* La nota de la mano anterior, que se queda puesta mientras juegas la
     siguiente. Con "seguir solo" la mano se va en dos segundos y el veredicto
     se iba con ella: lo que te dice si vas bien no puede durar menos que el
     tiempo que tardas en mirarlo. */
  .tlast{display:flex;align-items:center;gap:10px;font-size:12px;color:var(--dim);
         background:var(--panel);border:1px solid var(--line);border-radius:20px;
         padding:4px 12px}
  .tlast b{color:var(--fg)}
  .tmsg{text-align:center;padding:10px 14px;border-radius:10px;max-width:70%;
        background:#3a2326;border:1px solid #7a3b42;color:#ffd9dd}
  .tend{text-align:center;padding:10px 14px;border-radius:10px;min-width:60%;
        background:#0f1a18cc;border:1px solid #24413a}
  .tend .big{font-size:24px;font-weight:700}
  /* El veredicto de la mano. Lo grande no es lo que ganaste -- eso depende de
     como caigan las cartas -- sino lo que te costo jugarla, que es lo unico
     que depende de ti. */
  .vered{display:inline-flex;align-items:center;gap:10px;margin:8px 0 2px;
         padding:7px 14px;border-radius:24px;font-weight:700;font-size:15px;
         letter-spacing:.02em}
  .vered small{font-weight:400;font-size:12px;opacity:.85;letter-spacing:0}
  .v0{background:#1d4a33;border:1px solid #3f8f62;color:#b6f5d2}
  .v1{background:#244436;border:1px solid #3f7a5c;color:#a9e6c4}
  .v2{background:#3a3a24;border:1px solid #7a7340;color:#ecdf9f}
  .v3{background:#43301f;border:1px solid #8a6134;color:#f3cf9c}
  .v4{background:#452225;border:1px solid #8c4147;color:#ffc4c9}
  .tend .win{color:#7ee2a8} .tend .lose{color:#ff8b8b}

  .tside{display:flex;flex-direction:column;gap:12px}
  .tside .box{background:var(--panel);border:1px solid var(--line);
              border-radius:10px;padding:10px 12px}
  .tside h3{margin:0 0 8px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;
            color:var(--dim);font-weight:600}
  .tside table{width:100%;border-collapse:collapse;font-size:13px}
  .tside td{padding:2px 0}
  .tside td.k{color:var(--dim)}
  .tside td.v{text-align:right;font-variant-numeric:tabular-nums}
  .tlog{margin:0;padding-left:16px;font-size:12px;color:var(--dim);max-height:150px;
        overflow:auto}
  .tlog li{margin:1px 0}
  .tlog li.vacio{list-style:none;margin-left:-16px;opacity:.7;font-style:italic}
  .tlog li.you{color:var(--fg)}

  /* El consejo. Una fila por accion: frecuencia en barra y EV en numero. */
  .advrow{display:grid;grid-template-columns:1fr 54px 62px;gap:6px;align-items:center;
          font-size:12px;padding:2px 0}
  .advrow .bar{position:relative;height:16px;border-radius:4px;background:#0f1319;
               border:1px solid var(--line);overflow:hidden}
  .advrow .bar i{position:absolute;left:0;top:0;bottom:0;background:var(--accent);
                 opacity:.55}
  .advrow .bar span{position:relative;padding-left:6px;line-height:16px}
  .advrow .fq,.advrow .ev{text-align:right;font-variant-numeric:tabular-nums}
  .advrow.best .ev{color:#7ee2a8;font-weight:700}
  .advoff{color:var(--dim);font-size:12px}

  /* El historial. Una fila por mano, y se puede volver a jugar cualquiera:
     la semilla la reparte entera otra vez. */
  .hist{max-height:190px;overflow:auto;display:flex;flex-direction:column;gap:3px}
  .hrow{display:grid;grid-template-columns:22px 1fr auto auto;gap:8px;align-items:center;
        padding:3px 6px;border-radius:5px;background:#0f1319;border:1px solid var(--line);
        cursor:pointer;font-size:12px}
  .hrow:hover{border-color:var(--accent)}
  .hrow .n{color:var(--dim);text-align:right}
  .hrow .cc{font-weight:600;letter-spacing:.02em}
  .hrow .res{font-variant-numeric:tabular-nums}
  .hrow .res.up{color:#7ee2a8} .hrow .res.dn{color:#ff8b8b}
  .hrow .mk{width:9px;height:9px;border-radius:50%}
  .mk.v0{background:#3f8f62} .mk.v1{background:#3f7a5c} .mk.v2{background:#7a7340}
  .mk.v3{background:#8a6134} .mk.v4{background:#8c4147}

  /* El repaso: lo que costo cada decision. */
  .rev{width:100%;border-collapse:collapse;font-size:12px}
  .rev th{color:var(--dim);font-weight:600;text-align:left;padding:2px 4px}
  .rev td{padding:2px 4px;border-top:1px solid var(--line)}
  .rev td.n{text-align:right;font-variant-numeric:tabular-nums}
  .rev tr.bad td.n{color:#ff8b8b}
  .rev tr.good td.n{color:#7ee2a8}
</style>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Crect width='64' height='64' rx='14' fill='%234c8bf5'/%3E%3Ctext x='32' y='49' font-size='46' text-anchor='middle' fill='%23ffffff' font-family='Segoe UI Symbol, DejaVu Sans, serif'%3E%26%239824;%3C/text%3E%3C/svg%3E">
</head>
<body>

<header>
  <button id="setupBtn" class="sm" onclick="toggleSetup()"
          title="esconder el montaje y dejarle el ancho a la solución"></button>
  <span class="title">DCFR Solver</span><span id="ver" class="ver"></span>
  <button id="goTop" class="sm primary" onclick="solve()">Resolver</button>
  <button id="stopTop" class="sm stop" onclick="stopSolve()"
          style="display:none">Stop</button>
  <button id="playBtn" class="sm" onclick="trainOpen()"
          title="jugar el árbol resuelto contra la solución, mano a mano">Jugar</button>
  <span class="board" id="boardView"></span>
  <span class="pill" id="streetPill"></span>
  <span class="pill" id="statusPill">parado</span>
  <span class="pill" id="valuePill" style="display:none"></span>
  <span class="pill" id="sizePill"></span>
  <span class="pill" id="convPill"></span>
  <span class="pill" id="progPill" style="display:none"></span>
  <span class="gearwrap">
    <button id="gearBtn" class="gear" onclick="toggleOpts()"
            title="opciones">&#9881;</button>
    <div class="opts" id="optsPanel" style="display:none">
      <h3>Idioma / Language</h3>
      <div class="lang">
        <button class="sm" id="langEs" onclick="ponIdioma(&quot;es&quot;)">Español</button>
        <button class="sm" id="langEn" onclick="ponIdioma(&quot;en&quot;)">English</button>
      </div>
      <div class="sep"></div>
      <h3>Avanzado</h3>
      <div class="row">
        <div><label>Limite de memoria (GB)</label>
          <input type="number" id="maxMem" class="needsIdle" min="1" step="1"
            title="El árbol se rechaza por encima de esto, antes de reservar nada"></div>
        <div><label>Hilos <span id="thNote" style="color:var(--dim)"></span></label>
          <input type="number" id="threads" min="0" step="1"></div>
      </div>
      <div class="note nx">Sale del <b>75% de la RAM</b> de esta máquina.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">El limite de memoria sale del <b>75% de la RAM de esta
        máquina</b>, así que normalmente no hay que tocarlo. El árbol se rechaza
        <b>antes</b> de reservar nada: subirlo no rompe nada, como mucho el
        sistema empieza a tirar de disco y todo va despacio.</span></div>
      <div class="row">
        <div><label>Tope de iteraciones</label>
          <input type="number" id="iters" min="50" step="50"
            title="Red de seguridad: corta el cálculo aunque no se alcance la precisión"></div>
        <div><label>Tope de tiempo (s)</label>
          <input type="number" id="timeout" min="0" step="30"
            title="Para el cálculo al pasar ese tiempo. 0 = sin tope"></div>
        <div><label>All-in si compromete más del %</label><input type="number" id="allinPct"
          class="needsIdle" min="0" max="100" step="1"
          title="Del stack efectivo INICIAL. Una apuesta que comprometa más de esto se convierte en all-in limpio. 67 es el valor habitual; 100 lo desactiva"></div>
      </div>
      <div class="note nx">El tope de iteraciones es una <b>red de seguridad</b>, no el criterio.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">El tope de iteraciones es una red de seguridad, no el criterio:
        corta el cálculo si la precisión no se alcanza nunca. En condiciones
        normales no llega a actuar.
        <br><br>El all-in es del stack efectivo <b>inicial</b>, no del bote. Con dos
        tercios dentro ya estas comprometido: el tercio de detras no compra ninguna
        decision, solo ramas. 100 lo desactiva.</span></div>
      <div class="row">
        <div><label>Rake %</label><input type="number" id="rakePct" class="needsIdle" min="0" max="20" step="0.5"
          title="Porcentaje del bote igualado. 0 = sin rake, que es lo correcto para torneo y para teoria"></div>
        <div><label>Tope de rake</label><input type="number" id="rakeCap" class="needsIdle" min="0" step="0.5"
          title="En fichas. 0 = sin tope"></div>
      </div>
      <div class="note nx">Sin rake por defecto.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Sin rake por defecto. Con rake el juego deja de ser suma cero:
        OOP + IP ya no suman el bote, y la diferencia es lo que se lleva la casa.</span></div>
      <div class="row" style="margin-top:8px">
        <button class="sm" id="optApply" onclick="aplicaOpciones()">Aplicar</button>
      </div>
      <div class="note" id="optNote"></div>
      <div class="sep"></div>
      <div class="kv"><span>DCFR Solver</span><b id="optVer">-</b></div>
      <div class="note">GPL v3, sin garantía. Se reparte tal cual.</div>
    </div>
  </span>
</header>

<main>
<div class="col-l">

  <!-- Los locks, en una tira.
       Ocupaba un panel entero arriba de la columna derecha, con cuatro lineas
       de explicacion, y empujaba hacia abajo lo primero que uno mira: el
       reparto OOP/IP. Ademas casi siempre no hay ninguno, asi que era un panel
       para decir "no locks". Ahora, sin locks, es una linea; con ellos, se abre
       y saca los botones. -->
  <div id="fambadge"></div>
  <div id="lockStrip" class="lockstrip"></div>

  <div class="panel">
    <h2>Board</h2>
    <div class="bslots" id="bslots"></div>
    <div class="row" style="margin-top:8px">
      <button class="sm" style="flex:0 0 auto" id="deckToggle"
              onclick="toggleDeck()">Elegir flop</button>
      <input type="text" id="board" class="needsIdle" placeholder="Ah9h4h">
      <button class="sm needsIdle" style="flex:0 0 auto" onclick="applyBoard()">Poner</button>
    </div>
    <div class="deck" id="deck" style="display:none"></div>
    <div class="note" id="comboNote"></div>
  </div>

  <div class="panel">
    <h2>Rangos</h2>
    <div class="tabs">
      <button id="tabOOP" class="act" onclick="setTab(0)">OOP</button>
      <button id="tabIP" onclick="setTab(1)">IP</button>
    </div>
    <div class="grid" id="rangeGrid"></div>
    <label>Peso de lo que pintes: <span id="wLabel">100%</span></label>
    <input type="range" id="weight" class="needsIdle" min="0" max="100" value="100" step="5"
           oninput="document.getElementById('wLabel').textContent=this.value+'%'">
    <label>Rango en texto</label>
    <input type="text" id="rangeSpec" class="needsIdle">
    <div class="note nx">Formato de texto estándar, el de cualquier librería de rangos.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Formato de texto: <code>AA,KK,AKs,A8o:0.5</code>. Las manos
      puras van sin peso y las parciales con <code>:0.5</code>. Se pega tal cual desde
      cualquier librería de rangos o desde otro solver, y lo que sale de la
      cuadrícula se pega en ellos igual.</span></div>
    <div class="row" style="margin-top:8px">
      <button class="sm needsIdle" onclick="applyRangeText()">Aplicar texto</button>
      <button class="sm" onclick="clearRange()">Limpiar</button>
    </div>
    <div class="row" style="margin-top:8px">
      <input type="text" id="rangeName" placeholder="nombre del rango">
      <button class="sm needsIdle" style="flex:0 0 auto" onclick="storeRange('save')">Guardar</button>
    </div>
    <div class="row" style="margin-top:6px">
      <select id="rangePick"></select>
      <button class="sm needsIdle" style="flex:0 0 auto" onclick="storeRange('load')">Cargar</button>
      <button class="sm needsIdle" style="flex:0 0 auto" onclick="storeRange('delete')">Borrar</button>
    </div>
    <div class="note nx">Se guardan <b>los dos</b>, OOP e IP, y se cargan los dos.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Se guardan <b>los dos rangos juntos</b>, OOP e IP: un rango
      es de un spot concreto y solo dice algo con el del otro lado al lado.
      Cargarlo pone los dos.</span></div>
    <div class="note" id="rangeNote"></div>
  </div>

  <div class="panel">
    <h2>Montaje del árbol</h2>
    <div class="row">
      <div><label>Bote inicial</label><input type="number" id="pot" class="needsIdle" step="0.5"></div>
      <div><label>Stack efectivo</label><input type="number" id="stack" class="needsIdle" step="1"></div>
    </div>
    <div id="streets"></div>
    <div class="row">
      <div><label>Precisión deseada (% del bote)</label>
           <input type="number" id="accPct" min="0" step="1"></div>
      <div class="chk"><input type="checkbox" id="accStop">
           <label for="accStop">parar el cálculo al alcanzar la precisión deseada</label></div>
    </div>
    <div class="note nx">Más bajo, más fina la solución y más tarda.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Cuanto más bajo el número, más fina la solución y más
      tarda. El solver para solo cuando la alcanza y te dice que la alcanzó.
      Nunca te dará peor de lo que pides.</span></div>
    <div class="row" style="margin-top:10px">
      <button id="buildBtn" onclick="applyTree()">Montar árbol</button>
      <button class="primary" id="goBtn" onclick="solve()">Resolver</button>
      <button id="moreBtn" onclick="solveMore()">+ iteraciones</button>
      <button class="stop" id="stopBtn" onclick="stopSolve()" style="display:none">Stop</button>
      <button class="sm" style="flex:0 0 auto" onclick="abreScript()"
              title="resolver una lista de boards de una tirada, sin estar delante">Varios boards…</button>
    </div>
    <div id="progWrap" style="display:none">
      <div class="pbar"><div id="pbarFill"></div></div>
      <div class="note" id="progNote"></div>
    </div>
    <div class="note" id="treeNote"></div>
  </div>

  <div class="panel">
    <h2>Guardar</h2>
    <div class="note nx" style="margin:0 0 8px"><b>Config</b>: la receta. <b>Árbol</b>: la receta y la solución hecha.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Una <b>config</b> es la receta: board,
      rangos y tamaños. Unos pocos KB. Un <b>árbol</b> es la receta más la solución
      ya calculada: pesa lo que pese, pero vuelve al spot sin recalcular y puedes
      seguir iterando donde lo dejaste.</span></div>
    <div class="row">
      <div><label>Nombre</label><input type="text" id="saveName" class="needsIdle" placeholder="btn-vs-bb-monotono"></div>
      <div><label>&nbsp;</label>
        <div class="row" style="gap:6px;margin:0">
          <button class="needsIdle" onclick="store('save','config')">Guardar config</button>
          <button class="needsIdle" onclick="store('save','tree')" id="saveTreeBtn">Guardar árbol</button>
        </div>
      </div>
    </div>
    <div class="row" style="margin-top:8px">
      <div><label>Configs guardados</label>
        <select id="cfgList" class="needsIdle" onchange="pickSave(0)"></select></div>
      <div><label>&nbsp;</label>
        <div class="row" style="gap:6px;margin:0">
          <button class="needsIdle" onclick="store('load','config')">Cargar</button>
          <button class="stop needsIdle" onclick="store('delete','config')">Borrar</button>
        </div>
      </div>
    </div>
    <div class="row" style="margin-top:8px">
      <div><label>Árboles guardados</label>
        <select id="treeList" class="needsIdle" onchange="pickSave(1)"></select></div>
      <div><label>&nbsp;</label>
        <div class="row" style="gap:6px;margin:0">
          <button class="needsIdle" onclick="store('load','tree')">Cargar</button>
          <button class="stop needsIdle" onclick="store('delete','tree')">Borrar</button>
        </div>
      </div>
    </div>
    <div class="note" id="diskNote"></div>
    <div class="note" id="saveNote"></div>
  </div>


</div>

<div class="col-r">

  <div id="hoverCard"></div>

  <!-- The nodelock dialog. Everything in it is the grid and two controls:
       pick an action, paint the hands, move the weight. No typing. -->
  <!-- El detalle de una nota. Una sola caja para todas: lo que se ensena lo
       pone el enlace que la abre. -->
  <div id="startDlg" onclick="if(event.target===this)startClose()">
   <div id="startBox">
    <h2>¿Dónde empieza la mano?</h2>
    <div class="note nx">Baja por el árbol hasta el punto que quieras entrenar y dale a
      <b>Empezar aquí</b>. Las cartas que falten se reparten al azar en cada mano, así que
      entrenas el nodo y no una carta concreta.<span class="q"
      onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Empezar
      en un nodo de dentro no es lo mismo que empezar por el principio y llegar hasta él:
      la mano se reparte con el rango que <b>llega ahí</b>, que es más estrecho. Si eliges
      el nodo de después de pagar una apuesta, te tocarán manos que pagan, no manos
      cualesquiera.</span></div>
    <div id="startPath" class="startpath"></div>
    <div id="startActs" class="startacts"></div>
    <div class="row" style="margin-top:12px">
      <button class="primary" id="startUse" onclick="startUse()">Empezar aquí</button>
      <button class="sm" onclick="startRoot()">Volver al principio</button>
      <button class="sm" onclick="startClose()">Cerrar</button>
    </div>
   </div>
  </div>
  <div id="noteDlg" onclick="if(event.target===this)cierraNota()">
   <div id="noteBox">
    <h2 id="noteTitle"></h2>
    <div id="noteBody"></div>
    <div class="row" style="margin-top:12px">
      <button class="primary" onclick="cierraNota()">Entendido</button>
    </div>
   </div>
  </div>

  <!-- Resolver muchos boards de una tirada. La explicacion va DENTRO de la
       ventana: esto se usa una vez cada mucho, y nadie se acuerda de como iba. -->
  <div id="scrDlg" onclick="if(event.target===this)cierraScript()">
   <div id="scrBox">
    <h2>Resolver muchos boards seguidos</h2>
    <div class="note manual" style="font-size:12px;line-height:1.6">
      Esto <b>no resuelve aquí</b>: escribe un <b>script</b>, un fichero de texto con
      la lista de órdenes, y ese fichero lo lanzas desde una terminal y lo dejas
      corriendo el tiempo que haga falta —toda la noche, si quieres—. El navegador
      puedes cerrarlo.
      <br><br>
      <b>Cómo va, de principio a fin:</b>
      <br>1. Monta aquí el spot que quieres: board de ejemplo, rangos, tamaños, bote
      y stack. Lo que haya en la pantalla es lo que va a usar <i>cada</i> board.
      <br>2. Pon abajo la lista de boards, uno por línea. El botón
      <b>Flops al azar</b> te la llena.
      <br>3. Dile cuándo parar en cada board: por <b>precisión</b>, por <b>tiempo</b>,
      o por los dos —para en el primero que llegue—.
      <br>4. <b>Generar</b>. Se guarda tu spot con el nombre que pongas y sale el
      texto del script.
      <br>5. <b>Descargar</b>, y en una terminal, en la carpeta del solver:
      <code>solver --script nombre.txt</code>
      <br><br>
      Cada board resuelto se guarda como un árbol en <code>saves/trees</code>, con el
      nombre que diga el patrón. Después los abres uno a uno desde
      <b>Guardar → Árboles guardados</b> y los miras como cualquier otro.
      <br><br>
      <b>Cuidado con el sitio en el disco</b>: un árbol de flop ocupa lo que ocupa —
      lo dice el panel de guardar — y veinte boards son veinte veces eso.
    </div>

    <div class="row" style="margin-top:12px">
      <div style="flex:2">
        <label>Boards, uno por línea</label>
        <textarea id="scrBoards" rows="8" placeholder="Ah9h4h&#10;Kd7c2s&#10;Ts9s8d"></textarea>
      </div>
      <div style="flex:1">
        <label>Flops al azar</label>
        <div class="row">
          <input type="number" id="scrN" min="1" max="500" step="1" value="20">
          <button class="sm" style="flex:0 0 auto" onclick="scriptFlops()">Poner</button>
        </div>
        <div class="note">Ninguno repetido, y ninguno que sea otro con los palos
          cambiados de nombre: esos dan la misma solución.</div>
      </div>
    </div>

    <div class="row" style="margin-top:8px">
      <div><label>Precisión deseada (% del bote)</label>
        <input type="number" id="scrAcc" min="0" step="0.1"></div>
      <div><label>Tope de tiempo por board (s)</label>
        <input type="number" id="scrTmo" min="0" step="30" value="600"></div>
      <div><label>Tope de iteraciones</label>
        <input type="number" id="scrIters" min="50" step="50"></div>
    </div>
    <div class="note nx">Para en el primero de los tres que llegue.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Para en el primero de los tres que llegue. El tope de tiempo
      es el que hace que la lista quepa en una noche: sin él, un board que no
      alcanza la precisión se come las horas de los demás.</span></div>

    <div class="row" style="margin-top:8px">
      <div><label>Nombre del spot guardado</label>
        <input type="text" id="scrName" value="script"></div>
      <div><label>Nombre de cada árbol</label>
        <input type="text" id="scrPat" value="{board}"></div>
      <div class="chk"><input type="checkbox" id="scrSave" checked>
        <label for="scrSave">guardar el árbol de cada board</label></div>
    </div>
    <div class="note nx">El nombre admite <code>{board}</code> y <code>{n}</code>.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det"><code>{board}</code> es el board y <code>{n}</code> el número
      de orden. Sin guardar, el script resuelve y no deja nada: sirve para medir
      cuánto tarda la lista antes de comprometer el disco.</span></div>

    <div class="row" style="margin-top:12px">
      <button class="primary" onclick="generaScript()">Generar</button>
      <button class="sm" onclick="copiaScript()">Copiar</button>
      <button class="sm" onclick="bajaScript()">Descargar</button>
      <button class="sm" onclick="cierraScript()">Cerrar</button>
    </div>
    <div class="note" id="scrNote"></div>
    <textarea id="scrOut" rows="12" spellcheck="false"
              style="margin-top:8px;font-family:ui-monospace,monospace;font-size:11px"></textarea>
   </div>
  </div>

  <div id="szDlg" onclick="if(event.target===this)closeSizeHelp()">
   <div id="szBox">
    <h2>Cómo se escriben los tamaños</h2>
    <div class="note manual" style="font-size:13px;line-height:1.65">
      Una lista separada por <b>comas, espacios o punto y coma</b>. Da igual:
      <code>33 75</code> y <code>33,75</code> son lo mismo.

      <br><br><b>En el campo de tamaños de bet</b>
      <table style="width:100%;margin-top:6px;border-collapse:collapse">
        <tr><td style="width:120px;padding:3px 0"><code>30</code></td>
            <td>un 30% del bote</td></tr>
        <tr><td style="padding:3px 0"><code>75</code> o <code>75%</code></td>
            <td>tres cuartos del bote; el <code>%</code> es opcional</td></tr>
        <tr><td style="padding:3px 0"><code>100</code> o <code>pot</code></td>
            <td>el bote entero</td></tr>
        <tr><td style="padding:3px 0"><code>300</code></td>
            <td>tres botes, una sobreapuesta</td></tr>
        <tr><td style="padding:3px 0"><code>none</code></td>
            <td>esa calle no apuesta</td></tr>
      </table>
      <br>Los tamaños van <b>en por ciento del bote</b>, como se dicen en una
      mesa. Un decimal pequeño se rechaza a propósito: <code>0.75</code> ya no
      son tres cuartos, se escribe <code>75</code>.

      <br><br><b>En el campo de tamaños de raise</b>
      <table style="width:100%;margin-top:6px;border-collapse:collapse">
        <tr><td style="width:120px;padding:3px 0"><code>3x</code></td>
            <td>subir <b>por</b> 3 veces lo que hay que pagar, encima de lo que
                ya llevas puesto. Sobre una apuesta de 100: <b>300</b>; si el
                rival vuelve a subir, <b>700</b>, y luego <b>1500</b>. Es la
                cuenta estándar.</td></tr>
        <tr><td style="padding:3px 0"><code>2.5x</code></td>
            <td>lo mismo con decimal. El decimal es el <b>punto</b>, que es lo
                que el programa te devuelve escrito en el campo al construir.
                Con coma, <code>2,5x</code>, también vale: aquí la coma
                <b>no separa</b>, porque un número suelto no es una subida.</td></tr>
        <tr><td style="padding:3px 0"><code>min</code></td>
            <td>la subida <b>minima legal</b>, la que sea en cada nivel: sobre
                una apuesta de 12 sube a 24, sobre esa a 36, luego 48. Sale lo
                mismo que <code>2x</code>, porque la minima es justo subir por
                una vez lo que pagas.</td></tr>
        <tr><td style="padding:3px 0"><code>none</code></td>
            <td>nadie sube en esa calle</td></tr>
      </table>
      <br>Aqui <b>no</b> valen porcentajes: <code>50</code> se rechaza. En este
      campo solo hay una cuenta, y así no hay que mirar la letra para saber cual.

      <br><b>Dos cosas que sorprenden</b>
      <br>• Dos tamaños que se diferencien menos de un <b>10%</b> se funden
      en uno. <code>50,52</code> da una sola rama: dos apuestas casi iguales
      cuestan el doble y no ensenan nada distinto.
      <br>• El <b>all-in</b> no se escribe en la lista, se marca con la
      casilla <i>Añadir allin</i> de cada calle. Y una apuesta que comprometa más
      del umbral (engranaje → Avanzado) se convierte en all-in ella sola.
      <br>• Como mucho <b>8 acciones</b> por nodo, contando pasar, pagar y
      retirarse. Pasarse da un error, no un árbol raro.

      <br><br>Cada tamaño extra <b>multiplica</b> el árbol: en un flop, pasar de
      uno a dos casi lo triplica. Si la memoria aprieta, recorta tamaños antes
      que rangos.
    </div>
    <div class="row" style="margin-top:12px">
      <button class="primary" onclick="closeSizeHelp()">Entendido</button>
    </div>
   </div>
  </div>

  <div id="lockDlg" onclick="if(event.target===this)closeLockDlg()">
   <div id="lockBox">
    <h2>Nodelock <span id="lockWhere" style="color:var(--fg);text-transform:none;
        letter-spacing:0;font-weight:400"></span></h2>
    <div class="note" id="lockWhy" style="display:none;font-size:13px;line-height:1.6"></div>
    <div id="lockBody">
    <div class="row" style="margin-bottom:8px">
      <label class="tick"><input type="radio" name="lkview" value="edited" checked
        onchange="renderLockDlg()">Estrategia editada</label>
      <label class="tick"><input type="radio" name="lkview" value="orig"
        onchange="renderLockDlg()">Original</label>
      <div></div>
    </div>
    <div class="lkacts" id="lockActs"></div>
    <div class="lkwrap">
      <div class="lkleft">
        <div class="grid strat" id="lockGrid"></div>
        <div class="note nx" style="margin-top:8px">Palos que entran al pintar una casilla.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Palos que entran al pintar una casilla.
          <b>Arriba el palo de la carta alta, abajo el de la baja.</b> Ninguno marcado en
          una fila = todos. Marcar ♥ arriba y ♠ abajo elige <b>ese</b> combo
          offsuit y no el simetrico; marcar el mismo palo en las dos filas elige el suited.</span></div>
        <div id="lockSuits"></div>
        <div class="note nx" style="margin-top:8px">Los combos de la casilla que pinches.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Combos de la casilla que pinches —
          el nombre o las barras pintan ese combo suelto, y pinchar otra vez se lo
          quita; <b>−</b> y <b>+</b> mueven <b>solo esa mano</b> 5 puntos de la
          acción elegida arriba</span></div>
        <div id="lockCombos"></div>
        <div class="note nx">Cada casilla lleva el color de la acción de arriba.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Cada casilla se pinta del color de la acción elegida arriba,
          llena hasta el peso que tiene en ella — el mismo número que aparece debajo del
          nombre.</span></div>
      </div>
      <div class="lkright">
        <label>Peso que pinta el raton, para <b id="lockActName">la acción</b></label>
        <div class="lkbig" id="lockWval">100%</div>
        <input type="range" id="lockSlider" min="0" max="100" step="1" value="100"
               oninput="lockPreview()" onchange="lockPreview()">
        <div class="row" style="margin-top:6px">
          <button class="sm" onclick="lkSetW(0)">0%</button>
          <button class="sm" onclick="lkSetW(50)">50%</button>
          <button class="sm" onclick="lkSetW(100)">100%</button>
        </div>
        <div class="note nx" style="margin-top:8px">Pincha o arrastra: pintar ya es cambiarlo.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Pincha o arrastra una mano y le pone
          ese peso en esa acción; el resto de sus acciones se reparten lo que queda en
          proporcion a lo que ya tenian. No hay que confirmar nada: pintar ya es
          cambiarlo.</span></div>
        <div class="row" style="margin-top:6px">
          <label class="tick"><input type="radio" name="lkmode" value="fixed" checked
            onchange="lockPreview()">Fijo</label>
          <label class="tick"><input type="radio" name="lkmode" value="scale"
            onchange="lockPreview()">Proporcional</label>
        </div>
        <div class="note" id="lockModeNote"></div>
        <div class="row" style="margin-top:8px">
          <button class="sm" onclick="lkPaintAll()">Pintar todo el rango</button>
          <button class="sm" onclick="lockUndoAll()">Deshacer cambios</button>
        </div>
        <div class="note nx" style="margin-top:6px">Se puede repetir por todo el rango antes de fijar.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Puedes repetir esto por todo el rango
          —elegir, mover, elegir otra cosa, mover— y fijarlo todo de una vez al final.
          Nada llega al solver hasta <b>Fijar y cerrar</b>.</span></div>
        <div class="note" id="lockTouched" style="font-size:12px;color:var(--fg)"></div>
        <div class="row" style="margin-top:6px">
          <button class="sm" onclick="lockFreezeAll()">Congelar lo que no hayas tocado</button>
          <div></div>
        </div>
        <div class="note nx" style="margin-top:4px">Deja el resto del rango clavado donde está.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Deja clavado el resto del rango
          donde esta, sin cambiarlo. Sirve porque al bloquear solo unas manos el
          solver sigue pudiendo mover las demas para compensar: si tu lectura es
          <i>«las fuertes siempre apuestan»</i> y no dice nada de sus
          faroles, congelar el resto impide que se los reajuste solo. Respeta lo
          que ya hayas pintado.</span></div>
        <div class="row" style="margin-top:10px">
          <button class="sm" onclick="lockCommit()">Fijar y cerrar</button>
          <button class="sm" onclick="closeLockDlg()">Cancelar</button>
        </div>
      </div>
    </div>
    </div>
    <div class="row" style="margin-top:10px">
      <button class="sm" onclick="closeLockDlg()">Cerrar</button>
      <div></div>
    </div>
   </div>
  </div>
  <div class="hud" id="hud">
    <div class="hudrow">
      <div class="hudside oop" id="hudOOP">
        <div class="who">OOP</div><div class="ev" id="evOOP">-</div>
        <div class="sub" id="subOOP"></div>
      </div>
      <div class="hudside ip" id="hudIP">
        <div class="who">IP</div><div class="ev" id="evIP">-</div>
        <div class="sub" id="subIP"></div>
      </div>
    </div>
    <div class="split"><div class="a" id="splitA"></div><div class="b" id="splitB"></div></div>
    <div class="hudmeta" id="hudMeta"></div>
  </div>

  <div class="panel">
    <h2>Árbol de decisión</h2>
    <div class="navboard" id="navBoard"></div>
    <div class="ptree" id="ptree"></div>
    <div id="cardPick"></div>
    <div class="note" id="navNote"></div>
  </div>

  <div class="panel" id="runoutPanel" style="display:none">
    <h2>Cada carta <span id="runoutTitle" style="color:var(--fg);text-transform:none;
        letter-spacing:0;font-weight:400"></span></h2>
    <div class="note nx" style="margin:0 0 8px">Están resueltos todos: esto no es una muestra.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">Todos los runouts están resueltos, así
      que esto no es una muestra. Pincha una cabecera para ordenar; pincha una
      carta para repartirla.</span></div>
    <div id="runoutBody"></div>
  </div>

  <div class="panel">
    <h2>Estrategia <span id="nodeTitle" style="color:var(--fg);text-transform:none;
        letter-spacing:0;font-weight:400"></span></h2>
    <div class="row" style="margin-bottom:8px">
      <button class="sm needsIdle" onclick="openLockDlg()">Nodelock</button>
      <div></div><div></div>
    </div>
    <div class="kv" id="nodeInfo"></div>
    <div class="acts" id="actBtns"></div>
    <div class="bar" id="freqBar"></div>
    <div class="legend" id="legend"></div>
    <div class="row" style="margin-bottom:8px">
      <div style="flex:0 0 160px">
        <select id="gridMode" onchange="renderStrategy()">
          <option value="strat">Color: estrategia</option>
          <option value="eq">Color: equity</option>
          <option value="ev">Color: EV</option>
        </select>
        <label style="display:inline-flex;align-items:center;gap:5px;margin:0 0 0 10px;
               font-size:11px;color:var(--dim);cursor:pointer">
          <input type="checkbox" id="gridWeight" onchange="renderStrategy()">
          cuadro proporcional al peso</label>
      </div>
      <div></div>
    </div>
    <div class="grid strat" id="stratGrid"></div>
  </div>

  <div class="panel" id="madePanel" style="display:none">
    <h2>Por mano hecha</h2>
    <div class="note nx">Qué tienes <b>hecho</b>, no qué cartas tienes. Las celdas de acción son <b>barras</b>: arrástralas.<span class="q" onclick="abreNota(this)" title="ver la explicación">i</span><span class="det">La rejilla dice que <b>cartas</b> tienes; esto dice que
      tienes <b>hecho</b>. En un board dado los tríos están repartidos por toda
      la rejilla, así que ahí no se ven juntos.
      <b>Cada celda de acción es una barra</b>: arrástrala y mueves la familia
      entera de golpe -- todas las dobles parejas a apostar, por ejemplo --
      sin pintar un combo. El botón ↺ que aparece al lado del
      nombre la devuelve a lo que hacía la solución.</span></div>
    <table id="madeTable"></table>
  </div>


  <div class="panel" id="comboPanel" style="display:none">
    <h2>Combos <span id="comboTitle" style="color:var(--fg);text-transform:none;
        letter-spacing:0;font-weight:400"></span></h2>
    <label class="tick" style="margin-bottom:8px" title="Recorre el árbol entero: cuesta como una iteracion">
      <input type="checkbox" id="brChk" onchange="toggleBR()">
      Mejor respuesta: cuanto gana quien juega para explotar esta solución</label>
    <table id="comboTable"></table>
  </div>


</div>
</main>

<section class="trainer" id="trainer">
  <div class="tbar">
    <button class="sm" onclick="trainClose()">◀ Volver</button>
    <b>Entrenador</b>
    <span class="pill">mano <b id="tHand">-</b></span>
    <span class="pill" id="tSeedPill" title="Con la misma semilla salen las mismas cartas y el mismo runout">semilla <b id="tSeed">-</b></span>
    <button class="sm" id="tStartBtn" onclick="startPick()"
            title="elige el punto del árbol donde quieres que empiecen las manos">Empezar en:</button>
    <span class="pill">juegas de
      <button class="sm" id="tSideO" onclick="trainSide(0)">OOP</button>
      <button class="sm" id="tSideI" onclick="trainSide(1)">IP</button>
    </span>
    <span class="sp"></span>
    <label class="tick"><input type="checkbox" id="tAdvice" onchange="trainAdvice()">ver el consejo</label>
    <label class="tick" title="Al acabar una mano espera un par de segundos, para que te dé tiempo a ver lo que costó, y reparte la siguiente"><input type="checkbox" id="tAuto">Mano automática</label>
    <button class="sm" onclick="trainRepeat()" title="La misma mano otra vez: mismas cartas, mismo runout">Repetir esta mano</button>
    <button class="sm" onclick="trainNew()">Otra mano</button>
    <span class="teclas" title="F retirarse · X pasar · C pagar · 1, 2… apostar o subir · N otra mano · R repetirla · A el consejo">teclas</span>
  </div>

  <div class="tgrid">
    <div class="felt">
      <div class="mesa">
        <div class="silla arriba">
          <div class="chapa" id="tVillPlate">
            <span class="av">BOT</span>
            <span class="quien">
              <span class="nick">DCFR Bot</span>
              <span class="stk">stack <b id="tVillStack">-</b></span>
            </span>
            <span class="pos" id="tVillWho"></span>
            <span class="dealer" id="tVillD" style="display:none">D</span>
          </div>
          <div class="hand" id="tVill"></div>
        </div>

        <div class="apuesta arriba" id="tVillFront"></div>

        <div class="centro">
          <div class="potline"><span class="chip"></span>bote <b id="tPot">-</b></div>
          <div class="boardrow" id="tBoard"></div>
        </div>

        <div class="apuesta abajo" id="tHeroFront"></div>

        <div class="silla abajo">
          <div class="hand" id="tHero"></div>
          <div class="chapa" id="tHeroPlate">
            <span class="av">TU</span>
            <span class="quien">
              <span class="nick">Hero</span>
              <span class="stk">stack <b id="tHeroStack">-</b></span>
            </span>
            <span class="pos" id="tHeroWho"></span>
            <span class="dealer" id="tHeroD" style="display:none">D</span>
          </div>
        </div>
      </div>

      <div class="tlast" id="tLast" style="display:none"></div>
      <div class="tmsg" id="tMsg" style="display:none"></div>
      <div class="tactions" id="tActions"></div>
      <div class="tend" id="tEnd" style="display:none"></div>
    </div>

    <aside class="tside">
      <div class="box">
        <h3>La situación</h3>
        <table>
          <tr><td class="k">Calle</td><td class="v" id="tsStreet">-</td></tr>
          <tr><td class="k">Eres</td><td class="v" id="tsSide">-</td></tr>
          <tr><td class="k">Bote total</td><td class="v" id="tsPot">-</td></tr>
          <tr><td class="k">Por pagar</td><td class="v" id="tsCall">-</td></tr>
          <tr><td class="k">SPR</td><td class="v" id="tsSPR">-</td></tr>
          <tr><td class="k">Tienes</td><td class="v" id="tsYou">-</td></tr>
        </table>
      </div>

      <div class="box">
        <h3>Lo que ha pasado</h3>
        <ol class="tlog" id="tLog"></ol>
      </div>

      <div class="box">
        <h3>Consejo</h3>
        <div id="tAdv"></div>
      </div>

      <div class="box">
        <h3>Marcador</h3>
        <table>
          <tr><td class="k">Manos</td><td class="v" id="tmHands">0</td></tr>
          <tr><td class="k">Decisiones</td><td class="v" id="tmDec">0</td></tr>
          <tr><td class="k">EV perdido</td><td class="v" id="tmLost">0</td></tr>
          <tr><td class="k">Por decisión</td><td class="v" id="tmPer">0</td></tr>
          <tr><td class="k">Fichas</td><td class="v" id="tmWon">0</td></tr>
        </table>
        <div id="tmGrade" style="margin-top:8px"></div>
        <button class="sm" onclick="trainReset()" style="margin-top:8px">Empezar de cero</button>
      </div>

      <div class="box" id="tHistBox" style="display:none">
        <h3>Manos jugadas</h3>
        <div class="hist" id="tHist"></div>
        <div class="advoff" style="margin-top:6px">Pincha una para volver a jugarla igual.</div>
      </div>

      <div class="box" id="tRevBox" style="display:none">
        <h3>Lo que costó cada decisión</h3>
        <table class="rev" id="tRev"></table>
      </div>
    </aside>
  </div>
</section>

<script>
const RANKS=['A','K','Q','J','T','9','8','7','6','5','4','3','2'];
const NT_DECISION=0, NT_CONT=1, NT_SHOWDOWN=2, NT_FOLD=3;
let state=null, node=null, curCls=-1, tab=0;
let nav={ctx:0,node:0,slots:[]};
// Que accion se esta mirando a solas, o -1 si el rango entero.
let actFilter=-1;
// Y que categoria de mano, o null. Se combinan: se puede mirar el color que
// apuesta.
let madeFilter=null;
function comboIn(c){
  // La familia es de mano hecha (kind 0) o de proyecto (kind 1), nunca las dos
  // cruzadas: son dos listas independientes y "set + flush_draw" no es una
  // familia suya, es `set & flush_draw` en su lenguaje de filtros.
  if(!madeFilter) return true;
  return madeFilter.kind===1 ? c.di===madeFilter.cls : c.mi===madeFilter.cls;
}
let line=[], sel=0;
let nodeSeq=0, liveTick=0, solvingLive=false, nodePending=false;
let ranges=[new Array(169).fill(0),new Array(169).fill(0)];
let painting=false, paintVal=0;

// Un numero se escribe SIEMPRE igual, lo lea quien lo lea.
//
// `toLocaleString()` dejaba la decision al navegador: el mismo arbol ensenaba
// "43.271 nodos" en un Chrome en espanol y "43,271 nodos" en uno en ingles, y
// al lado, en la misma linea, un "0.53 GB" que no cambia nunca. En la primera
// version el punto significa dos cosas seguidas -- miles y decimal -- y no hay
// manera de saber cual es cual.
//
// Aqui el decimal es el PUNTO, que es lo que usa todo lo demas: el fichero
// guardado escribe "2.5x", el CSV y el JSON llevan punto porque son datos, la
// consola tambien, y la referencia igual. Los miles se separan con un espacio fino,
// que no se confunde con ninguna de las dos cosas en ningun idioma.
// ------------------------------------------------------------------ idiomas
//
// El español vive donde siempre: en el marcado y en el código. Aquí solo está
// el inglés, y la tabla se lee POR EL TEXTO EN ESPAÑOL, no por una clave
// inventada. Así nadie tiene que mantener dos ficheros en paralelo ni buscar
// qué es 'lbl_47': lo que ves en el fuente es lo que se traduce.
//
// Y no se traduce llamando a una función en cada sitio -- son cerca de
// cuatrocientos -- sino mirando lo que de verdad llega a la pantalla: un
// MutationObserver ve cada trozo de texto que aparece, y si está en la tabla lo
// cambia. Lo que no está se queda en español, que es el comportamiento correcto
// para un texto que todavía no se ha traducido: se ve, y se ve que falta.
const EN = {
// -- barra de arriba y pastillas
'parado':'idle', 'lista':'ready', 'resolviendo':'solving', 
 'cargando':'loading', 'sin resolver':'not solved',
'sin locks':'no locks', 'opciones':'options',
'Idioma / Language':'Idioma / Language',
'GPL v3, sin garantía. Se reparte tal cual.':'GPL v3, no warranty. Shared as is.',
'esconder el montaje y dejarle el ancho a la solución':
  'hide the setup and give the width to the solution',
// -- board
'Board':'Board', 'Elegir flop':'Pick flop', 'Poner':'Set',
'Montaje':'Setup',
// -- rangos
'Rangos':'Ranges', 'Peso de lo que pintes:':'Painting weight:',
'Rango en texto':'Range as text', 'Formato de texto:':'Text format:',
'. Las manos puras van sin peso y las parciales con':
  '. Pure hands carry no weight, partial ones use',
'. Se pega tal cual desde cualquier librería de rangos o desde otro solver, y lo que sale de la cuadrícula se pega en ellos igual.':
  '. Paste it straight from any range library or another solver, and what comes out of the grid pastes back into them just the same.',
'Aplicar texto':'Apply text', 'Limpiar':'Clear', 'Guardar':'Save',
'Cargar':'Load', 'Borrar':'Delete', 'nombre del rango':'range name',
'Se guardan':'It saves',
'los dos rangos juntos':'both ranges together',
', OOP e IP: un rango es de un spot concreto y solo dice algo con el del otro lado al lado. Cargarlo pone los dos.':
  ', OOP and IP: a range belongs to one spot, and only says something with the other side next to it. Loading it sets both.',
// -- montaje del arbol
'Montaje del árbol':'Tree building', 'Bote inicial':'Starting pot',
'Stack efectivo':'Effective stack',
'Precisión deseada (% del bote)':'Accuracy target (% of pot)',
'parar el cálculo al alcanzar la precisión deseada':
  'stop the solve once it reaches the target',
'Hilos':'Threads',
'Cuanto más bajo el número, más fina la solución y más tarda. El solver para solo cuando la alcanza y te dice que la alcanzó. Nunca te dará peor de lo que pides.':
  'The lower the number, the finer the solution and the longer it takes. The solver stops on its own when it gets there, and says so. It will never hand you worse than you asked for.',
'Avanzado':'Advanced', 'Limite de memoria (GB)':'Memory limit (GB)',
'Tope de iteraciones':'Iteration cap', 'Rake %':'Rake %', 'Tope de rake':'Rake cap',
'Sin rake por defecto. Con rake el juego deja de ser suma cero: OOP + IP ya no suman el bote, y la diferencia es lo que se lleva la casa.':
  'No rake by default. With rake the game stops being zero-sum: OOP + IP no longer add up to the pot, and the difference is the house cut.',
'All-in si compromete más del %':'All-in if it commits more than %',
'El all-in es del stack efectivo':'The all-in is of the effective stack',
'inicial':'at the start',
', no del bote. Con dos tercios dentro ya estas comprometido: el tercio de detras no compra ninguna decision, solo ramas. 100 lo desactiva.':
  ', not of the pot. With two thirds in you are committed already: the third behind buys no decision, only branches. 100 turns it off.',
'El limite de memoria sale del':'The memory limit comes from',
'75% de la RAM de esta máquina':'75% of this machine RAM',
', así que normalmente no hay que tocarlo. El árbol se rechaza':
  ', so normally there is nothing to touch. The tree is refused',
'antes':'before',
'de reservar nada: subirlo no rompe nada, como mucho el sistema empieza a tirar de disco y todo va despacio.':
  'anything is allocated: raising it breaks nothing, at worst the system starts swapping and everything crawls.',
'El tope de iteraciones es una red de seguridad, no el criterio: corta el cálculo si la precisión no se alcanza nunca. En condiciones normales no llega a actuar.':
  'The iteration cap is a safety net, not the criterion: it cuts the solve off if the accuracy target is never reached. Normally it never fires.',
'El árbol se rechaza por encima de esto, antes de reservar nada':
  'The tree is refused above this, before anything is allocated',
'Red de seguridad: corta el cálculo aunque no se alcance la precisión':
  'Safety net: it cuts the solve off even if the target is never reached',
'Porcentaje del bote igualado. 0 = sin rake, que es lo correcto para torneo y para teoria':
  'Per cent of the matched pot. 0 = no rake, which is right for tournaments and for theory',
'En fichas. 0 = sin tope':'In chips. 0 = no cap',
'Del stack efectivo INICIAL. Una apuesta que comprometa más de esto se convierte en all-in limpio. 67 es el valor habitual; 100 lo desactiva':
  'Of the STARTING effective stack. A bet committing more than this becomes a clean all-in. 67 is the usual value; 100 turns it off',
'Montar árbol':'Build tree', 'Resolver':'Solve', '+ iteraciones':'+ iterations',
'Stop':'Stop',
// -- calles (se montan desde el codigo)
'Tamaños de bet (x bote)':'Bet sizings (x pot)', 'Tamaños de raise':'Raise sizings',
'Tamaños de donk':'Donk sizings', 'Añadir allin':'Add allin', 'Sin 3-bet':'No 3-bet',
'Copiar de IP a OOP':'Copy IP to OOP',
'OOP liderando contra quien fue agresivo en la calle anterior. Vacio (none) = no lidera':
  'OOP leading into last street aggressor. Empty (none) = no lead',
'3x = subir hasta 3 veces el bet · 0.5 = medio bote encima':
  '3x = raise to 3 times the bet · 0.5 = half a pot on top',
'Anade una rama de all-in a pelo, ademas de los tamaños':
  'Adds a bare all-in branch on top of the sizings',
'IP no hace la tercera acción agresiva de la calle: apuesta IP, sube OOP, IP ya no resube':
  'IP does not make the third aggressive action of the street: IP bets, OOP raises, IP does not re-raise',
// -- guardar
'Una':'A', 'config':'config',
'es la receta: board, rangos y tamaños. Unos pocos KB. Un':
  'is the recipe: board, ranges and sizings. A few KB. A',
'árbol':'tree',
'es la receta más la solución ya calculada: pesa lo que pese, pero vuelve al spot sin recalcular y puedes seguir iterando donde lo dejaste.':
  'is the recipe plus the solution already computed: it weighs what it weighs, but it brings the spot back without recomputing and you can carry on iterating where you left off.',
'Nombre':'Name', 'Guardar config':'Save config', 'Guardar árbol':'Save tree',
'Configs guardados':'Saved configs', 'Árboles guardados':'Saved trees',
// -- ayuda de tamaños
'Cómo se escriben los tamaños':'How sizings are written',
'Una lista separada por':'A list separated by',
'comas, espacios o punto y coma':'commas, spaces or semicolons',
'. Da igual:':'. It makes no difference:', 'y':'and', 'son lo mismo.':'are the same.',
'En el campo de tamaños de bet':'In the bet sizing field',
'un 30% del bote':'30% of the pot', 'o':'or',
'tres cuartos del bote; el':'three quarters of the pot; the',
'es opcional':'is optional', 'pot':'pot', 'el bote entero':'the whole pot',
'tres botes, una sobreapuesta':'three pots, an overbet',
'none':'none', 'esa calle no apuesta':'that street does not bet',
'Los tamaños van':'Sizings go',
'en por ciento del bote':'in per cent of the pot',
', como se dicen en una mesa. Un decimal pequeño se rechaza a propósito:':
  ', the way they are said at a table. A small decimal is refused on purpose:',
'ya no son tres cuartos, se escribe':'is not three quarters any more, write',
'En el campo de tamaños de raise':'In the raise sizing field',
'subir':'raise', 'por':'by',
'3 veces lo que hay que pagar, encima de lo que ya llevas puesto. Sobre una apuesta de 100:':
  '3 times what you have to call, on top of what you already have in. Over a bet of 100:',
'; si el rival vuelve a subir,':'; if they raise again,',
', y luego':', and then', '. Es la cuenta estándar.':'. That is the standard arithmetic.',
'lo mismo con decimal. El decimal es el':'the same with a decimal. The decimal mark is the',
'punto':'dot',
', que es lo que el programa te devuelve escrito en el campo al construir. Con coma,':
  ', which is what the program writes back into the field when it builds. With a comma,',
', también vale: aquí la coma':', it works too: here the comma',
'no separa':'does not separate',
', porque un número suelto no es una subida.':', because a bare number is not a raise.',
'min':'min', 'la subida':'the', 'minima legal':'smallest legal raise',
', la que sea en cada nivel: sobre una apuesta de 12 sube a 24, sobre esa a 36, luego 48. Sale lo mismo que':
  ', whatever it is at each level: over a bet of 12 it raises to 24, over that to 36, then 48. It comes out the same as',
', porque la minima es justo subir por una vez lo que pagas.':
  ', because the minimum is exactly raising by one times what you call.',
'nadie sube en esa calle':'nobody raises on that street',
'Aqui':'Here', 'no':'no', 'valen porcentajes:':'percentages do not work:',
'se rechaza. En este campo solo hay una cuenta, y así no hay que mirar la letra para saber cual.':
  'is refused. There is one arithmetic in this field, so you never have to read the suffix to know which.',
'Dos cosas que sorprenden':'Two things that surprise people',
'• Dos tamaños que se diferencien menos de un':'• Two sizings closer than',
'se funden en uno.':'are merged into one.',
'da una sola rama: dos apuestas casi iguales cuestan el doble y no ensenan nada distinto.':
  'gives a single branch: two near-identical bets cost twice as much and show nothing different.',
'• El':'• The', 'all-in':'all-in',
'no se escribe en la lista, se marca con la casilla':'is not written in the list, it is ticked with',
'de cada calle. Y una apuesta que comprometa más del umbral (engranaje → Avanzado) se convierte en all-in ella sola.':
  'on each street. And a bet committing more than the threshold (gear → Advanced) becomes an all-in on its own.',
'• Como mucho':'• At most', '8 acciones':'8 actions',
'por nodo, contando pasar, pagar y retirarse. Pasarse da un error, no un árbol raro.':
  'per node, counting check, call and fold. Going over is an error, not a strange tree.',
'Cada tamaño extra':'Each extra sizing', 'multiplica':'multiplies',
'el árbol: en un flop, pasar de uno a dos casi lo triplica. Si la memoria aprieta, recorta tamaños antes que rangos.':
  'the tree: on a flop, going from one to two nearly triples it. If memory is tight, trim sizings before ranges.',
'Entendido':'Got it',
// -- nodelock
'Nodelock':'Nodelock', 'Estrategia editada':'Edited strategy', 'Original':'Original',
'Palos que entran al pintar una casilla.':'Suits that get painted when you click a square.',
'Arriba el palo de la carta alta, abajo el de la baja.':
  'Top row is the high card suit, bottom row the low one.',
'Ninguno marcado en una fila = todos. Marcar ♥ arriba y ♠ abajo elige':
  'None ticked in a row = all of them. Ticking ♥ on top and ♠ below picks',
'ese':'that',
'combo offsuit y no el simetrico; marcar el mismo palo en las dos filas elige el suited.':
  'offsuit combo and not its mirror; ticking the same suit in both rows picks the suited one.',
'mueven':'move', 'solo esa mano':'that hand only',
'5 puntos de la acción elegida arriba':'5 points of the action picked above',
'Cada casilla se pinta del color de la acción elegida arriba, llena hasta el peso que tiene en ella — el mismo número que aparece debajo del nombre.':
  'Each square is painted in the colour of the action picked above, filled to the weight it has in it — the same number shown under the name.',
'Peso que pinta el raton, para':'The weight the mouse paints, for',
'la acción':'the action',
'Pincha o arrastra una mano y le pone ese peso en esa acción; el resto de sus acciones se reparten lo que queda en proporcion a lo que ya tenian. No hay que confirmar nada: pintar ya es cambiarlo.':
  'Click or drag a hand and it takes that weight in that action; its other actions share what is left in proportion to what they already had. Nothing to confirm: painting is changing it.',
'Fijo':'Fixed', 'Proporcional':'Proportional',
'Pintar todo el rango':'Paint the whole range', 'Deshacer cambios':'Undo changes',
'Fijar y cerrar':'Lock and close', 'Cancelar':'Cancel', 'Cerrar':'Close',
'Congelar lo que no hayas tocado':'Freeze what you did not touch',
'Deja clavado el resto del rango donde esta, sin cambiarlo. Sirve porque al bloquear solo unas manos el solver sigue pudiendo mover las demas para compensar: si tu lectura es':
  'Pins the rest of the range where it is. It matters because locking only a few hands leaves the solver free to move the others to compensate: if your read is',
'y no dice nada de sus faroles, congelar el resto impide que se los reajuste solo. Respeta lo que ya hayas pintado.':
  'and says nothing about their bluffs, freezing the rest stops it quietly re-tuning them. It respects whatever you already painted.',
// -- columna derecha
'Árbol de decisión':'Decision tree', 'Cada carta':'Every card',
'Todos los runouts están resueltos, así que esto no es una muestra. Pincha una cabecera para ordenar; pincha una carta para repartirla.':
  'Every runout is solved, so this is not a sample. Click a header to sort; click a card to deal it.',
'Estrategia':'Strategy', 'Color: estrategia':'Colour: strategy',
'Color: equity':'Colour: equity', 'Color: EV':'Colour: EV',
'cuadro proporcional al peso':'square scaled to the weight',
'Por mano hecha':'By made hand', 'La rejilla dice que':'The grid says which',
'cartas':'cards', 'tienes; esto dice que tienes':'you hold; this says what you have',
'hecho':'made',
'. En un board dado los tríos están repartidos por toda la rejilla, así que ahí no se ven juntos.':
  '. On a given board the sets are scattered all over the grid, so you never see them together there.',
'Cada celda de acción es una barra':'Every action cell is a bar',
'Combos':'Combos',
'Mejor respuesta: cuanto gana quien juega para explotar esta solución':
  'Best response: what a perfect exploiter gains against this solution',
'Recorre el árbol entero: cuesta como una iteracion':
  'It walks the whole tree: it costs about one iteration',
'resuelve para ver el reparto':'solve to see the split',
'resuelve para ver la estrategia':'solve to see the strategy',
// -- lo que se arma desde el codigo: estado, avisos, tablas
'Español':'Español',
'el solver respondió':'the solver answered',
'sin respuesta del solver — ¿sigue abierto en esta máquina?':
  'no answer from the solver — is it still open on this machine?',
'sin respuesta del solver':'no answer from the solver',
'sin conexión con el solver':'no connection to the solver',
'(0 = los':'(0 = all', 'combos en este board,':'combos on this board,',
'por salir':'still to come', 'desde el':'from the',
'runouts en uno':'runouts in one', 'nodos':'nodes',
'Dos runouts que solo se diferencian en el palo dan la misma':
  'Two runouts that differ only in suit give the same',
'estrategia, así que se resuelven una sola vez: de ahí que el solve':
  'strategy, so they are solved once, which is why the solve',
'cueste':'costs', 'veces menos.':'times less.',
'Este board da para juntar':'This board would group',
', y con tus':', and with your',
'rangos y locks aguantan':'ranges and locks it holds',
'. Si el lock lo haces':'. Lock by',
'por rangos, vuelve el resto.':'ranges instead and the rest comes back.',
'Es todo lo que este board da de sí, y tus rangos y locks lo':
  'That is everything this board has, and your ranges and locks keep it',
'Cada runout se resuelve por su cuenta: hay un lock puesto sobre':
  'Every runout is solved on its own: there is a lock on',
'cartas concretas y eso rompe la simetría de palos del board. Está':
  'named cards, and that breaks the board suit symmetry. It is',
'bien hecho, pero cuesta varias veces más. Si el lock lo haces por':
  'correct, but it costs several times more. Lock by',
'rangos o por buckets, la simetría vuelve.':
  'ranges or by buckets instead and the symmetry comes back.',
'Cada runout se resuelve por su cuenta: o el board no tiene dos':
  'Every runout is solved on its own: either the board has no two',
'palos intercambiables (arcoíris), o uno de los rangos no es':
  'interchangeable suits (rainbow), or one of the ranges is not',
'simétrico en palos.':'suit-symmetric.',
'lock sin aplicar':'lock not applied yet', 'locks sin aplicar':'locks not applied yet',
'sigue desde donde esta, sin tirar lo hecho':
  'carries on from where it is, without throwing away the work',
'primero hay que solvear algo':'solve something first',
'el flop ya está puesto — quita una carta primero':
  'the flop is set — take a card off first',
'quitar del flop':'take off the flop', 'poner en el flop':'put on the flop',
'quitar':'take off',
'la reparte la línea en la que estás':'the line you are on deals it',
'para el flop':'for the flop',
'copiado en OOP: dale a Aplicar':'copied into OOP: press Apply',
'? cómo se escriben':'? how they are written',
'raise:':'raise:', '= 3 veces el bet ·':'= 3 times the bet ·',
'= medio bote encima':'= half a pot on top',
'Sin tamaños no hay acción: escribe':'No sizings, no action: write',
'para quitar una lista.':'to remove a list.',
'No hay tope de bet+raise: la cadena termina donde la termina el umbral de':
  'There is no bet+raise cap: the chain ends where the all-in threshold above',
'all-in de arriba. Dos tamaños que se diferencien menos de un 10% se':
  'ends it. Two sizings closer than 10% to each other are',
'colapsan en uno.':'merged into one.',
'combos vivos en este board':'live combos on this board',
'el rango está vacío':'the range is empty',
'Cambiar el rango':'Change the range',
'el rango se queda como estaba':'the range stays as it was',
'aplicando el rango...':'applying the range...',
'Cambiar el board':'Change the board',
'el board se queda como estaba':'the board stays as it was',
'y una explotabilidad del':'and an exploitability of',
'% del bote':'% of the pot',
'tira la solución que tienes:':'throws away the solution you have:',
'Esto no se puede deshacer. Si la quieres conservar, cancela y guarda el':
  'This cannot be undone. To keep it, cancel and save the',
'árbol primero. ¿Seguir?':'tree first. Carry on?',
'Construir el árbol':'Build the tree',
'el árbol se queda como estaba':'the tree stays as it was',
'construyendo el árbol...':'building the tree...',
'árbol construido':'tree built', 'nodos del árbol,':'nodes in the tree,',
'Este árbol no tiene subidas:':'This tree has no raises:',
'ninguna calle tiene':'no street has',
'. Escribe por ejemplo':'. Write for example',
'en las':'on whichever', 'que quieras.':'streets you want.',
'Lo guardado ocupa':'What is saved takes',
'libres en el disco':'free on the disk',
'· guardar este árbol pide':'· saving this tree needs',
'pon un nombre':'type a name',
'escribiendo el árbol, puede tardar...':'writing the tree, this can take a while...',
'cargando el árbol...':'loading the tree...',
'trabajando con el disco...':'working with the disk...',
'para el solve para cambiar esto':'stop the solve to change this',
'ponle un nombre':'give it a name',
'no hay rangos guardados':'no saved ranges',
'no se pudo':'could not',
'añadiendo iteraciones…':'adding iterations…',
'explotable en un':'exploitable for',
'% del bote por mano':'% of the pot per hand',
'no se resolvio nada':'nothing was solved',
'parado en':'stopped at', 'de':'of',
'precisión alcanzada en':'target reached in',
'resuelto en':'solved in',
'parado antes de tiempo — la estrategia vale, pero está menos convergida.':
  'stopped early — the strategy is usable, but less converged.',
'parando al terminar esta iteración...':'stopping at the end of this iteration...',
'Raíz':'Root', 'principio del árbol':'start of the tree',
'· nodo de azar, bote':'· chance node, pot',
'— aquí no hay nada que resolver, vuelve atrás en la línea':
  '— nothing to solve here, go back up the line',
'Reparte el':'Deal the',
'— las cartas que son equivalentes al permutar los palos':
  '— cards that are equivalent under a suit swap',
'comparten un solve, así que algunas de estas dan la misma':
  'share a solve, so some of these give the same',
'estrategia.':'strategy.',
'está en el board':'is on the board', 'ya repartida':'already dealt',
'sin respuesta':'no answer',
'La media pondera cada carta por su alcance: una que bloquea':
  'The average weights each card by its reach: one that blocks',
'medio rango no cuenta lo mismo que una que no bloquea nada.':
  'half a range does not count the same as one that blocks nothing.',
'leyendo el nodo...':'reading the node...',
'leyendo el nodo…':'reading the node…',
'hay un solve en marcha — los datos del nodo esperan':
  'a solve is running — the node data waits',
'Respecto al valor de todo el juego, que es lo que vale la raíz':
  'Against the value of the whole game, which is what the root is worth',
'vs la raíz':'vs the root',
'bote en el nodo':'pot at the node',
'llega el':'reached by', 'de las manos':'of hands',
'partida entera':'whole game', 'juega':'to act',
'elige un nodo de decisión para ver su reparto':
  'pick a decision node to see its split',
'= el bote':'= the pot',
'elige un nodo del árbol':'pick a node in the tree',
'aún no hay nada que leer en este nodo':'nothing to read at this node yet',
'EV del nodo':'node EV',
'pinchar otra vez para quitar el filtro':'click again to drop the filter',
'ver solo el rango que hace':'show only the range that plays',
'% en todos los runouts':'% over every runout',
'% en este runout,':'% on this runout,',
'% promediado sobre los':'% averaged over the',
'runouts guardados de esta línea':'saved runouts of this line',
'ahi no habia nada':'there was nothing there',
'categoría':'category', 'combos':'combos', 'eq%':'eq%',
'a lo que hacia':'to what it was doing',
'la solución':'the solution',
'devolver':'put back',
'arrastra para fijar que hace':'drag to set what',
'aqui':'does here',
'ningún combo de esta casilla hace':'no combo in this square does',
'Todavia no hay estrategia: pulsa':'No strategy yet: press',
'Elige un punto del':'Pick a point in the',
'árbol de decisiones':'decision tree',
'(a la derecha) antes de abrir esto.':'(on the right) before opening this.',
'Ese punto no es una decision: es un reparto de carta o un final de mano.':
  'That point is not a decision: it is a card being dealt or the end of a hand.',
'Elige uno donde alguien tenga que actuar.':'Pick one where somebody has to act.',
'Ese nodo no tiene combos que leer.':'That node has no combos to read.',
'pintar el peso de la brocha en esta mano':'paint the brush weight on this hand',
'pincha una casilla de la rejilla':'click a square in the grid',
'Fijo: esa acción pasa a ser exactamente ese porcentaje, y el resto se reparte lo que queda en proporcion a lo que ya tenía.':
  'Fixed: that action becomes exactly that percentage, and the rest share what is left in proportion to what they already had.',
'Proporcional: esa acción se multiplica por ese porcentaje (50% = la mitad de lo que hacia), y el resto recoge la diferencia en proporcion.':
  'Proportional: that action is multiplied by that percentage (50% = half of what it was doing), and the rest pick up the difference in proportion.',
'combo(s) clavados donde estaban -- todavia sin fijar':
  'combo(s) pinned where they were -- not locked in yet',
'no has cambiado nada':'you have not changed anything',
'sin converger':'not converged',
'No hay solución todavía: dale a Resolver.':'No solution yet: press Solve.',
'· locks sin aplicar':'· locks not applied yet',
'Explotabilidad: lo que le sacaria a esta estrategia un rival que la':
  'Exploitability: what a player who played perfectly against this strategy',
'jugara perfectamente, en % del bote por mano. Cuanto más bajo, más cerca':
  'would take from it, in % of the pot per hand. The lower, the closer',
'del equilibrio.':'to equilibrium.',
'Hay locks puestos que este numero todavia no lleva: es el de la':
  'There are locks this number does not include yet: it is from the',
'menos de 0.5% convergida':'under 0.5% converged',
'2 a 5% utilizable, que es para lo que se usa':'2 to 5% usable, which is what it is for',
'5 a 20% floja: mirala con cuidado':'5 to 20% loose: read it carefully',
'más de 20% sin converger, no es una estrategia':
  'over 20% not converged, this is not a strategy',
'nodo':'node',
'Quitar los de este nodo':'Remove the ones at this node',
// -- lo que arma el codigo y no estaba: salio a la vista al poner el ingles
//    de salida, que es justo para lo que sirve cambiar el idioma de fabrica.
// -- el entrenador
'Jugar':'Play', 'jugar el árbol resuelto contra la solución, mano a mano':
  'play the solved tree against the solution, hand by hand',
'◀ Volver':'◀ Back', 'Entrenador':'Trainer',
'mano':'hand', 'semilla':'seed',
'Con la misma semilla salen las mismas cartas y el mismo runout':
  'The same seed deals the same cards and the same runout',
'juegas de':'you play', 'ver el consejo':'show the advice',
'Repetir esta mano':'Replay this hand',
'La misma mano otra vez: mismas cartas, mismo runout':
  'The same hand again: same cards, same runout',
'Otra mano':'Next hand',
'La situación':'The spot', 'Calle':'Street', 'Eres':'You are',
'Bote total':'Pot (total)', 'Por pagar':'To call', 'SPR':'SPR', 'Tienes':'You have',
'Lo que ha pasado':'What has happened', 'Consejo':'Advice',
'Marcador':'Score', 'Manos':'Hands', 'Decisiones':'Decisions',
'EV perdido':'EV lost', 'Por decisión':'Per decision', 'Fichas':'Chips',
'Empezar de cero':'Start over', 'Lo que costó cada decisión':'What each decision cost',
'no se pudo':'could not do that',
'apagado: marca «ver el consejo» arriba para ver las frecuencias y el EV de cada acción mientras juegas':
  'off: tick «show the advice» above to see the frequency and the EV of every action while you play',
'no te toca decidir ahora mismo':'nothing to decide right now',
'EV en fichas, para TU mano exacta. Dos acciones que valen lo mismo son las dos correctas.':
  'EV in chips, for YOUR exact hand. Two actions worth the same are both right.',
'la mano se acabó sin showdown':'the hand ended without a showdown',
'calle':'street', 'hiciste':'you did', 'lo mejor':'the best', 'coste':'cost',
'juega unas manos y aquí sale tu nota':'play a few hands and your grade shows up here',
'impecable':'flawless', 'muy bien':'very good', 'bien':'good',
'hay fugas':'leaking', 'a repasar':'needs work',
'% del bote por decisión':'% of the pot per decision',
'elige el punto del árbol donde quieres que empiecen las manos':
  'pick the point of the tree where the hands should start',
'Empezar en:':'Start at:', 'el principio':'the beginning',
'¿Dónde empieza la mano?':'Where does the hand start?',
'Baja por el árbol hasta el punto que quieras entrenar y dale a':
  'Walk down the tree to the point you want to drill and press',
'Empezar aquí':'Start here',
'. Las cartas que falten se reparten al azar en cada mano, así que entrenas el nodo y no una carta concreta.':
  '. The cards that are missing are dealt at random every hand, so you drill the node and not one particular card.',
'Empezar en un nodo de dentro no es lo mismo que empezar por el principio y llegar hasta él: la mano se reparte con el rango que':
  'Starting at a node inside the tree is not the same as starting at the beginning and walking to it: the hand is dealt from the range that',
'llega ahí':'gets there',
', que es más estrecho. Si eliges el nodo de después de pagar una apuesta, te tocarán manos que pagan, no manos cualesquiera.':
  ', which is narrower. Pick the node after calling a bet and you will be dealt hands that call, not just any hand.',
'Volver al principio':'Back to the beginning', 'Cerrar':'Close',
'sale una carta':'a card comes', 'carta':'card',
'aquí se acaba la mano: no hay nada que jugar':'the hand ends here: nothing to play',
'resuelve primero':'solve first',
'Hero':'Hero', 'DCFR Bot':'DCFR Bot', 'stack':'stack',
'BOT':'BOT', 'TU':'YOU',
'Mano automática':'Auto next hand', 'sin perder EV':'no EV lost',
'fichas':'chips', 'siguiente mano en 2 s':'next hand in 2s',
 'Manos jugadas':'Hands played',
'Al acabar una mano espera un par de segundos, para que te dé tiempo a ver lo que costó, y reparte la siguiente':
  'When a hand ends it waits a couple of seconds, long enough to see what it cost, and deals the next one',
'Pincha una para volver a jugarla igual.':'Click one to play it again exactly.',
'volver a jugarla':'play it again', 'perfecta':'perfect',
'todavía no ha pasado nada':'nothing has happened yet',
'teclas':'keys',
'F retirarse · X pasar · C pagar · 1, 2… apostar o subir · N otra mano · R repetirla · A el consejo':
  'F fold · X check · C call · 1, 2… bet or raise · N next hand · R replay it · A the advice',
'PERFECTA':'PERFECT', 'MUY BIEN':'VERY GOOD', 'BIEN':'GOOD',
'MEJORABLE':'COULD BE BETTER', 'CARA':'EXPENSIVE',
'no dejaste nada de EV en la mesa':'you left nothing on the table',
'de EV':'of EV', '% del bote':'% of the pot',
'no te tocó decidir':'nothing for you to decide',
'-- ninguno --':'-- none --',
'convergida':'converged', 'buena':'good', 'utilizable':'usable',
'floja':'weak', '0.5 a 2%       buena':'0.5 to 2%      good',
'ocultar':'hide', 'ver cuales':'see which',

'mano hecha':'made hand', 'proyecto':'draw', 'todo':'all',
'mirar este punto':'look at this point',
'se aplican en el siguiente solve; cambiar de board los tira':
  'they apply on the next solve; changing the board drops them',
'Quitar todos':'Remove all',
'fuera del rango':'not in the range',
'Esto':'This', 'script':'script', 'cada':'each', 'tiempo':'time',
'solver --script nombre.txt':'solver --script name.txt',
'saves/trees':'saves/trees', '{board}':'{board}', '{n}':'{n}',
'Ah9h4h&#10;Kd7c2s&#10;Ts9s8d':'Ah9h4h&#10;Kd7c2s&#10;Ts9s8d',
'Para en el primero de los tres que llegue.':
  'It stops on whichever of the three comes first.',
'El nombre admite':'The name takes', 'y':'and',
'.':'.',
'Tope de tiempo (s)':'Time limit (s)',
'Para el cálculo al pasar ese tiempo. 0 = sin tope':
  'Stops the solve once that long has passed. 0 = no limit',
// La ventana de resolver muchos boards.
'Resolver muchos boards seguidos':
  'Solving many boards in a row',
'no resuelve aquí':
  'does not solve here',
': escribe un':
  ': it writes a',
', un fichero de texto con la lista de órdenes, y ese fichero lo lanzas desde una terminal y lo dejas corriendo el tiempo que haga falta —toda la noche, si quieres—. El navegador puedes cerrarlo.':
  ', a text file with the list of commands, and you launch that file from a terminal and leave it running as long as it needs -- all night, if you want. You can close the browser.',
'Cómo va, de principio a fin:':
  'How it goes, start to finish:',
'1. Monta aquí el spot que quieres: board de ejemplo, rangos, tamaños, bote y stack. Lo que haya en la pantalla es lo que va a usar':
  '1. Build the spot you want here: an example board, ranges, sizings, pot and stack. Whatever is on screen is what',
'board.':
  'board will use.',
'2. Pon abajo la lista de boards, uno por línea. El botón':
  '2. Put the list of boards below, one per line. The',
'Flops al azar':
  'Random flops',
'te la llena.':
  'button fills it for you.',
'3. Dile cuándo parar en cada board: por':
  '3. Tell it when to stop on each board: by',
'precisión':
  'accuracy',
', por':
  ', by',
', o por los dos —para en el primero que llegue—.':
  ', or by both -- it stops on whichever comes first.',
'. Se guarda tu spot con el nombre que pongas y sale el texto del script.':
  '. Your spot is saved under the name you give, and the script text comes out.',
', y en una terminal, en la carpeta del solver:':
  ', and in a terminal, in the solver folder:',
'Cada board resuelto se guarda como un árbol en':
  'Each solved board is saved as a tree in',
', con el nombre que diga el patrón. Después los abres uno a uno desde':
  ', named by the pattern. Afterwards you open them one by one from',
'Guardar → Árboles guardados':
  'Save → Saved trees',
'y los miras como cualquier otro.':
  'and read them like any other.',
'Cuidado con el sitio en el disco':
  'Mind the room on the disk',
': un árbol de flop ocupa lo que ocupa — lo dice el panel de guardar — y veinte boards son veinte veces eso.':
  ': a flop tree takes what it takes -- the save panel says how much -- and twenty boards are twenty times that.',
'Boards, uno por línea':
  'Boards, one per line',
'Ninguno repetido, y ninguno que sea otro con los palos cambiados de nombre: esos dan la misma solución.':
  'None repeated, and none that is another with the suits renamed: those have the same solution.',
'Tope de tiempo por board (s)':
  'Time limit per board (s)',
'Para en el primero de los tres que llegue. El tope de tiempo es el que hace que la lista quepa en una noche: sin él, un board que no alcanza la precisión se come las horas de los demás.':
  'It stops on whichever of the three comes first. The time limit is what makes the list fit in one night: without it, a board that never reaches the accuracy eats the hours of all the others.',
'Nombre del spot guardado':
  'Name of the saved spot',
'Nombre de cada árbol':
  'Name of each tree',
'guardar el árbol de cada board':
  'save the tree of each board',
'es el board y':
  'is the board and',
'el número de orden. Sin guardar, el script resuelve y no deja nada: sirve para medir cuánto tarda la lista antes de comprometer el disco.':
  'the running number. With nothing saved, the script solves and leaves nothing: useful to measure how long the list takes before committing the disk.',
'Generar':
  'Generate',
'Copiar':
  'Copy',
'Descargar':
  'Download',
'Poner':
  'Add',
'generando...':
  'generating...',
'listo:':
  'done:',
'boards. Descárgalo y lánzalo con solver --script':
  'boards. Download it and run it with solver --script',
'genéralo primero':
  'generate it first',
'copiado':
  'copied',
'descargado como':
  'downloaded as',
'resolver una lista de boards de una tirada, sin estar delante':
  'solve a list of boards in one go, without sitting in front of it',
'Varios boards…':
  'Many boards…',

'ver la explicación':'see the explanation',
'Los combos de la casilla que pinches.':'The combos of the square you click.',
'Cada casilla lleva el color de la acción de arriba.':
  'Each square takes the colour of the action picked above.',
'Pincha o arrastra: pintar ya es cambiarlo.':
  'Click or drag: painting is changing it.',
'Se puede repetir por todo el rango antes de fijar.':
  'You can repeat it across the range before locking it in.',
// Los resumenes de las notas: la linea que se ve sin pasar el raton.
'Sale del':'It comes from the', '75% de la RAM':'75% of the RAM',
'de esta máquina.':'of this machine.',
'El tope de iteraciones es una':'The iteration cap is a',
'red de seguridad':'safety net', ', no el criterio.':', not the criterion.',
'Sin rake por defecto.':'No rake by default.',
'Formato de texto estándar, el de cualquier librería de rangos.':
  'Standard text format, the one every range library uses.',
'los dos':'both', ', OOP e IP, y se cargan los dos.':
  ', OOP and IP, and loading sets both.',
'Más bajo, más fina la solución y más tarda.':
  'Lower is a finer solution and a longer wait.',
': la receta.':': the recipe.', 'Árbol':'Tree', 'Config':'Config',
': la receta y la solución hecha.':': the recipe and the solution already computed.',
'Deja el resto del rango clavado donde está.':
  'Pins the rest of the range where it is.',
'Están resueltos todos: esto no es una muestra.':
  'Every one of them is solved: this is not a sample.',
'Qué tienes':'What you have', ', no qué cartas tienes. Las celdas de acción son':
  ', not which cards you hold. The action cells are',
': arrástralas.':': drag them.', 'barras':'bars',
'Aplicar':'Apply', 'Aplicar las opciones':'Applying the options',
'las opciones se quedan como estaban':'the options stay as they were',
'opciones aplicadas':'options applied',
// Lo que se escribe igual en los dos idiomas. Entra en la tabla a proposito:
// asi "esto no se traduce" es una decision escrita y no un olvido, y la
// comprobacion puede exigir que TODO lo que se lee este aqui.
'DCFR Solver':'DCFR Solver', 'English':'English',
'OOP':'OOP', 'IP':'IP',
'AA,KK,AKs,A8o:0.5':'AA,KK,AKs,A8o:0.5',
'3x':'3x', '2.5x':'2.5x', '2,5x':'2,5x', '2x':'2x',
'Ah9h4h':'Ah9h4h', 'btn-vs-bb-monotono':'bu-vs-bb-monotone',
'respetan.':'whole.',
'solucion anterior. Dale a Resolver.':'previous solution. Press Solve.',
'fijando':'locking', 'cambios descartados':'changes discarded',
'Este árbol no tiene subidas: ninguna calle tiene':
  'This tree has no raises: no street has',
'en las que quieras.':'on whichever streets you want.',
'La media pondera cada carta por su alcance: una que bloquea medio rango no cuenta lo mismo que una que no bloquea nada.':
  'The average weights each card by its reach: one that blocks half a range does not count the same as one that blocks nothing.',
'% rango':'% of range',
'iteraciones':'iterations',
'para quitar una lista. No hay tope de bet+raise: la cadena termina donde la termina el umbral de all-in de arriba. Dos tamaños que se diferencien menos de un 10% se colapsan en uno.':
  'to remove a list. There is no bet+raise cap: the chain ends where the all-in threshold above ends it. Two sizings closer than 10% to each other are merged into one.',
'combo(s) cambiados':' combo(s) changed',
'(sin fijar todavia)':' (not locked in yet)',
'«las fuertes siempre apuestan»':'«the strong hands always bet»',
'nodos ·':' nodes · ',
'juega · bote':' to act  ·  pot ',
'· bote':'  ·  pot ',
'· nodo de azar, bote':'  ·  chance node, pot ',
'iteraciones ·':' iterations  ·  ',
'bote':'pot', 'resolviendo':'solving',
'Combos de la casilla que pinches — el nombre o las barras pintan ese combo suelto, y pinchar otra vez se lo quita;':
  'Combos of the square you click — the name or the bars paint that single combo, and clicking again takes it away;',
'Puedes repetir esto por todo el rango —elegir, mover, elegir otra cosa, mover— y fijarlo todo de una vez al final. Nada llega al solver hasta':
  'You can repeat this across the whole range —pick, move, pick something else, move— and lock it all in at the end. Nothing reaches the solver until',
': arrástrala y mueves la familia entera de golpe -- todas las dobles parejas a apostar, por ejemplo -- sin pintar un combo. El botón ↺ que aparece al lado del nombre la devuelve a lo que hacía la solución.':
  ': drag it and you move the whole family at once -- every two pair to betting, say -- without painting a single combo. The ↺ button next to the name puts it back to what the solution was doing.',
'del bote por mano':'of the pot per hand',
'a lo que hacia la solucion':'to what the solution was doing',
'aún no hay nada que leer en este nodo':'nothing to read at this node yet',
'en':'in',
};

// El idioma elegido se queda en el navegador de cada uno: el programa es el
// mismo para todos y esto es una preferencia de quien mira.
//
// De fabrica, ingles. La pagina esta escrita en español -- es el idioma de quien
// la mantiene, y las claves de la tabla son el propio texto español -- pero se
// abre traducida: esto se publica, y la primera pantalla tiene que entenderla
// cualquiera. El español esta en el engranaje y no se vuelve a preguntar.
let idioma = 'en';
try { idioma = localStorage.getItem('solverLang') || 'en'; } catch(e) {}

// Para textos que se arman en el codigo con numeros dentro.
function t(x){
  if(idioma!=='en') return x;
  const en = EN[x.replace(/\s+/g,' ').trim()];
  if(en === undefined) return x;
  return (/^\s/.test(x)?' ':'') + en + (/\s$/.test(x)?' ':'');
}

// El español original de cada trozo de texto, para poder volver.
const ORIG = new WeakMap();

function traduceNodo(n){
  const orig = ORIG.has(n) ? ORIG.get(n) : n.nodeValue;
  const k = orig.replace(/\s+/g,' ').trim();
  if(!k) return;
  const en = EN[k];
  if(en === undefined) return;
  if(!ORIG.has(n)) ORIG.set(n, orig);
  if(idioma !== 'en'){ n.nodeValue = orig; return; }
  // Los espacios de los lados se respetan: muchos de estos trozos van pegados
  // a un <code> o a un <b> y sin ellos las palabras se juntan.
  n.nodeValue = (/^\s/.test(orig) ? ' ' : '') + en + (/\s$/.test(orig) ? ' ' : '');
}

function traduceAttr(el, attr, guarda){
  if(!el.hasAttribute(attr) && !el.dataset[guarda]) return;
  const orig = el.dataset[guarda] !== undefined ? el.dataset[guarda] : el.getAttribute(attr);
  const k = (orig||'').replace(/\s+/g,' ').trim();
  if(!k) return;
  const en = EN[k];
  if(en === undefined) return;
  if(el.dataset[guarda] === undefined) el.dataset[guarda] = orig;
  el.setAttribute(attr, idioma === 'en' ? en : orig);
}

function traduce(raiz){
  if(!raiz) return;
  if(raiz.nodeType === 3){ traduceNodo(raiz); return; }
  if(raiz.nodeType !== 1 && raiz.nodeType !== 9 && raiz.nodeType !== 11) return;
  const it = document.createTreeWalker(raiz, NodeFilter.SHOW_TEXT);
  const nodos = [];
  while(it.nextNode()) nodos.push(it.currentNode);
  nodos.forEach(traduceNodo);
  const els = raiz.querySelectorAll ? raiz.querySelectorAll('[title],[placeholder]') : [];
  els.forEach(el=>{ traduceAttr(el,'title','esTitle'); traduceAttr(el,'placeholder','esPh'); });
  if(raiz.nodeType === 1){
    traduceAttr(raiz,'title','esTitle');
    traduceAttr(raiz,'placeholder','esPh');
  }
}

function ponIdioma(l){
  idioma = (l === 'en') ? 'en' : 'es';
  try { localStorage.setItem('solverLang', idioma); } catch(e) {}
  document.documentElement.lang = idioma;
  document.getElementById('langEs').className = 'sm' + (idioma==='es'?' act':'');
  document.getElementById('langEn').className = 'sm' + (idioma==='en'?' act':'');
  traduce(document.body);
  // Lo que se arma en el codigo con t() hay que volver a armarlo: una vez
  // escrito en la pantalla ya es un texto suelto con numeros dentro, y eso
  // no hay observador que lo traduzca.
  if(state){
    applyState(false);
    renderStreets(); renderHud(); renderStrategy(); renderMade(); renderNav();
    renderDeck();
  }
}

function toggleOpts(){
  const p = document.getElementById('optsPanel');
  const abierto = p.style.display === 'none';
  p.style.display = abierto ? '' : 'none';
  document.getElementById('gearBtn').className = 'gear' + (abierto ? ' on' : '');
  // Los campos los rellena applyState como los de cualquier otro panel: aqui
  // solo la version, que no es un campo.
  if(abierto){
    document.getElementById('optVer').textContent = state ? 'v'+state.version : '-';
    document.getElementById('optNote').textContent = '';
  }
}
// Pinchar fuera lo cierra: es un panel, no una pestaña.
document.addEventListener('mousedown', e=>{
  const p = document.getElementById('optsPanel');
  if(!p || p.style.display === 'none') return;
  if(e.target.closest('.gearwrap')) return;
  p.style.display = 'none';
  document.getElementById('gearBtn').className = 'gear';
});

// Todo lo que aparezca de nuevo en la pantalla pasa por aquí. Es la única forma
// de que una tabla que se repinta cada segundo salga traducida sin llenar el
// código de llamadas.
new MutationObserver(ms=>{
  if(idioma !== 'en') return;
  ms.forEach(m=>{ m.addedNodes.forEach(traduce); });
}).observe(document.body, {childList:true, subtree:true});

function arrancaIdioma(){ ponIdioma(idioma); }

function num(x, dec){
  if(typeof x!=='number' || !isFinite(x)) return '-';
  const s=(dec>0 ? x.toFixed(dec) : Math.round(x).toString());
  const p=s.split('.');
  return p[0].replace(/\B(?=(\d{3})+(?!\d))/g,' ')+(p[1]?'.'+p[1]:'');
}
function clsName(i,j){
  if(i===j) return RANKS[i]+RANKS[i];
  if(i<j)   return RANKS[i]+RANKS[j]+'s';
  return RANKS[j]+RANKS[i]+'o';
}
function status(txt,cls){
  const p=document.getElementById('statusPill');
  p.textContent=txt; p.className='pill'+(cls?' '+cls:'');
}
function getCSS(v){return getComputedStyle(document.documentElement).getPropertyValue(v).trim();}
function actionColor(actions,i){
  const k=actions[i].kind;
  if(k===0) return getCSS('--fold');
  if(k===1||k===2) return getCSS('--check');
  let n=0; for(let x=0;x<i;x++) if(actions[x].kind>=3) n++;
  return getCSS(['--bet1','--bet2','--bet3'][Math.min(n,2)]);
}
// Never throws. A rejected promise here becomes an unhandled rejection inside
// whichever handler happened to call it, which on this page means a status that
// says "reading node..." for ever and nothing else to go on. The solver is a
// local process the user can close, so losing it is an ordinary event, and an
// ordinary event should arrive as a value.
// El idioma va en CADA peticion. Los errores los escribe el motor, y el motor
// no tiene forma de saber en que idioma esta mirando esto quien lo mira: se lo
// dice la pagina, que es la unica que lo sabe.
async function api(path,form){
  const f2 = form ? Object.assign({}, form, {lang:idioma}) : null;
  const opt=f2?{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
                  body:new URLSearchParams(f2).toString()}:{};
  if(!f2) path += (path.indexOf('?')>=0?'&':'?')+'lang='+idioma;
  try{
    const res=await fetch(path,opt);
    if(!res.ok) return {ok:false,offline:true,
                        note:t('el solver respondió ')+res.status+' a '+path};
    return await res.json();
  }catch(e){
    return {ok:false,offline:true,
            note:t('sin respuesta del solver — ¿sigue abierto en esta máquina?')};
  }
}
// One banner for "the process is not there", because every panel going quiet
// with no explanation is the worst way to say it.
let offlineNow=false;
function showOffline(r){
  offlineNow=true;
  const el=document.getElementById('treeNote');
  if(el) el.innerHTML='<span class="err">'+((r&&r.note)||t('sin respuesta del solver'))+
                      '</span>';
  status(t('sin conexión con el solver'),'err');
}
function ctxOf(i){return state.tree[i];}
// Every mutating call answers with the whole state, EXCEPT when the server
// refuses it -- a solve is running, say, and then there is no state in the
// reply at all. Assigning that anyway left `state` undefined, the next render
// threw on it, and the page stayed dead until a reload: no strategy, no grid,
// no HUD, nothing, with a two-word note as the only clue. Refusals are normal,
// so taking a state has to be the thing that checks.
function takeState(r, resetNav){
  if(r && r.state){ state=r.state; applyState(!!resetNav); return true; }
  return false;
}
function nodeOf(){return state.tree[nav.ctx].nodes[nav.node];}
// Where the user just clicked, named from the tree the client already holds --
// so the header can be right while the numbers are still on their way.
function navWhere(){
  const c=state.tree[nav.ctx], n=c.nodes[nav.node];
  return c.label+' '+n.path+'  ('+(n.player===0?'OOP':'IP')+' juega)';
}

// ---------------------------------------------------------------- state
async function refresh(){
  const s=await api('/api/state');
  // /api/state answers with the state itself, not wrapped, so "did it work" is
  // "does it look like a state".
  if(s && s.tree){
    state=s; applyState(true);
    // Coming back is as much news as going away.
    if(offlineNow){ offlineNow=false; status(state.solved?'lista':'parado',state.solved?'on':''); }
    return true;
  }
  showOffline(s);
  return false;
}

function applyState(resetNav){
  document.getElementById('board').value=state.board;
  document.getElementById('pot').value=state.pot;
  document.getElementById('stack').value=state.stack;
  document.getElementById('allinPct').value=(state.allinPct*100).toFixed(2).replace(/\.?0+$/,'');
  document.getElementById('rakePct').value=(state.rakePct*100).toFixed(2).replace(/\.?0+$/,'');
  document.getElementById('rakeCap').value=state.rakeCap;
  document.getElementById('maxMem').value=state.maxMem;
  document.getElementById('iters').value=state.iters;
  document.getElementById('timeout').value=num(state.timeout,0);
  document.getElementById('accPct').value=state.accPct;
  document.getElementById('accStop').checked=!!state.accStop;
  document.getElementById('threads').value=state.threadsCfg;
  document.getElementById('thNote').textContent=t('(0 = los ')+state.threadsMax+')';
  ranges[0]=state.oopRange.slice();
  ranges[1]=state.ipRange.slice();
  document.getElementById('comboNote').textContent=
    state.combos+t(' combos en este board, ')+state.runouts+' runout'+
    (state.runouts===1?'':'s')+t(' por salir');
  document.getElementById('streetPill').textContent=t('desde el ')+state.streetName;
  // La version, pequena y al lado del nombre. Cuando alguien cuenta un problema
  // en el Discord, lo primero que hace falta saber es que build tiene.
  const vr=document.getElementById('ver');
  if(vr && state.version){ vr.textContent='v'+state.version; }
  if(state.version) document.title='DCFR Solver v'+state.version;
  document.getElementById('sizePill').textContent=
    num(state.instNodes,0)+t(' nodos · ')+num(state.memGB,2)+' GB'+
    (state.iso?'  · '+state.isoGroup+t(' runouts en uno'):'');
  document.getElementById('sizePill').title=state.iso
    ? (t('Dos runouts que solo se diferencian en el palo dan la misma ')+
       t('estrategia, así que se resuelven una sola vez: de ahí que el solve ')+
       'cueste '+state.isoGroup+t(' veces menos.')+
       (state.isoGroup<state.isoBoardGroup
         ? t(' Este board da para juntar ')+state.isoBoardGroup+t(', y con tus ')+
           t('rangos y locks aguantan ')+state.isoGroup+t('. Si el lock lo haces ')+
           t('por rangos, vuelve el resto.')
         : t(' Es todo lo que este board da de sí, y tus rangos y locks lo ')+
           t('respetan.')))
    : (state.isoOffByLock
       ? t('Cada runout se resuelve por su cuenta: hay un lock puesto sobre ')+
         t('cartas concretas y eso rompe la simetría de palos del board. Está ')+
         t('bien hecho, pero cuesta varias veces más. Si el lock lo haces por ')+
         t('rangos o por buckets, la simetría vuelve.')
       : t('Cada runout se resuelve por su cuenta: o el board no tiene dos ')+
         t('palos intercambiables (arcoíris), o uno de los rangos no es ')+
         t('simétrico en palos.'));
  if(resetNav || !lineValid()){ lineInit(); curCls=-1; }
  else syncNav();
  boardSel=state.boardCards.slice();
  renderBoard(); renderDeck(); renderStreets(); renderRangeGrid();
  renderNav(); renderLocks();
  renderConv(state.solved ? state.explPct : -1, false);
  // El aviso va en el boton que hay que pulsar, que es donde se mira. Y si el
  // servidor dice que ya no queda nada pendiente, lo que guardabamos aqui sobra.
  if(!state.locksPending) pendVacia();
  const gb=document.getElementById('goBtn');
  if(gb){
    const n=(state.locks||[]).length;
    gb.textContent = state.locksPending
      ? 'Resolver · ' + n + (n===1?t(' lock sin aplicar'):t(' locks sin aplicar'))
      : 'Resolver';
    gb.classList.toggle('pend', !!state.locksPending);
  }
  const vp=document.getElementById('valuePill');
  if(state.solved){
    vp.style.display=''; vp.className='pill on';
    vp.textContent='OOP '+state.evOOP.toFixed(3)+' / IP '+state.evIP.toFixed(3)+
                   '  ('+state.done+' iters, '+state.threads+' threads)';
    loadNode();
  } else { vp.style.display='none'; node=null; renderStrategy(); renderHud(); }
  // Without both ranges there is nothing to solve, and saying so beats an
  // enabled button that fails.
  const go=document.getElementById('goBtn'), st=document.getElementById('saveTreeBtn');
  go.disabled=!state.ready;
  go.title=state.ready?'':state.notReady;
  espejaGo();
  const mb=document.getElementById('moreBtn');
  mb.disabled=solvingLive||!state.solved;
  mb.title=state.solved?t('sigue desde donde esta, sin tirar lo hecho')
                       :t('primero hay que solvear algo');
  st.disabled=!state.solved;
  st.title=state.solved?'':t('primero hay que solvear algo');
  if(!state.ready)
    document.getElementById('treeNote').innerHTML='<span class="err">'+state.notReady+'</span>';
}
// ---------------------------------------------------------------- deck
const SUITS=['s','h','d','c'];
const SUIT_GLYPH={s:'♠',h:'♥',d:'♦',c:'♣'};
const DECK_RANKS=['A','K','Q','J','T','9','8','7','6','5','4','3','2'];
let boardSel=[];

// A card face, four-colour. `extra` adds classes, `pos` shows its board slot.
function cardFace(card,base,extra,pos){
  const d=document.createElement('div');
  d.className=base+' '+card[1]+(extra?' '+extra:'');
  d.innerHTML='<span>'+card[0]+'</span><span class="su">'+SUIT_GLYPH[card[1]]+'</span>'+
              (pos?'<span class="pos">'+pos+'</span>':'');
  return d;
}

function renderBoard(){
  const el=document.getElementById('boardView'); el.innerHTML='';
  state.boardCards.forEach(c=>el.appendChild(cardFace(c,'bcard sm')));
}

function renderDeck(){
  const el=document.getElementById('deck'); el.innerHTML='';
  for(const su of SUITS){
    const row=document.createElement('div'); row.className='deckrow';
    for(const r of DECK_RANKS){
      const card=r+su;
      const i=boardSel.indexOf(card);
      const full=boardSel.length>=3 && i<0;
      const d=cardFace(card,'pc',(i>=0?'sel':'')+(full?' dead':''),
                       i>=0?String(i+1):'');
      d.title=full?t('el flop ya está puesto — quita una carta primero')
                  :(i>=0?t('quitar del flop'):t('poner en el flop'));
      if(!full) d.onclick=()=>toggleCard(card);
      row.appendChild(d);
    }
    el.appendChild(row);
  }
  renderSlots();
}

// The flop is chosen here; the turn and river are whatever the line you are
// walking has dealt, so those slots mirror the tree rather than taking input.
function dealtCards(){
  if(!line.length || !state) return [];
  const out=[];
  for(let k=1;k<=sel && k<line.length;k++)
    if(line[k].slot!==undefined) out.push(state.deck[line[k].slot]);
  return out;
}

function renderSlots(){
  const el=document.getElementById('bslots');
  if(!el) return;
  el.innerHTML='';
  const dealt=dealtCards();
  // Anything past the flop that is already part of the board (a 4- or 5-card
  // board typed by hand) counts as dealt too.
  const runout=boardSel.slice(3).concat(dealt);
  const groups=[['flop',0,3],['turn',3,4],['river',4,5]];
  for(const [name,a,b] of groups){
    const g=document.createElement('div'); g.className='bgroup';
    const l=document.createElement('span'); l.className='lab'; l.textContent=name;
    g.appendChild(l);
    for(let i=a;i<b;i++){
      const isFlop=i<3;
      const card=isFlop?boardSel[i]:runout[i-3];
      if(card){
        const c=cardFace(card,'bcard',isFlop?'':'dealt');
        if(isFlop){ c.title='quitar'; c.onclick=()=>toggleCard(card); }
        else c.title=t('la reparte la línea en la que estás');
        g.appendChild(c);
      } else {
        const e=document.createElement('div'); e.className='bcard empty'; g.appendChild(e);
      }
    }
    el.appendChild(g);
  }
}

function toggleDeck(){
  const d=document.getElementById('deck');
  const open=d.style.display==='none';
  d.style.display=open?'':'none';
  document.getElementById('deckToggle').textContent=open?'Listo':t('Elegir flop');
}

function toggleCard(card){
  const i=boardSel.indexOf(card);
  if(i>=0) boardSel.splice(i,1);
  else if(boardSel.length<3) boardSel.push(card);   // the picker sets the flop only
  else return;
  renderDeck();
  document.getElementById('board').value=boardSel.join('');
  if(boardSel.length>=3) pushBoard();
  else document.getElementById('comboNote').textContent=
    'faltan '+(3-boardSel.length)+' carta'+(boardSel.length===2?'':'s')+
    t(' para el flop');
}

// ---------------------------------------------------------------- streets
// One box per player per street, the way they are usually laid out: the two of them do
// not bet the same sizes, so they do not share a field. IP carries "don't
// 3-bet"; OOP carries the donk sizes, and only where a donk can exist.
function streetBox(s,i,who){
  const ip=(who==='ip'), p=ip?'ip':'oop';
  // En la calle donde empieza el solve no hay donk, y antes eso era un hueco
  // en blanco: se ve que al turn y al river OOP tiene su campo y al flop no,
  // y lo que uno piensa es que ahi OOP no puede liderar. Puede -- la apertura
  // de OOP SON sus tamaños de bet --, lo que no hay es un donk, porque un donk
  // es liderar contra quien fue agresivo en la calle anterior y en la calle de
  // salida no hubo agresor. Igual que en la referencia. Asi que se dice.
  const donkRow = ip ? '' : (s.donkable
    ? '<div class="row"><div><label>Tamaños de donk</label>'+
      '<input type="text" id="d'+i+'" title="OOP liderando contra quien fue agresivo en la calle anterior. Vacio (none) = no lidera"></div>'+
      '<div></div></div>'
    : '');
  const gag = ip
    ? '<label class="tick" title="IP no hace la tercera acción agresiva de la calle: apuesta IP, sube OOP, IP ya no resube">'+
      '<input type="checkbox" id="n'+i+'">Sin 3-bet</label>'
    : '<div></div>';
  const d=document.createElement('div');
  d.className='street';
  d.innerHTML='<h3>'+s.name+' '+(ip?'IP':'OOP')+'</h3>'+
    '<div class="row">'+
      '<div><label>Tamaños de bet (x bote)</label><input type="text" id="'+p+'b'+i+'"></div>'+
      '<div><label>Tamaños de raise</label><input type="text" id="'+p+'r'+i+'" title="3x = subir hasta 3 veces el bet · 0.5 = medio bote encima"></div>'+
    '</div>'+
    donkRow+
    '<div class="row">'+
      '<label class="tick" title="Anade una rama de all-in a pelo, ademas de los tamaños">'+
        '<input type="checkbox" id="'+p+'a'+i+'">Añadir allin</label>'+
      gag+
    '</div>';
  return d;
}

function renderStreets(){
  const el=document.getElementById('streets'); el.innerHTML='';
  const live=state.streets.map((s,i)=>({s:s,i:i})).filter(x=>x.s.active);

  const hIP=document.createElement('div'); hIP.className='side'; hIP.textContent='IP';
  el.appendChild(hIP);
  const rIP=document.createElement('div'); rIP.className='streetrow';
  live.forEach(x=>rIP.appendChild(streetBox(x.s,x.i,'ip')));
  el.appendChild(rIP);

  const btn=document.createElement('button');
  btn.textContent=t('Copiar de IP a OOP');
  btn.style.width='100%'; btn.style.marginBottom='6px';
  btn.onclick=()=>{
    live.forEach(x=>{
      document.getElementById('oopb'+x.i).value=document.getElementById('ipb'+x.i).value;
      document.getElementById('oopr'+x.i).value=document.getElementById('ipr'+x.i).value;
      document.getElementById('oopa'+x.i).checked=document.getElementById('ipa'+x.i).checked;
    });
    // Copying fills the boxes, it does not build: nothing is sent until Apply,
    // which is the same rule as every other field on this panel.
    status(t('copiado en OOP: dale a Aplicar'),'');
  };
  el.appendChild(btn);

  const hOOP=document.createElement('div'); hOOP.className='side'; hOOP.textContent='OOP';
  el.appendChild(hOOP);
  const rOOP=document.createElement('div'); rOOP.className='streetrow';
  live.forEach(x=>rOOP.appendChild(streetBox(x.s,x.i,'oop')));
  el.appendChild(rOOP);

  const mg=document.createElement('div');
  mg.className='note';
  mg.innerHTML='<button class="sm" style="float:right;margin-left:8px" '+
    'onclick="openSizeHelp()">? cómo se escriben</button>'+
    'raise: <b>3x</b> = 3 veces el bet · <b>0.5</b> = medio bote encima<br>'+
    'Sin tamaños no hay acción: escribe <b>none</b> para quitar una lista. '+
    'No hay tope de bet+raise: la cadena termina donde la termina el umbral de '+
    'all-in de arriba. Dos tamaños que se diferencien menos de un 10% se '+
    'colapsan en uno.';
  el.appendChild(mg);

  live.forEach(x=>{
    const s=x.s, i=x.i;
    document.getElementById('oopb'+i).value=s.oopBets;
    document.getElementById('oopr'+i).value=s.oopRaises;
    document.getElementById('oopa'+i).checked=!!s.oopAllin;
    document.getElementById('ipb'+i).value=s.ipBets;
    document.getElementById('ipr'+i).value=s.ipRaises;
    document.getElementById('ipa'+i).checked=!!s.ipAllin;
    document.getElementById('n'+i).checked=!!s.no3bet;
    if(s.donkable) document.getElementById('d'+i).value=s.donks;
  });
}

// ---------------------------------------------------------------- ranges
function setTab(t){
  tab=t;
  document.getElementById('tabOOP').className=t===0?'act':'';
  document.getElementById('tabIP').className=t===1?'act':'';
  renderRangeGrid();
}
function renderRangeGrid(){
  const g=document.getElementById('rangeGrid'); g.innerHTML='';
  for(let i=0;i<13;i++)for(let j=0;j<13;j++){
    const idx=i*13+j;
    const c=document.createElement('div');
    c.className='cell';
    c.innerHTML='<div class="fill"></div><div class="lab">'+clsName(i,j)+'</div>';
    c.onmousedown=e=>{e.preventDefault();painting=true;
      paintVal=(ranges[tab][idx]>0)?0:(+document.getElementById('weight').value/100);
      paintCell(idx);};
    c.onmouseenter=()=>{ if(painting) paintCell(idx); };
    g.appendChild(c);
  }
  paintGrid();
  document.getElementById('rangeSpec').value=tab===0?state.oopSpec:state.ipSpec;
  document.getElementById('rangeNote').textContent=
    (tab===0?state.oopCombos:state.ipCombos)+t(' combos vivos en este board');
}
function paintGrid(){
  const cells=document.getElementById('rangeGrid').children;
  for(let k=0;k<169;k++){
    const w=ranges[tab][k]||0;
    cells[k].className='cell'+(w>0?' on':'');
    cells[k].querySelector('.fill').style.background=
      w>0?'rgba(76,139,245,'+(0.25+0.75*Math.min(w,1))+')':'transparent';
    // El peso, al pasar el raton por encima. El color dice que la mano esta
    // dentro y mas o menos cuanto, pero '¿esto es un 60 o un 75?' no se ve en
    // un tono de azul, y es justo lo que hay que saber para retocar un rango.
    cells[k].title=clsName(Math.floor(k/13),k%13)+
      (w>0 ? '  '+num(100*Math.min(w,1),w>=0.995?0:1)+'%'
           : '  '+t('fuera del rango'));
  }
}
function paintCell(idx){ ranges[tab][idx]=paintVal; paintGrid(); }
document.addEventListener('mouseup',()=>{ if(painting){painting=false; pushRange();} });
// El texto que sale de la cuadricula, en el formato de la referencia: las manos
// puras van sin peso y solo las parciales llevan ':0.5'. Es el formato que
// circula por foros, videos y librerias, asi que un rango pintado aqui se pega
// en cualquier solver y uno copiado de ellos se pega aqui.
function wtxt(w){
  if(w>=0.9995) return '';
  let s=w.toFixed(3);
  while(s.charAt(s.length-1)==='0') s=s.slice(0,-1);
  if(s.charAt(s.length-1)==='.') s=s.slice(0,-1);
  return ':'+s;
}
function rangeToSpec(t){
  const out=[];
  for(let i=0;i<13;i++)for(let j=0;j<13;j++){
    const w=ranges[t][i*13+j];
    if(w>0) out.push(clsName(i,j)+wtxt(w));
  }
  return out.join(',');
}
function clearRange(){
  ranges[tab]=new Array(169).fill(0);
  paintGrid(); pushRange();
}
async function pushRange(){
  const spec=rangeToSpec(tab);
  if(!spec){ document.getElementById('rangeNote').innerHTML='<span class="err">el rango está vacío</span>'; return; }
  if(!tirariaElSolve(t('Cambiar el rango'))){ status(t('el rango se queda como estaba'),''); return; }
  status(t('aplicando el rango...'),'busy');
  const r=await api('/api/config',tab===0?{oop:spec}:{ip:spec});
  takeState(r,false);
  status(r.ok?'range set':'error',r.ok?'':'err');
  if(!r.ok) document.getElementById('rangeNote').innerHTML='<span class="err">'+r.note+'</span>';
}
async function applyRangeText(){
  if(!tirariaElSolve(t('Cambiar el rango'))){ status(t('el rango se queda como estaba'),''); return; }
  status(t('aplicando el rango...'),'busy');
  const spec=document.getElementById('rangeSpec').value;
  const r=await api('/api/config',tab===0?{oop:spec}:{ip:spec});
  takeState(r,false);
  status(r.ok?'range set':'error',r.ok?'':'err');
  if(!r.ok) document.getElementById('rangeNote').innerHTML='<span class="err">'+r.note+'</span>';
}

// ---------------------------------------------------------------- config
async function pushBoard(){
  if(!tirariaElSolve(t('Cambiar el board'))){ status(t('el board se queda como estaba'),''); return; }
  status('reconstruyendo...','busy');
  const r=await api('/api/config',{board:document.getElementById('board').value});
  takeState(r,true);
  status(r.ok?'board puesto':'error',r.ok?'':'err');
  if(!r.ok) document.getElementById('comboNote').innerHTML='<span class="err">'+r.note+'</span>';
}
async function applyBoard(){ await pushBoard(); }
// Reconstruir el arbol TIRA la solucion, y hasta ahora lo hacia callando.
//
// Pasa de verdad: resolver un arbol de 2,7 GB, darle a Construir para mirar otra
// cosa, y quedarse con 7 iteraciones y una explotabilidad del 211% del bote --
// con todos los paneles ensenando tablas que parecen una solucion. Y no se puede
// deshacer: los arrepentimientos acumulados no se guardan en ningun sitio.
//
// Asi que se pregunta, y se dice lo que se pierde en numeros, no en abstracto.
// Solo cuando hay algo que perder: sin iteraciones hechas no molesta a nadie.
function tirariaElSolve(que){
  if(!state || !(state.done>0)) return true;
  const ex=(typeof state.explPct==='number' && state.explPct>=0)
    ? t(' y una explotabilidad del ')+state.explPct.toFixed(3)+t('% del bote') : '';
  return confirm(que+t(' tira la solución que tienes: ')+
    num(state.done,0)+' iteraciones'+ex+'. '+
    t('Esto no se puede deshacer. Si la quieres conservar, cancela y guarda el ')+
    t('árbol primero. ¿Seguir?'));
}

// Aplicar lo del engranaje sin pasar por Montar arbol.
//
// El limite de memoria, los hilos y el tope de iteraciones no tocan el arbol:
// se ponen y ya esta. El umbral de all-in y el rake SI lo tocan, asi que solo
// si de verdad han cambiado se avisa de que eso tira la solucion.
async function aplicaOpciones(){
  const v = id => document.getElementById(id).value;
  const ap = parseFloat(v('allinPct')||'67')/100;
  const rp = parseFloat(v('rakePct')||'0')/100;
  const rc = parseFloat(v('rakeCap')||'0');
  const cambiaArbol = !state ||
      Math.abs(ap - state.allinPct) > 1e-9 ||
      Math.abs(rp - state.rakePct)  > 1e-9 ||
      Math.abs(rc - state.rakeCap)  > 1e-9;
  if(cambiaArbol && !tirariaElSolve(t('Aplicar las opciones'))){
    status(t('las opciones se quedan como estaban'),'');
    return;
  }
  const f={ maxmem:v('maxMem'), threads:v('threads'),
            iters:v('iters'), timeout:v('timeout'),
            allinPct:ap, rakePct:rp, rakeCap:rc };
  const r=await api('/api/config',f);
  takeState(r,false);
  document.getElementById('optNote').textContent = r.ok ? (r.note||'') : (r.note||'');
  status(r.ok?t('opciones aplicadas'):'error', r.ok?'':'err');
}

// ---- script de varios boards ----------------------------------------------
// Lo escribe el servidor, no esto: el script lleva el spot dentro y el spot lo
// tiene el servidor entero. Aqui solo se recogen las opciones.
function abreScript(){
  document.getElementById('scrAcc').value=document.getElementById('accPct').value;
  document.getElementById('scrIters').value=document.getElementById('iters').value;
  document.getElementById('scrNote').textContent='';
  document.getElementById('scrDlg').classList.add('on');
}
function cierraScript(){ document.getElementById('scrDlg').classList.remove('on'); }

async function scriptFlops(){
  const n=document.getElementById('scrN').value;
  const r=await api('/api/flops?n='+encodeURIComponent(n));
  if(!r.ok||!r.flops){ document.getElementById('scrNote').textContent=t('no se pudo'); return; }
  const ta=document.getElementById('scrBoards');
  const hay=ta.value.trim();
  ta.value=(hay?hay+'\n':'')+r.flops.join('\n');
}

async function generaScript(){
  const f={boards:document.getElementById('scrBoards').value,
           acc:document.getElementById('scrAcc').value,
           timeout:document.getElementById('scrTmo').value,
           iters:document.getElementById('scrIters').value,
           name:document.getElementById('scrName').value,
           pattern:document.getElementById('scrPat').value,
           save:document.getElementById('scrSave').checked?'1':'0'};
  document.getElementById('scrNote').textContent=t('generando...');
  const r=await api('/api/script',f);
  if(!r.ok){
    document.getElementById('scrOut').value='';
    document.getElementById('scrNote').innerHTML='<span class="err">'+(r.note||t('no se pudo'))+'</span>';
    return;
  }
  document.getElementById('scrOut').value=r.script;
  const n=(r.script.match(/^board /gm)||[]).length;
  document.getElementById('scrNote').textContent=
    t('listo: ')+n+t(' boards. Descárgalo y lánzalo con solver --script');
  refreshSaves();
}
function copiaScript(){
  const ta=document.getElementById('scrOut');
  if(!ta.value){ document.getElementById('scrNote').textContent=t('genéralo primero'); return; }
  ta.select();
  try{ document.execCommand('copy'); }catch(e){}
  document.getElementById('scrNote').textContent=t('copiado');
}
// Descargar sin servidor: el fichero se arma aqui mismo. Es texto y pesa nada.
function bajaScript(){
  const txt=document.getElementById('scrOut').value;
  if(!txt){ document.getElementById('scrNote').textContent=t('genéralo primero'); return; }
  const nom=(document.getElementById('scrName').value||'script')+'.txt';
  const a=document.createElement('a');
  a.href=URL.createObjectURL(new Blob([txt],{type:'text/plain'}));
  a.download=nom;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(a.href);
  document.getElementById('scrNote').textContent=t('descargado como ')+nom;
}

async function applyTree(){
  if(!tirariaElSolve(t('Construir el árbol'))){ status(t('el árbol se queda como estaba'),''); return; }
  status(t('construyendo el árbol...'),'busy');
  const f={pot:document.getElementById('pot').value,
           stack:document.getElementById('stack').value,
           allinPct:(parseFloat(document.getElementById('allinPct').value||'67')/100),
           rakePct:(parseFloat(document.getElementById('rakePct').value||'0')/100),
           rakeCap:document.getElementById('rakeCap').value,
           maxmem:document.getElementById('maxMem').value,
           iters:document.getElementById('iters').value,
           accPct:document.getElementById('accPct').value,
           timeout:document.getElementById('timeout').value,
           accStop:document.getElementById('accStop').checked?'1':'0',
           threads:document.getElementById('threads').value};
  state.streets.forEach((s,i)=>{
    if(!s.active) return;
    f[s.name+'OopBets']=document.getElementById('oopb'+i).value;
    f[s.name+'OopRaises']=document.getElementById('oopr'+i).value;
    f[s.name+'OopAllin']=document.getElementById('oopa'+i).checked?'1':'0';
    f[s.name+'IpBets']=document.getElementById('ipb'+i).value;
    f[s.name+'IpRaises']=document.getElementById('ipr'+i).value;
    f[s.name+'IpAllin']=document.getElementById('ipa'+i).checked?'1':'0';
    f[s.name+'No3bet']=document.getElementById('n'+i).checked?'1':'0';
    // Nothing is sent for the street the solve starts on: there is nobody to
    // lead into there, and the server refuses it rather than pretending.
    if(s.donkable) f[s.name+'Donks']=document.getElementById('d'+i).value;
  });
  const r=await api('/api/config',f);
  takeState(r,true);
  status(r.ok?t('árbol construido'):'error',r.ok?'':'err');
  document.getElementById('treeNote').innerHTML=
    r.ok?(num(state.instNodes,0)+t(' nodos del árbol, ')+num(state.memGB,3)+' GB'+
          (state.noRaises
            ? '<br><span style="color:var(--warn,#e0b33a)">Este árbol no tiene subidas: '+
              'ninguna calle tiene <b>raise sizes</b>. Escribe por ejemplo <b>3x</b> en las '+
              'que quieras.</span>'
            : ''))
        :('<span class="err">'+r.note+'</span>');
}
// ---------------------------------------------------------------- guardar
function fmtBytes(b){
  if(b<0) return '?';
  if(b<1024) return b+' B';
  if(b<1048576) return (b/1024).toFixed(1)+' KB';
  if(b<1073741824) return (b/1048576).toFixed(1)+' MB';
  return (b/1073741824).toFixed(2)+' GB';
}
let saves={configs:[],trees:[]};
async function refreshSaves(){
  const r=await api('/api/saves',{});
  if(!r.ok) return;
  saves=r;
  [['cfgList',r.configs],['treeList',r.trees],['rangePick',r.ranges||[]]].forEach(([id,list])=>{
    const el=document.getElementById(id), keep=el.value;
    el.innerHTML='<option value="">'+t('-- ninguno --')+'</option>'+
      list.map(x=>'<option value="'+x.name+'">'+x.name+'  ('+fmtBytes(x.bytes)+')</option>').join('');
    if(list.some(x=>x.name===keep)) el.value=keep;
  });
  // Lo que ocupa todo esto, y lo que queda. Un arbol resuelto son gigas y se
  // guardan sin mirar: doce ficheros llegaron a 7,2 GB con el disco al 93% y
  // nada en pantalla lo decia. El tamano del proximo sale de la cuenta exacta
  // de lo que se va a escribir, no de una estimacion.
  const dn=document.getElementById('diskNote');
  if(dn){
    const libre=(typeof r.freeBytes==='number')?r.freeBytes:-1;
    const apretado=libre>=0 && libre<10*1073741824;
    dn.innerHTML=t('Lo guardado ocupa')+' <b>'+fmtBytes(r.savesBytes||0)+'</b>'+
      (libre>=0?' · <b>'+fmtBytes(libre)+'</b> '+t('libres en el disco'):'')+
      (r.nextTreeBytes>0?t(' · guardar este árbol pide ')+fmtBytes(r.nextTreeBytes):'');
    dn.style.color=apretado?'var(--warn,#e0b33a)':'';
  }
}
// Picking from a list fills the name box, so load/delete and a later save all
// act on the thing you just clicked without retyping it.
function pickSave(k){
  const v=document.getElementById(k?'treeList':'cfgList').value;
  if(v) document.getElementById('saveName').value=v;
}
async function store(verb,kind){
  const sel=document.getElementById(kind==='tree'?'treeList':'cfgList').value;
  const name=(verb==='save')?document.getElementById('saveName').value.trim()
                            :(sel||document.getElementById('saveName').value.trim());
  const note=document.getElementById('saveNote');
  if(!name){ note.innerHTML='<span class="err">pon un nombre</span>'; return; }
  if(verb==='delete'&&!confirm('Borrar '+kind+' "'+name+'"?')) return;
  const heavy=(kind==='tree'&&verb!=='delete');
  note.textContent=heavy?(verb==='save'?t('escribiendo el árbol, puede tardar...'):t('cargando el árbol...'))
                        :'...';
  status(heavy?t('trabajando con el disco...'):'...','busy');
  const r=await api('/api/store',{verb:verb,kind:kind,name:name});
  note.innerHTML=r.ok?r.note:('<span class="err">'+r.note+'</span>');
  status(r.ok?'listo':'error',r.ok?'':'err');
  await refreshSaves();
  if(r.ok&&verb==='load') await refresh();
}

// The solve runs on the server's own thread; the page polls its progress and
// can raise the stop flag. A stopped solve is still a usable solution, just
// less converged.
let polling=null;

function setSolving(on){
  solvingLive=on;
  document.getElementById('goBtn').disabled=on;
  document.getElementById('buildBtn').disabled=on;
  // The server refuses every change to the session while a solve owns it, so
  // leaving these live meant clicks that did nothing but print a note -- and
  // the range grid painted itself first, showing a range the solver did not
  // have. Off while it runs, with the reason on hover. Navigating, reading and
  // recolouring stay live: looking is what the live view is for.
  const why=t('para el solve para cambiar esto');
  document.querySelectorAll('.needsIdle').forEach(el=>{
    el.disabled=on; el.title=on?why:'';
  });
  ['rangeGrid','deck','streets'].forEach(id=>{
    const el=document.getElementById(id); if(!el) return;
    el.style.pointerEvents=on?'none':'';
    el.style.opacity=on?'0.45':'';
    el.title=on?why:'';
  });
  document.getElementById('moreBtn').disabled=on||!(state&&state.solved);
  document.getElementById('stopBtn').style.display=on?'':'none';
  document.getElementById('stopTop').style.display=on?'':'none';
  espejaGo();
  document.getElementById('progWrap').style.display=on?'':'none';
  document.getElementById('progPill').style.display=on?'':'none';
}
// MEDIDO abriendo la pagina en limpio, con la ventana de 808 px que trae
// un portatil: el boton de Resolver estaba a 1823 px de alto en una pagina de
// 2208, o sea fuera de la primera pantalla, detras de la rejilla entera de
// rangos. Y la columna de la derecha repetia tres veces "resuelve para ver
// esto" sin que hubiera nada que pulsar a la vista. Ahora esta tambien en la
// cabecera, al lado de los avisos que hablan de el.
//
// El de arriba no tiene estado propio a proposito: lo copia del de abajo,
// que ya sabe si falta un rango, si hay locks sin aplicar y por que no se
// puede. Dos botones con dos estados acaban discrepando.
function espejaGo(){
  const g=document.getElementById('goBtn'), a=document.getElementById('goTop');
  if(!g||!a) return;
  a.disabled=g.disabled;
  a.title=g.title;
  a.classList.toggle('pend', g.classList.contains('pend'));
  a.style.display=solvingLive?'none':'';
}
function fmtSecs(x){
  if(x<90) return x.toFixed(0)+'s';
  const m=Math.floor(x/60), sc=Math.round(x%60);
  return m+'m '+(sc<10?'0':'')+sc+'s';
}

// Guardar y cargar rangos. Actua sobre el lado abierto en las pestanas, que es
// lo que uno esta mirando cuando le da al boton.
// El detalle de una nota, en su dialogo. El titulo es el resumen que ya se ve
// en la pantalla, para que se sepa de que campo se esta hablando.
function abreNota(el){
  const nota=el.closest('.note');
  if(!nota) return;
  const det=nota.querySelector('.det');
  if(!det) return;
  let titulo='';
  nota.childNodes.forEach(n=>{ if(n!==el && n!==det) titulo+=(n.textContent||''); });
  document.getElementById('noteTitle').textContent=titulo.replace(/\s+/g,' ').trim();
  document.getElementById('noteBody').innerHTML=det.innerHTML;
  document.getElementById('noteDlg').classList.add('on');
}
function cierraNota(){ document.getElementById('noteDlg').classList.remove('on'); }
document.addEventListener('keydown', e=>{ if(e.key==='Escape') cierraNota(); });
function openSizeHelp(){ document.getElementById('szDlg').classList.add('on'); }
function closeSizeHelp(){ document.getElementById('szDlg').classList.remove('on'); }

async function storeRange(verb){
  const sel=document.getElementById('rangePick');
  const name=(verb==='save')
    ? document.getElementById('rangeName').value.trim()
    : sel.value;
  if(!name){ status(verb==='save'?t('ponle un nombre'):t('no hay rangos guardados'),'err'); return; }
  // El lado solo hace falta para cargar uno de los ficheros de antes, que
  // llevaban un rango suelto: ahi se pone en el que estes mirando.
  const r=await api('/api/store',{verb:verb,kind:'range',name:name,
                                  player:tab===1?'ip':'oop'});
  if(!r.ok){ status(r.note||t('no se pudo'),'err'); return; }
  takeState(r,false);
  status(r.note,'on');
  if(verb==='save') document.getElementById('rangeName').value='';
  refreshSaves();
}

async function startSolve(more){
  const n=document.getElementById('iters').value;
  await api('/api/config',{threads:document.getElementById('threads').value,
                           iters:n});
  const r=await api('/api/solve',more?{iters:n,more:1}:{iters:n});
  if(!r.ok){ status(r.note||'busy','err'); return; }
  setSolving(true);
  status(more?t('añadiendo iteraciones…'):'resolviendo…','busy');
  document.getElementById('pbarFill').style.width='0%';
  document.getElementById('progNote').textContent='starting...';
  poll();
}
// Go throws the current solve away and starts over, which is what you want
// when the spot changed. "+ iteraciones" keeps it and adds to it, which is
// what you want after watching 109 of 5000 go by and stopping to look.
function solve(){ return startSolve(false); }
function solveMore(){ return startSolve(true); }

function poll(){
  clearTimeout(polling);
  polling=setTimeout(async()=>{
    const p=await api('/api/progress');
    if(!p || p.ok===false){        // the solver went away mid-solve
      showOffline(p);
      poll(); return;
    }
    const pct=p.total?Math.min(100,100*p.done/p.total):0;
    document.getElementById('pbarFill').style.width=pct.toFixed(1)+'%';
    const rate=p.done>0?p.seconds/p.done:0;
    const eta=(p.running&&p.done>2)?rate*(p.total-p.done):0;
    document.getElementById('progNote').innerHTML=
      pct.toFixed(1)+'%  ·  '+p.done+' / '+p.total+t(' iteraciones  ·  ')+
      fmtSecs(p.seconds)+(eta>0?'  ·  ~'+fmtSecs(eta)+' left':'')+
      (rate>0?'  ·  '+(rate*1000).toFixed(0)+' ms/iter':'')+
      (p.explPct>=0?'<br><b>'+t('explotable en un')+' '+p.explPct.toFixed(4)+
                    t('% del bote por mano')+'</b> ('+t('bote')+' '+state.pot+')':'');
    document.getElementById('progPill').textContent=
      t('resolviendo')+' '+pct.toFixed(0)+'%'+(p.explPct>=0?' · '+p.explPct.toFixed(2)+'%':'');
    // Mientras corre, la pildora va con el solve: es justo cuando se ve bajar.
    renderConv(p.explPct, !!p.running);
    if(p.running){
      // Refresh what is on screen every so often so the strategy fills in live.
      if(!liveTick || Date.now()-liveTick>1500){ liveTick=Date.now(); loadNode(curCls); }
      poll(); return;
    }
    liveTick=0;

    setSolving(false);
    // A solve can end with nothing solved -- the tree did not fit in the memory
    // limit, say. Reporting that as "solved" in green would be a lie, and the
    // reason is sitting right there in the note.
    const nothing = p.state && p.state.solved===false;
    if(nothing)        status(t('no se resolvio nada'),'err');
    else if(p.stopped) status(t('parado en ')+p.done+t(' de ')+p.total+t(' iteraciones'),'');
    else if(p.accHit)  status(t('precisión alcanzada en ')+p.done+t(' iteraciones')+' ('+
                              p.seconds.toFixed(2)+'s)','on');
    else               status(t('resuelto en ')+p.seconds.toFixed(2)+'s','on');
    takeState(p,false);
    const tn=document.getElementById('treeNote');
    const msg=(p.stopped?t('parado antes de tiempo — la estrategia vale, pero está menos convergida. '):'')+
              (p.note||'');
    if(nothing) tn.innerHTML='<span class="err">'+(p.note||t('no se resolvio nada'))+'</span>';
    else        tn.textContent=msg;
  },250);
}

async function stopSolve(){
  document.getElementById('stopBtn').disabled=true;
  await api('/api/stop',{});
  status(t('parando al terminar esta iteración...'),'busy');
  setTimeout(()=>{document.getElementById('stopBtn').disabled=false;},1500);
}

// ---------------------------------------------------------------- navigation
//  The line runs left to right. Selecting a cell picks the decision point you
//  are sitting at; the alternatives available there hang underneath the cell
//  that follows it, so switching branch is one click.
function lineInit(){
  line=[{label:'Raíz', cls:'root', ctx:0, node:state.tree[0].root}];
  sel=0; syncNav();
}
function lineValid(){
  if(!line.length) return false;
  for(const e of line){
    const c=state.tree[e.ctx];
    if(!c || !c.nodes[e.node]) return false;
  }
  return true;
}
function navAt(i){
  const slots=[];
  for(let k=1;k<=i;k++) if(line[k].slot!==undefined) slots.push(line[k].slot);
  return {ctx:line[i].ctx, node:line[i].node, slots:slots};
}
function syncNav(){ nav=navAt(sel); }

// Bets and raises are shaded by size: the biggest is the darkest.
function actClass(acts,ai){
  const k=acts[ai].kind;
  if(k===0) return 'fold';
  if(k===1||k===2) return 'chk';
  const sizes=acts.filter(x=>x.kind>=3).map(x=>x.to).sort((p,q)=>q-p);
  const r=sizes.indexOf(acts[ai].to);
  return 'bet'+Math.min(r<0?0:r,2);
}
function isRed(card){ return card[1]==='h'||card[1]==='d'; }

function renderNav(){
  const el=document.getElementById('ptree');
  const cp=document.getElementById('cardPick');
  const note=document.getElementById('navNote');
  el.innerHTML=''; cp.innerHTML=''; note.textContent='';
  document.getElementById('runoutPanel').style.display='none';
  if(!state.tree.length) return;
  if(!lineValid()) lineInit();
  if(sel>=line.length) sel=line.length-1;

  for(let i=0;i<line.length;i++){
    const col=document.createElement('div'); col.className='pcol';
    const e=line[i];
    const c=document.createElement('div');
    c.className='pcell '+(e.cls||'')+(i===sel?' sel':'');
    // Una carta repartida se dibuja como carta, con el mismo palo de cuatro
    // colores que el selector. Antes era una pastilla de texto: el mismo objeto
    // con dos aspectos distintos segun donde lo mirases.
    if(e.slot!==undefined){
      c.classList.add('pcard');
      c.appendChild(cardFace(e.label,'bcard sm'));
    } else {
      c.textContent=e.label;
    }
    c.title=i===0?t('principio del árbol'):t('mirar este punto');
    c.onclick=()=>{ sel=i; syncNav(); curCls=-1; renderNav(); renderSlots(); loadNode(); };
    col.appendChild(c);
    if(i===sel+1) addAlts(col);
    el.appendChild(col);
  }
  if(sel===line.length-1){          // nothing chosen yet: offer the choices
    const col=document.createElement('div'); col.className='pcol';
    addAlts(col);
    if(col.children.length) el.appendChild(col);
  }

  // El board de la linea que se esta mirando, aqui mismo: el de arriba enseña
  // el board inicial, y en un flop lo que importa es que carta ha caido en ESTA
  // rama. Tenerlo lejos obliga a recordarlo.
  const bv=document.getElementById('navBoard');
  if(bv){
    bv.innerHTML='';
    state.boardCards.forEach(c=>bv.appendChild(cardFace(c,'bcard sm')));
    for(let i=1;i<=sel;i++)
      if(line[i].slot!==undefined)
        bv.appendChild(cardFace(line[i].label,'bcard sm','dealt'));
  }

  const e=line[sel], ctx=state.tree[e.ctx], n=ctx.nodes[e.node];
  if(n.type===NT_DECISION){
    const where=(ctx.label==='R'?n.path:ctx.label+' '+n.path);
    // El camino interno (R, R/B33...) sirve para seguir una linea y para
    // ponerla al lado de la suya, pero en la raiz es solo una R suelta que no
    // dice nada: ahi se calla.
    note.textContent=t(ctx.streetName)+'  ·  '+(n.player===0?'OOP':'IP')+t(' juega  ·  bote ')+
                     n.pot.toFixed(1)+(where==='R'?'':'  ·  '+where);
  } else if(n.type===NT_CONT){
    note.textContent=t(ctx.streetName)+t('  ·  nodo de azar, bote ')+n.pot.toFixed(1);
    renderCards(cp,n,ctx);
    loadRunouts();
  } else {
    note.textContent=(n.type===NT_SHOWDOWN?'showdown':'fold')+t('  ·  bote ')+n.pot.toFixed(1)+
                     t('  — aquí no hay nada que resolver, vuelve atrás en la línea');
  }
}

function addAlts(col){
  const e=line[sel], ctx=state.tree[e.ctx], n=ctx.nodes[e.node];
  if(n.type!==NT_DECISION) return;
  n.actions.forEach((a,ai)=>{
    if(line[sel+1] && line[sel+1].act===ai) return;   // that one is already on top
    const b=document.createElement('div');
    b.className='pcell alt '+actClass(n.actions,ai);
    b.textContent=a.label.toUpperCase();
    const child=ctx.nodes[a.child];
    if(child.type===NT_FOLD)          b.title='fold, hand ends';
    else if(child.type===NT_SHOWDOWN) b.title='showdown';
    b.onclick=()=>takeAction(ai);
    col.appendChild(b);
  });
}

function renderCards(cp,n,ctx){
  const used=[];
  for(let k=1;k<=sel;k++) if(line[k].slot!==undefined) used.push(line[k].slot);
  const taken=(line[sel+1]&&line[sel+1].slot!==undefined)?line[sel+1].slot:-1;
  cp.innerHTML='<div class="note">Reparte el '+state.streets[ctx.street+1].name+
               (state.iso?t(' — las cartas que son equivalentes al permutar los palos ')+
                          t('comparten un solve, así que algunas de estas dan la misma ')+
                          'estrategia.':'')+
               '</div><div class="deck" id="cg"></div>';
  const cg=document.getElementById('cg');
  for(const su of SUITS){
    const row=document.createElement('div'); row.className='deckrow';
    for(const r of DECK_RANKS){
      const card=r+su;
      const slot=state.deck.indexOf(card);
      const gone=slot<0 || used.indexOf(slot)>=0;
      const d=cardFace(card,'pc',(gone?'dead':'')+(slot===taken?' taken':''));
      d.title=gone?(slot<0?t('está en el board'):t('ya repartida')):'repartir '+card;
      if(!gone) d.onclick=()=>dealCard(slot);
      row.appendChild(d);
    }
    cg.appendChild(row);
  }
}

// ------------------------------------------------------- runout aggregate
// One row per card that can still come. Sorting by an action column is the
// point of the whole thing: "which turns does OOP bet?" is a question you
// answer by looking down a sorted list, not by clicking 49 nodes.
let runouts=null, runoutSort={col:'card',desc:true}, runoutSeq=0;

function loadRunouts(){
  const panel=document.getElementById('runoutPanel');
  const body=document.getElementById('runoutBody');
  if(!state.solved && !solvingLive){ panel.style.display='none'; return; }
  panel.style.display='';
  body.innerHTML='<div class="note">leyendo...</div>';
  // Same rule as a node read: a sequence number says somebody asked later,
  // which is not the same as the user still being here. Navigating away
  // from a chance node issues no new request, so without this the answer
  // for the node you left lands in the table for the node you are on.
  const want=nav.ctx+'/'+nav.node+'/'+nav.slots.join(',');
  const seq=++runoutSeq;
  const url='/api/runouts?ctx='+nav.ctx+'&node='+nav.node+'&slots='+nav.slots.join(',');
  fetch(url).then(r=>r.json()).then(r=>{
    if(seq!==runoutSeq) return;            // a newer request already won
    if(want!==nav.ctx+'/'+nav.node+'/'+nav.slots.join(',')) return;   // and it moved
    if(!r.ok){ body.innerHTML='<div class="note err">'+(r.note||'error')+'</div>'; return; }
    // The answer names its own node, so this is checkable rather than
    // merely believed.
    if(r.ctx!==nav.ctx || r.node!==nav.node) return;
    runouts=r; renderRunouts();
  }).catch(()=>{ if(seq===runoutSeq) body.innerHTML='<div class="note err">sin respuesta</div>'; });
}

function sortRunouts(col){
  if(runoutSort.col===col) runoutSort.desc=!runoutSort.desc;
  else runoutSort={col:col,desc:true};
  renderRunouts();
}

function rankOf(card){ return DECK_RANKS.indexOf(card[0]); }

function renderRunouts(){
  const r=runouts; if(!r) return;
  document.getElementById('runoutTitle').textContent=
    '· '+r.street+' · decide '+(r.player===0?'OOP':'IP')+' · '+r.rows.length+' cartas';

  const rows=r.rows.slice();
  const s=runoutSort;
  rows.sort((a,b)=>{
    let d;
    // rankOf is 0 for an ace, so descending by card strength means ascending
    // by rankOf. Getting this backwards put the deuces at the top.
    if(s.col==='card')       d=(rankOf(a.card)-rankOf(b.card)) || a.card.localeCompare(b.card);
    else if(s.col==='reach') d=b.reach-a.reach;
    else if(s.col==='oop')   d=b.evOOP-a.evOOP;
    else if(s.col==='ip')    d=b.evIP-a.evIP;
    else                     d=b.freq[s.col]-a.freq[s.col];
    if(!s.desc) d=-d;
    // Ties keep a stable, readable order rather than whatever sort() decides.
    return d || (rankOf(a.card)-rankOf(b.card)) || a.card.localeCompare(b.card);
  });

  const arrow=c=>s.col===c?(s.desc?' ▾':' ▴'):'';
  let h='<div style="overflow-x:auto"><table><thead><tr>'+
        '<th style="cursor:pointer" onclick="sortRunouts(\'card\')">carta'+arrow('card')+'</th>'+
        '<th style="cursor:pointer" onclick="sortRunouts(\'reach\')">alcance'+arrow('reach')+'</th>';
  r.actions.forEach((a,i)=>{
    h+='<th style="cursor:pointer;color:'+actionColor(r.actions,i)+'" onclick="sortRunouts('+i+')">'+
       a.code+arrow(i)+'</th>';
  });
  h+='<th style="cursor:pointer" onclick="sortRunouts(\'oop\')">EV OOP'+arrow('oop')+'</th>'+
     '<th style="cursor:pointer" onclick="sortRunouts(\'ip\')">EV IP'+arrow('ip')+'</th></tr></thead><tbody>';

  for(const row of rows){
    h+='<tr>';
    h+='<td><span class="rc '+row.card[1]+'" onclick="dealCard('+row.slot+')" '+
       'title="repartir '+row.card+'">'+row.card[0]+SUIT_GLYPH[row.card[1]]+'</span></td>';
    h+='<td>'+row.reach.toFixed(1)+'%</td>';
    row.freq.forEach((f,i)=>{
      // A bar behind the number: the eye finds the pattern down the column
      // long before it finishes reading the digits.
      h+='<td style="position:relative"><span style="position:absolute;left:0;top:2px;bottom:2px;'+
         'width:'+Math.max(0,Math.min(100,f)).toFixed(1)+'%;background:'+actionColor(r.actions,i)+
         ';opacity:.30"></span><span style="position:relative">'+f.toFixed(1)+'%</span></td>';
    });
    h+='<td>'+row.evOOP.toFixed(2)+'</td><td>'+row.evIP.toFixed(2)+'</td>';
    h+='</tr>';
  }
  h+='</tbody><tfoot><tr style="font-weight:600">'+
     '<td>media</td><td></td>';
  r.actions.forEach(a=>{ h+='<td>'+a.avg.toFixed(1)+'%</td>'; });
  h+='<td>'+r.avgEvOOP.toFixed(2)+'</td><td>'+r.avgEvIP.toFixed(2)+'</td>'+
     '</tr></tfoot></table></div>'+
     '<div class="note">La media pondera cada carta por su alcance: una que bloquea '+
     'medio rango no cuenta lo mismo que una que no bloquea nada.</div>';
  document.getElementById('runoutBody').innerHTML=h;
}

function takeAction(ai){
  const e=line[sel], ctx=state.tree[e.ctx], n=ctx.nodes[e.node];
  line=line.slice(0,sel+1);
  line.push({label:n.actions[ai].label.toUpperCase(), cls:actClass(n.actions,ai),
             ctx:e.ctx, node:n.actions[ai].child, act:ai});
  sel=line.length-1; syncNav(); curCls=-1; renderNav(); renderSlots(); loadNode();
}
function dealCard(slot){
  const e=line[sel], ctx=state.tree[e.ctx], n=ctx.nodes[e.node];
  const card=state.deck[slot];
  line=line.slice(0,sel+1);
  line.push({label:card, cls:'chance'+(isRed(card)?' red':''),
             ctx:n.contCtx, node:state.tree[n.contCtx].root, slot:slot});
  sel=line.length-1; syncNav(); curCls=-1; renderNav(); renderSlots(); loadNode();
}

// ---------------------------------------------------------------- node
async function loadNode(cls){
  if(!state.solved && !solvingLive){ node=null; renderStrategy(); renderCombos(); renderMade(); return; }
  if(nodeOf().type!==NT_DECISION){ node=null; renderStrategy(); renderCombos(); renderMade(); return; }
  // Reading a node takes the solver's lock, and mid-solve the worker only
  // lets go between chunks -- a third of a second, plus whatever is queued
  // ahead. Leaving the previous node's numbers on screen for that long is
  // worse than showing nothing: the header says IP to act above OOP's
  // strategy, and the click reads as having done nothing at all. So the
  // moment the target changes, the old numbers go.
  if(node && (node.ctx!==nav.ctx || node.node!==nav.node)){
    // El filtro no viaja entre nodos: "la accion 0" en uno es pasar y en otro
    // tirarse, y arrastrarlo seria ensenar un rango que nadie ha pedido.
    actFilter=-1; madeFilter=null;
    node=null; nodePending=true;
    renderStrategy(); renderCombos(); renderMade(); renderHud();
  }
  const pill=document.getElementById('statusPill');
  const prev=pill.textContent, prevCls=pill.className.replace('pill','').trim();
  if(!solvingLive) status(t('leyendo el nodo...'),'busy');
  let url='/api/node?ctx='+nav.ctx+'&node='+nav.node+'&slots='+nav.slots.join(',');
  // La mejor respuesta recorre el arbol entero, o sea cuesta como una
  // iteracion. Solo se pide cuando esta puesta la casilla: navegar nodos no
  // tiene por que pagarla.
  if(verBR) url+='&br=1';
  if(cls!==undefined&&cls>=0) url+='&cls='+cls;
  // Where this request is for. A sequence number alone says "somebody asked
  // later", which is not the same as "this is still where the user is": the
  // periodic live refresh and a click can interleave either way round. The
  // answer is only allowed to land if the user has not moved since.
  const want=nav.ctx+'/'+nav.node+'/'+nav.slots.join(',');
  const seq=++nodeSeq;
  const resp=await api(url);
  if(seq!==nodeSeq) return;          // a newer request already answered
  if(want!==nav.ctx+'/'+nav.node+'/'+nav.slots.join(',')) return;   // and it moved
  nodePending=false;
  if(resp.ok) node=resp;
  else if(resp.busy){                // never fall back to stale numbers silently
    status(t('hay un solve en marcha — los datos del nodo esperan'),'busy');
    renderStrategy(); renderCombos(); renderMade(); renderHud();
    return;
  } else node=null;
  if(!solvingLive)
    status(prev===t('leyendo el nodo...')?(state.solved?'ready':'idle'):prev,prevCls);
  renderStrategy(); renderCombos(); renderMade(); renderHud();
}
// The HUD: what this exact spot is worth to each player. The two always add up
// to the pot -- they are splitting it, not earning it separately -- so the bar
// shows the split, and the delta says how the line has shifted it against the
// value of the whole game.
function renderHud(){
  const setTxt=(id,t)=>{document.getElementById(id).textContent=t;};
  const meta=document.getElementById('hudMeta');
  document.getElementById('hudOOP').classList.remove('act');
  document.getElementById('hudIP').classList.remove('act');

  if(!state || (!state.solved && !node)){
    setTxt('evOOP','-'); setTxt('evIP','-');
    setTxt('subOOP',''); setTxt('subIP','');
    document.getElementById('splitA').style.width='50%';
    document.getElementById('splitB').style.width='50%';
    meta.innerHTML='<span>resuelve para ver el reparto</span>';
    return;
  }
  const pot=state.pot;
  // The whole-game reference only exists once a state has come back solved;
  // mid-solve there is none, so the delta is simply left out.
  const haveRoot=(typeof state.evOOP==='number');
  const o = node ? node.evOOP : (haveRoot?state.evOOP:pot/2);
  const i = node ? node.evIP  : (haveRoot?state.evIP :pot/2);

  setTxt('evOOP',o.toFixed(3));
  setTxt('evIP', i.toFixed(3));
  const sign=x=>(x>=0?'+':'')+x.toFixed(3);
  const dtxt=(v,ref)=>haveRoot
    ? ' &nbsp; <span class="'+((v-ref)>=0?'up':'dn')+'" title="Respecto al valor de todo el juego, que es lo que vale la raíz">'+sign(v-ref)+' '+t('vs la raíz')+'</span>'
    : '';
  document.getElementById('subOOP').innerHTML=
    (100*o/pot).toFixed(1)+t('% del bote')+dtxt(o,state.evOOP);
  document.getElementById('subIP').innerHTML=
    (100*i/pot).toFixed(1)+t('% del bote')+dtxt(i,state.evIP);
  document.getElementById('splitA').style.width=(100*o/pot)+'%';
  document.getElementById('splitB').style.width=(100*i/pot)+'%';

  if(node) document.getElementById(node.player===0?'hudOOP':'hudIP').classList.add('act');

  const bits=[];
  if(node){
    bits.push('<span>'+state.streets[node.street].name+'</span>');
    bits.push('<span>bote en el nodo <b>'+node.pot.toFixed(1)+'</b></span>');
    bits.push('<span>llega el <b>'+node.reach.toFixed(2)+'%</b> de las manos</span>');
    bits.push('<span><b>'+(node.player===0?'OOP':'IP')+'</b> juega</span>');
  } else {
    bits.push('<span>partida entera</span>');
    bits.push('<span>pot <b>'+pot.toFixed(1)+'</b></span>');
    bits.push('<span>elige un nodo de decisión para ver su reparto</span>');
  }
  bits.push('<span>OOP + IP = <b>'+(o+i).toFixed(2)+'</b> = el bote</span>');
  if(state.explPct>=0)
    bits.push('<span>explotable en un <b>'+state.explPct.toFixed(4)+
              '%</b> del bote por mano</span>');
  meta.innerHTML=bits.join('');
}

function renderStrategy(){
  const g=document.getElementById('stratGrid');
  const info=document.getElementById('nodeInfo');
  const bar=document.getElementById('freqBar');
  const leg=document.getElementById('legend');
  g.innerHTML=''; bar.innerHTML=''; leg.innerHTML='';
  document.getElementById('nodeTitle').textContent='';
  if(!node){
    info.innerHTML='<span>'+(nodePending
        ? 'leyendo el nodo…'
        : (state && state.solved ? t('elige un nodo del árbol')
           : (solvingLive ? 'aún no hay nada que leer en este nodo'
                          : t('resuelve para ver la estrategia'))))+'</span>';
    if(nodePending) document.getElementById('nodeTitle').textContent=' - '+navWhere();
    return;
  }
  document.getElementById('nodeTitle').textContent=' - '+node.ctxLabel+' '+node.path;
  info.innerHTML='<span><b>'+(node.player===0?'OOP':'IP')+'</b> juega</span>'+
    '<span>street <b>'+state.streets[node.street].name+'</b></span>'+
    '<span>pot <b>'+node.pot.toFixed(1)+'</b></span>'+
    '<span>EV del nodo <b>'+node.ev.toFixed(3)+'</b></span>'+
    (node.locked?'<span style="color:#ffd479">nodelocked</span>':'');

  // Una caja por accion, de ancho fijo, y pinchando se filtra el rango a las
  // manos que la toman. Es lo que hace la referencia y es lo util: ver "el rango
  // que apuesta" en la rejilla, no solo cuanto apuesta.
  const btns=document.getElementById('actBtns');
  btns.innerHTML='';
  if(actFilter>=node.actions.length) actFilter=-1;
  node.actions.forEach((a,i)=>{
    const c=actionColor(node.actions,i);
    const b=document.createElement('div');
    b.className='act-btn'+(actFilter===i?' on':(actFilter>=0?' off':''));
    b.style.background=c;
    b.innerHTML='<b>'+a.label+'</b>'+
      (typeof a.combos==='number'
        ? '<span class="cmb">'+a.combos.toFixed(1)+' combos</span>' : '')+
      '<span class="abf">'+a.freq.toFixed(2)+'%</span>';
    b.title=actFilter===i ? t('pinchar otra vez para quitar el filtro')
                          : t('ver solo el rango que hace ')+a.label;
    b.onclick=()=>{ actFilter=(actFilter===i?-1:i); renderStrategy(); renderCombos(); };
    btns.appendChild(b);

    const d=document.createElement('div');
    d.style.width=a.freq+'%'; d.style.background=c;
    d.textContent=a.freq>8?(a.code+' '+a.freq.toFixed(1)+'%'):'';
    bar.appendChild(d);
    const l=document.createElement('span');
    // The second figure is the same decision across every runout that gets
    // here. On a turn node the first says what happens on the Ts, the second
    // says what happens on a turn -- and reading only the first is how you
    // end up learning a card instead of a spot.
    l.innerHTML='<i style="background:'+c+'"></i>'+a.label+' '+a.freq.toFixed(1)+'%'+
      (typeof a.freqAll==='number'
        ? '<span style="color:var(--dim)"> · '+a.freqAll.toFixed(1)+'% en todos los runouts</span>'
        : '');
    l.title=(typeof a.freqAll==='number')
      ? a.label+': '+a.freq.toFixed(2)+t('% en este runout, ')+a.freqAll.toFixed(2)+
        t('% promediado sobre los ')+node.instances+t(' runouts guardados de esta línea')
      : a.label+': '+a.freq.toFixed(2)+'%';
    leg.appendChild(l);
  });

  const mode=document.getElementById('gridMode').value;
  // A class with three combos and one with twelve get the same square, which
  // makes the rare one shout as loudly as the common one. Optionally the
  // coloured area is scaled to its share of the range -- by the square root,
  // so it is the AREA that is proportional and not the side. Normalised
  // against the heaviest class, or a 1%-of-range square would be invisible.
  const byW=document.getElementById('gridWeight').checked;
  // Filtrar es quedarse con ESO. No atenuar el resto: lo que no pasa el filtro
  // desaparece de la rejilla igual que un combo que no esta en el rango, y lo
  // que pasa se pinta del color de su accion y nada mas. Si sigue viendose el
  // rango entero en otro tono, no se ha filtrado nada -- se ha subrayado.
  //
  // El peso de la categoria hay que rehacerlo desde los combos, porque la
  // casilla de la rejilla ya viene sumada y no se puede des-sumar.
  let clsW=null;
  if(madeFilter && node.combos){
    clsW=new Array(169).fill(0);
    node.combos.forEach(c=>{ if(comboIn(c)) clsW[c.cls]+=c.w; });
  }
  const filtrando = (actFilter>=0) || !!clsW;
  // Lo que sobrevive de una casilla, en peso.
  const wOf=(cell,idx)=>{
    if(!cell) return 0;
    const base = clsW ? clsW[idx] : cell.w;
    return actFilter>=0 ? base*(cell.f[actFilter]||0)/100 : base;
  };
  let maxW=0;
  if(byW) for(let k=0;k<169;k++) if(node.grid[k]) maxW=Math.max(maxW,wOf(node.grid[k],k));
  for(let i=0;i<13;i++)for(let j=0;j<13;j++){
    const idx=i*13+j, cell=node.grid[idx];
    // El numero, solo en los modos que van de numeros: en estrategia la
    // casilla ya lleva el reparto pintado y un numero mas seria ruido.
    const valor = (cell && mode!=='strat')
        ? (mode==='eq' ? cell.eq.toFixed(1) : cell.ev.toFixed(2)) : '';
    const c=document.createElement('div');
    c.className='cell'+(idx===curCls?' sel':'')+(cell&&cell.lk?' lk':'');
    c.innerHTML='<div class="fill"></div><div class="lab">'+clsName(i,j)+'</div>'+
                (valor==='' ? '' : '<div class="val">'+valor+'</div>');
    const fill=c.querySelector('.fill');
    const queda = wOf(cell,idx);
    // Fuera del filtro: la casilla se queda como una que no esta en el rango.
    const fuera = filtrando && cell && queda <= 1e-9;
    // Cuanto de la casilla llena el color. Con el peso proporcional marcado,
    // contra la casilla mas gorda; si no, con un filtro puesto es el TROZO que
    // sobrevive, para que una casilla que apuesta un cuarto se vea un cuarto.
    let area = 1;
    if(byW && cell && maxW>0)        area = queda/maxW;
    else if(filtrando && cell && cell.w>1e-9) area = queda/cell.w;
    if(cell && !fuera && area<1){
      // Como en los demas: el color ocupa el ANCHO ENTERO y crece desde abajo. Antes
      // se encogia un cuadrado hacia el CENTRO, y con pesos pequenos quedaba
      // un sello diminuto en mitad de la casilla, justo encima del nombre.
      const alto=Math.max(0,Math.min(1,area));
      fill.style.inset='auto 0 0 0';
      fill.style.height=(100*alto).toFixed(2)+'%';
    }
    if(!cell || fuera){
      // Gris solido, no transparente: con la letra en negro, lo transparente
      // dejaba el nombre de la casilla sobre el fondo oscuro de la pagina.
      // Sin atenuar: el gris ya dice "aqui no hay nada", y bajarle la opacidad
      // encima dejaba el nombre de la casilla en negro sobre gris oscuro, que es
      // lo que se venia a arreglar. Lo filtrado va un tono mas apagado que lo
      // que nunca estuvo en el rango, pero los dos se leen.
      fill.style.background = fuera ? 'var(--out2)' : 'var(--out)';
      fill.style.inset='0';
      c.style.opacity='1';
    }
    else{
      if(mode==='strat'){
        if(actFilter>=0){
          // Una sola accion: un solo color. El degradado de todas contaria lo
          // que hace con el resto, que es lo que se ha pedido quitar.
          fill.style.background=actionColor(node.actions,actFilter);
        } else {
          let acc=0; const stops=[];
          node.actions.forEach((a,k)=>{
            const col=actionColor(node.actions,k);
            stops.push(col+' '+acc+'%'); acc+=cell.f[k]; stops.push(col+' '+acc+'%');
          });
          // En HORIZONTAL, como los demas: las acciones se reparten de izquierda a
          // derecha. En vertical chocaba con la barra de peso, que tambien
          // crece de abajo arriba, y no se sabia que decia cada cosa.
          fill.style.background='linear-gradient(to right,'+stops.join(',')+')';
        }
      } else if(mode==='eq'){
        // Rojo a verde, y con suelo de luz: la letra de la casilla es NEGRA,
        // como en la referencia, y sobre un rojo oscuro no se lee. Con el numero dentro
        // esto pasa de ser un detalle a ser la diferencia entre leerlo y no.
        fill.style.background='hsl('+(cell.eq*1.2).toFixed(0)+',70%,'+
                              (38+cell.eq*0.14).toFixed(0)+'%)';
      } else {
        const tb=Math.max(0,Math.min(1,cell.ev/(node.pot||1)));
        fill.style.background='hsl('+(tb*120).toFixed(0)+',68%,'+
                              (38+tb*14).toFixed(0)+'%)';
      }
      c.title=clsName(i,j)+'  eq '+cell.eq.toFixed(1)+'%  EV '+cell.ev.toFixed(2)+'  '+
              node.actions.map((a,k)=>a.code+' '+cell.f[k].toFixed(0)+'%').join('  ');
      c.onclick=()=>{curCls=idx;renderCombos(); renderMade();};
      c.onmousemove=(e)=>showHoverCard(idx,e);
      c.onmouseleave=hideHoverCard;
      if(filtrando) c.title += '   (filtrado)';
    }
    g.appendChild(c);
  }
}
// The reference solvers show you the individual suit combinations when the pointer is
// over a square, and that is the right place for them: the square is an
// average, and the average is exactly what hides A(h)K(h) playing differently
// from A(s)K(s). The numbers were already being computed -- they were just
// behind a click and a round trip.
function combosOfClass(cls){
  if(!node||!node.combos) return [];
  return node.combos.filter(c=>c.cls===cls);
}
// Los combos de una casilla, en rejilla y con los palos, como en la referencia.
//
// La casilla de la rejilla es un promedio de hasta doce manos, y ese promedio
// esconde justo lo que se quiere mirar: en un board de corazones, el AKs de
// corazones no juega como el de picas, y la casilla AKs dice una sola cosa de
// los cuatro. Aqui salen los cuatro, cada uno con lo suyo.
//
// Las que no existen -- bloqueadas por el board -- salen en gris y no en blanco:
// que una mano no se pueda tener es informacion, y borrarla de la rejilla haria
// creer que el hueco no estaba.
function hoverCells(idx){
  const i=Math.floor(idx/13), j=idx%13;
  const hi=RANKS[Math.min(i,j)], lo=RANKS[Math.max(i,j)];
  const par=(i===j), suited=(i<j && i!==j);
  const pares=[];
  if(par){
    for(let a=0;a<4;a++) for(let b=a+1;b<4;b++) pares.push([SUITS[a],SUITS[b]]);
  } else if(suited){
    for(let a=0;a<4;a++) pares.push([SUITS[a],SUITS[a]]);
  } else {
    for(let a=0;a<4;a++) for(let b=0;b<4;b++) if(a!==b) pares.push([SUITS[a],SUITS[b]]);
  }
  return {pares, hi, lo, cols: par?3:(suited?2:3)};
}

// La clave de un combo, en el mismo orden que hoverCells los pide: primero la
// carta alta. En una pareja no hay alta, asi que los palos van ordenados.
function hoverKey(name){
  const a=name.slice(0,2), b=name.slice(2,4);
  const ra=RANKS.indexOf(a[0]), rb=RANKS.indexOf(b[0]);
  if(ra===rb){
    const x=SUITS.indexOf(a[1]), y=SUITS.indexOf(b[1]);
    return x<y ? a[1]+b[1] : b[1]+a[1];
  }
  return ra<rb ? a[1]+b[1] : b[1]+a[1];
}

function showHoverCard(idx,ev){
  const el=document.getElementById('hoverCard');
  const list=combosOfClass(idx);
  if(!node||!list.length){ el.style.display='none'; return; }
  const cell=node.grid[idx];
  const mode=document.getElementById('gridMode').value;
  const {pares,hi,lo,cols}=hoverCells(idx);
  const porClave={};
  list.forEach(c=>{ porClave[hoverKey(c.name)]=c; });

  const modo = mode==='eq' ? 'equity' : (mode==='ev' ? 'EV' : 'frecuencia');
  let h='<h4>'+clsName(Math.floor(idx/13),idx%13)+
        ' &nbsp;<span style="color:var(--dim);font-weight:400">'+list.length+
        (list.length===1?' combo':' combos')+
        (cell?' · eq '+cell.eq.toFixed(1)+'%':'')+' · '+modo+'</span></h4>';
  h+='<div class="hgrid" style="grid-template-columns:repeat('+cols+',1fr)">';
  pares.forEach(([sa,sb])=>{
    let c=porClave[sa+sb];
    // Con una categoria filtrada, los combos de la casilla que no son de esa
    // categoria salen como los que no existen. Filtrar es quedarse con eso
    // tambien aqui; si no, la rejilla dice una cosa y el raton otra.
    if(c && madeFilter && !comboIn(c)) c=null;
    const cartas='<span class="'+sa+'">'+hi+SUIT_GLYPH[sa]+'</span> '+
                 '<span class="'+sb+'">'+lo+SUIT_GLYPH[sb]+'</span>';
    if(!c){ h+='<div class="hc out">'+cartas+'</div>'; return; }
    // El fondo partido por la accion, como la casilla de la rejilla: se ve de
    // un vistazo si esta mano es de apostar o de pasar, sin leer numeros.
    let acc=0; const stops=[];
    node.actions.forEach((a,k)=>{
      const col=actionColor(node.actions,k);
      stops.push(col+' '+acc+'%'); acc+=c.f[k]; stops.push(col+' '+acc+'%');
    });
    let filas='';
    if(mode==='eq'){
      filas='<div class="hr"><span>eq</span><b>'+c.eq.toFixed(1)+'%</b></div>';
    } else {
      node.actions.forEach((a,k)=>{
        filas+='<div class="hr"><span>'+a.code+'</span><b>'+
               (mode==='ev' ? c.e[k].toFixed(2) : c.f[k].toFixed(1)+'%')+'</b></div>';
      });
    }
    h+='<div class="hc'+(c.lk?' lkd':'')+'" style="background:linear-gradient(to right,'+
       stops.join(',')+')">'+cartas+(c.lk?' *':'')+filas+'</div>';
  });
  h+='</div>';
  el.innerHTML=h;
  // Not '': the stylesheet says none, so clearing the inline style puts it
  // straight back.
  el.style.display='block';
  // Kept on screen: near the pointer, but flipped when it would fall off.
  const r=el.getBoundingClientRect();
  let x=ev.clientX+16, y=ev.clientY+14;
  if(x+r.width  > window.innerWidth  - 8) x=ev.clientX-r.width-16;
  if(y+r.height > window.innerHeight - 8) y=Math.max(8,ev.clientY-r.height-14);
  el.style.left=x+'px'; el.style.top=y+'px';
}
function hideHoverCard(){ document.getElementById('hoverCard').style.display='none'; }
// "AhKs" -> coloured pips, so the suits read at a glance like they do on a table.
function cardsHTML(nm){
  let out='';
  for(let i=0;i+1<nm.length;i+=2){
    const r=nm[i], su=nm[i+1];
    out+='<span class="'+su+'">'+r+SUIT_GLYPH[su]+'</span> ';
  }
  return out;
}

let verBR=false;
function toggleBR(){
  verBR=document.getElementById('brChk').checked;
  loadNode();
}
// Plegar el montaje, y recordarlo entre sesiones: quien lo pliega lo quiere
// plegado la proxima vez, no una decision nueva cada arranque.
let setupPlegado=false;
function pintaSetupBtn(){
  const b=document.getElementById('setupBtn');
  if(!b) return;
  const n=(state && state.locks) ? state.locks.length : 0;
  // Plegado se lleva por delante la tira de locks, asi que el numero se dice
  // aqui: esconder el montaje no puede esconder que hay manos bloqueadas.
  b.textContent=(setupPlegado?'▶ ':'◀ ')+t('Montaje')+
                (setupPlegado && n ? '  · '+n+' lock'+(n===1?'':'s') : '');
}
function aplicaSetup(){
  document.querySelector('main').classList.toggle('plegado', setupPlegado);
  pintaSetupBtn();
}
function toggleSetup(){
  setupPlegado=!setupPlegado;
  try{ localStorage.setItem('dcfr.setup', setupPlegado?'1':'0'); }catch(e){}
  aplicaSetup();
}
try{ setupPlegado = localStorage.getItem('dcfr.setup')==='1'; }catch(e){}

// Pinchar una categoria la deja sola en la rejilla y en la tabla de combos.
// Es lo que su nombre prometia y no hacia: era una tabla que se leia.
function pickMade(kind,cls){
  madeFilter = (madeFilter && madeFilter.kind===kind && madeFilter.cls===cls)
             ? null : {kind:kind, cls:cls};
  renderStrategy(); renderCombos(); renderMade();
}
// Lo que has pedido con las barras y todavia no se ha resuelto.
//
// Al soltar la barra, la fila se volvia a pintar con los numeros del solve que
// hay -- que no lleva el lock -- asi que la barra saltaba a su sitio de antes y
// parecia que no habia pasado nada. Y la marca de bloqueada tampoco salia,
// porque esa sale del solver y el lock no entra en el hasta el siguiente solve.
//
// Asi que la peticion se guarda aqui: la fila entera reequilibrada con la misma
// cuenta que hace el servidor -- la accion movida a lo que pediste y el resto en
// proporcion a lo que tenian --, y se pinta eso hasta que se resuelva. Cuando el
// servidor dice que ya no hay locks pendientes, esto se vacia solo.
let pend = { nodo: '', fam: {} };

function pendNodo(){ return nav.ctx + '|' + nav.node + '|' + nav.slots.join(','); }
function pendDe(g){
  if(pend.nodo !== pendNodo()) return null;
  const v = pend.fam[g.kind + '|' + g.cls];
  return (v && v.length) ? v : null;
}
function pendPon(g, ai, pct){
  if(pend.nodo !== pendNodo()) pend = { nodo: pendNodo(), fam: {} };
  const base = (pendDe(g) || g.f).slice();
  const movido = pct - base[ai];
  let resto = 0;
  for(let i = 0; i < base.length; i++) if(i !== ai) resto += base[i];
  const out = base.slice();
  out[ai] = pct;
  for(let i = 0; i < base.length; i++){
    if(i === ai) continue;
    out[i] = (resto > 1e-9) ? base[i] - movido * base[i] / resto
                            : base[i] - movido / (base.length - 1);
    if(out[i] < 0) out[i] = 0;
  }
  pend.fam[g.kind + '|' + g.cls] = out;
}
function pendQuita(g){
  if(pend.nodo !== pendNodo()) return;
  delete pend.fam[g.kind + '|' + g.cls];
}
function pendVacia(){ pend = { nodo: '', fam: {} }; }

// Deshacer una familia: se le quitan SUS locks en este nodo y se quedan los
// demas. Arrepentirse de las dobles parejas no puede llevarse por delante lo
// que hiciste con los colores.
async function resetFamilia(fam, kind, cls){
  const r = await api('/api/unlock', {ctx:nav.ctx, node:nav.node,
                                      slots:nav.slots.join(','), hands:fam});
  pendQuita({kind:kind, cls:cls});
  status(r.ok ? (fam + ': ' + r.note) : (r.note || t('ahi no habia nada')), r.ok ? 'on' : '');
  if(r.state) takeState(r, false);
  loadNode();
}

// Las barras del reparto por categorias: mover una familia entera.
//
// Lo que se manda es el NOMBRE de la familia -- "two_pair" --, no la lista de
// combos: el servidor sabe resolverlo, y lo resuelve con el board DEL NODO, que
// es lo unico correcto (top pair en el river no es top pair en el flop). La
// consola entiende el mismo nombre: `lock two_pair B=90%`.
//
// Y va por INDICE de accion, no por tipo: un river con dos tamanos tiene dos
// apuestas, y "la apuesta" no dice cual.
let barra=null;

function barraPct(ev,el){
  // Sobre el CARRIL, no sobre la celda: la celda tiene 4px de aire a cada lado
  // y con ellos dentro el 100% caia antes de llegar al final de la barra.
  const t2=el.querySelector('.ftrack');
  const r=(t2||el).getBoundingClientRect();
  if(r.width<=0) return 0;
  return Math.max(0,Math.min(100,100*(ev.clientX-r.left)/r.width));
}
function barraEmpieza(ev,el,ai){
  ev.preventDefault(); ev.stopPropagation();
  if(!node||!node.actions||!node.actions[ai]) return;
  const fam=el.getAttribute('data-fam');
  if(!fam) return;
  const g={kind:+el.getAttribute('data-kind'), cls:+el.getAttribute('data-cls'),
           f:(node.made||[]).filter(z=>z.name===fam).map(z=>z.f)[0]||[]};
  barra={fam:fam,ai:ai,el:el,g:g,pct:barraPct(ev,el),x:ev.clientX,y:ev.clientY};
  document.body.classList.add('arrastrando');
  barraPinta();
}
function barraPinta(){
  if(!barra) return;
  const f=barra.el.querySelector('.ffill');
  const e=barra.el.querySelector('.fedge');
  const tb=barra.el.querySelector('.fnum');
  const w=barra.pct.toFixed(1);
  if(f) f.style.width=w+'%';
  if(e) e.style.left=w+'%';
  if(tb) tb.textContent=w;
  const b=document.getElementById('fambadge');
  if(b){
    b.textContent=barra.fam+'  '+w+'%';
    b.style.display='block';
    b.style.left=Math.round(barra.x+14)+'px';
    b.style.top=Math.round(barra.y-28)+'px';
  }
}
function barraTermina(){
  const b=document.getElementById('fambadge');
  if(b) b.style.display='none';
  document.body.classList.remove('arrastrando');
}
document.addEventListener('mousemove',(ev)=>{
  if(!barra) return;
  barra.pct=barraPct(ev,barra.el);
  barra.x=ev.clientX; barra.y=ev.clientY;
  barraPinta();
});
document.addEventListener('mouseup',()=>{
  if(!barra) return;
  const b=barra; barra=null;
  barraTermina();
  moverFamilia(b.fam,b.ai,b.pct,b.g);
});

async function moverFamilia(fam,ai,pct,g){
  if(!node||!node.actions||!node.actions[ai]) return;
  const code=node.actions[ai].code;
  // Se guarda lo pedido ANTES de ir al servidor, para que la barra se quede
  // donde la soltaste en vez de volver al numero del solve viejo.
  if(g && g.f && g.f.length) pendPon(g,ai,pct);
  status(t('fijando ')+fam+'...','busy');
  const r=await api('/api/lock',{ctx:nav.ctx,node:nav.node,
                                 slots:nav.slots.join(','),
                                 how:'nudge',hands:fam,act:code.charAt(0),
                                 acti:ai,mode:'fixed',w:(pct/100).toFixed(4)});
  if(r.ok){
    status(r.note+' — dale a Resolver','on');
    takeState(r,false);
    loadNode();
  } else {
    status(r.note||t('no se pudo'),'err');
  }
}

function renderMade(){
  const p=document.getElementById('madePanel');
  const tb=document.getElementById('madeTable');
  if(!node||!node.made||!node.made.length){ p.style.display='none'; return; }
  p.style.display='';
  const ncol = 4 + node.actions.length + 1;
  let h='<thead><tr><th>categoría</th><th>combos</th><th>'+t('% rango')+'</th><th>eq%</th>';
  node.actions.forEach((a,i)=>{
    h+='<th><span class="achip" style="background:'+actionColor(node.actions,i)+
       '"></span>'+a.code+' %</th>';
  });
  h+='<th>EV</th></tr></thead><tbody>';
  // Dos bloques que no se cruzan, como el Range Explorer de la referencia: mano hecha
  // arriba, proyecto debajo, y cada uno reparte el rango entero. Por eso hay dos
  // totales del 100% y no uno.
  let visto=-1, sumaC=0, sumaW=0;
  // La barra de peso va en proporcion a la familia mas grande: repartido sobre
  // 100 casi todo sale a cero y no se distingue nada.
  const maxW=node.made.reduce((m,z)=>Math.max(m,z.w),0);
  const total=(etiqueta)=>{
    h+='<tr class="cattot"><td style="text-align:left">'+etiqueta+'</td><td>'+
       sumaC.toFixed(1)+'</td><td>'+sumaW.toFixed(1)+'%</td>'+
       '<td colspan="'+(ncol-3)+'"></td></tr>';
    sumaC=0; sumaW=0;
  };
  node.made.forEach(g=>{
    if(g.kind!==visto){
      if(visto>=0) total(t('todo'));
      visto=g.kind;
      h+='<tr class="catsub"><td style="text-align:left" colspan="'+ncol+'">'+
         t(g.kind===1?'proyecto':'mano hecha')+'</td></tr>';
    }
    sumaC+=g.combos; sumaW+=g.w;
    const on = madeFilter && madeFilter.kind===g.kind && madeFilter.cls===g.cls;
    // Una familia que cabe en el board pero que el rango no tiene sale igual, a
    // cero, porque eso dice algo. Pero sin eq ni EV al lado: ahi no hay manos, y
    // un 0,0 se lee como que valen cero.
    const vacia = g.combos <= 1e-9;
    // Lo pedido y sin resolver manda sobre lo que hay en pantalla: es lo que
    // acabas de arrastrar. Si no hay nada pedido, se pinta el solve. Se calcula
    // ANTES de escribir la fila porque el boton de deshacer sale en la primera
    // celda y necesita saberlo.
    const ped=pendDe(g);
    const suyos=(node.combos||[]).filter(c=>g.kind===1?c.di===g.cls:c.mi===g.cls);
    const bloq=!!ped || (suyos.length>0 && suyos.every(c=>c.lk));
    h+='<tr class="pick'+(on?' on':'')+(vacia?' vac':'')+
       '" onclick="pickMade('+g.kind+','+g.cls+')">'+
       '<td style="text-align:left">'+g.name+
       (bloq?' <button class="undo" title="'+t('devolver')+' '+g.name+
             t(' a lo que hacia la solucion')+'" onclick="event.stopPropagation();resetFamilia(&quot;'+
             g.name+'&quot;,'+g.kind+','+g.cls+')">↺</button>':'')+'</td>'+
       '<td>'+g.combos.toFixed(1)+'</td>'+
       '<td><span class="wtrack"><i style="width:'+
           (maxW>0?Math.min(100,100*g.w/maxW):0).toFixed(1)+'%"></i>'+
           '<em>'+g.w.toFixed(1)+'%</em></span></td>'+
       // La equity, con un fondo que va de rojo a verde: cual familia esta por
       // delante se ve sin leer trece numeros.
       '<td'+(vacia?'':' style="background:hsl('+(1.2*Math.max(0,Math.min(100,g.eq))).toFixed(0)+
             ',38%,15%)"')+'>'+(vacia?'–':g.eq.toFixed(1))+'</td>';
    // Cada celda de accion es una barra que se arrastra: mover una familia
    // entera de golpe es el nodelock que la gente de verdad quiere hacer
    // -- "este tio nunca frena con dos parejas" -- y pintar cuarenta combos a
    // mano para decir eso es justo lo que hace que nadie lo use.
    (ped||g.f).forEach((x,i)=>{
      if(vacia){ h+='<td>–</td>'; return; }
      // El nombre de la familia va en un `data-`, no dentro del atributo: asi no
      // hay comillas dentro de comillas y el nombre puede ser el que sea.
      const an=Math.max(0,Math.min(100,x)).toFixed(1);
      h+='<td class="fambar'+(bloq?' lk':'')+(ped?' pend':'')+'" data-fam="'+g.name+'"'+
         ' data-kind="'+g.kind+'" data-cls="'+g.cls+'"'+
         ' title="'+t('arrastra para fijar que hace')+' '+g.name+' '+t('aqui')+'"'+
         ' onmousedown="barraEmpieza(event,this,'+i+')">'+
         '<span class="ftrack">'+
           '<i class="ffill" style="width:'+an+'%;background:'+
             actionColor(node.actions,i)+'"></i>'+
           '<b class="fedge" style="left:'+an+'%"></b>'+
           '<em class="fnum">'+x.toFixed(1)+'</em>'+
         '</span></td>';
    });
    h+='<td>'+(vacia?'–':g.ev.toFixed(3))+'</td></tr>';
  });
  if(visto>=0) total(t('todo'));
  tb.innerHTML=h+'</tbody>';
}
function renderCombos(){
  const p=document.getElementById('comboPanel');
  const tb=document.getElementById('comboTable');
  if(!node||!node.combos||curCls<0){ p.style.display='none'; return; }
  p.style.display='';
  document.getElementById('comboTitle').textContent=
    ' - '+clsName(Math.floor(curCls/13),curCls%13);
  let h='<thead><tr><th>combo</th><th>made hand</th><th>eq%</th><th>weight</th>';
  node.actions.forEach(a=>{h+='<th>'+a.code+' %</th><th>EV</th>';});
  h+='<th>EV(node)</th>';
  // Lo que gana quien juega para explotar la solucion, y con que accion. Si la
  // solucion esta bien, esto es casi cero en casi todo; donde no lo es, ahi
  // esta el agujero.
  if(verBR) h+='<th>exploit</th><th>gain</th>';
  h+='</tr></thead><tbody>';
  // Every combo now arrives with the node, so this filters rather than asking
  // the server for one class at a time.
  //
  // Y con una accion seleccionada arriba, solo los combos que la toman: es lo
  // mismo que se ha pedido al pinchar el boton, y si la tabla siguiera
  // ensenandolos todos el filtro seria a medias.
  let mostrados=0;
  combosOfClass(curCls).forEach(c=>{
    if(actFilter>=0 && (c.f[actFilter]||0) < 0.05) return;
    if(!comboIn(c)) return;
    ++mostrados;
    h+='<tr><td class="mono">'+c.name+(c.lk?' *':'')+'</td>'+
       '<td style="text-align:left;color:var(--dim)">'+c.made+'</td>'+
       '<td>'+c.eq.toFixed(1)+'</td><td>'+c.w.toFixed(2)+'</td>';
    node.actions.forEach((a,k)=>{h+='<td>'+c.f[k].toFixed(1)+'</td><td>'+c.e[k].toFixed(3)+'</td>';});
    h+='<td>'+c.ev.toFixed(3)+'</td>';
    if(verBR){
      if(c.br===undefined){ h+='<td>-</td><td>-</td>'; }
      else {
        const g=c.brg||0;
        h+='<td class="mono">'+node.actions[c.br].code+'</td>'+
           '<td style="color:'+(g>0.005?'var(--bet1)':'var(--dim)')+'">'+
           (g>=0?'+':'')+g.toFixed(4)+'</td>';
      }
    }
    h+='</tr>';
  });
  if(actFilter>=0 && mostrados===0)
    h+='<tr><td colspan="9" style="color:var(--dim)">ningún combo de esta casilla hace '+
       node.actions[actFilter].label+'</td></tr>';
  tb.innerHTML=h+'</tbody>';
}

// ------------------------------------------------------- nodelock dialog
// A panel opens over the tree and the panel IS the control: you click the
// action you want to move, you paint the hands on the grid, and you drag the
// weight. Nothing is typed. The strategy you see while you drag is the one you
// are building, not the one that came out of the solve -- so you can see what
// you are doing before committing to it.
// `pick` holds combo NAMES, not class indices. A class is twelve different
// hands and they do not play alike -- AJo with the heart on a heart board is a
// different hand from AJo without it -- so choosing has to reach that far down
// or the weight you set is a weight on an average nobody holds.
// `s1`/`s2` are the two suit rows: which suits of the high and the low card
// come in when you paint a square.
// SUITS already exists further up, and redeclaring it killed the whole
// script -- which is what a button that does nothing looks like.
const LKGLYPH = { c:'♣', d:'♦', h:'♥', s:'♠' };
const LKSUITCOL = { c:'#3ba55d', d:'#4a7dff', h:'#e05a5a', s:'#cfd6e4' };
let lk = { open:false, act:0, edit:{}, painting:false, strokeW:1,
           cls:-1, s1:new Set(), s2:new Set(), cells:[] };

// A combo name is two cards, "JhAc", written low card first: rank, suit, rank,
// suit. So the HIGH card's suit is name[3] and the LOW card's is name[1] --
// which is what the two rows refer to, top and bottom.
function lkHighSuit(name){ return name[3]; }
function lkLowSuit(name){ return name[1]; }
// Nothing ticked means everything: a filter with no filter in it does not
// filter. Ticking narrows it, and the two rows are POSITIONAL -- the top row is
// the high card's suit and the bottom is the low card's. That is the difference
// between picking a suited hand and picking one particular offsuit combo:
// hearts on top and spades below is A(h)J(s) and nothing else, where matching
// in either order would also have taken A(s)J(h).
function lkRowHas(set, su){ return set.size===0 || set.has(su); }
function lkPasses(c){
  return lkRowHas(lk.s1, lkHighSuit(c.name)) && lkRowHas(lk.s2, lkLowSuit(c.name));
}
function lkClassCombos(cls){ return lkCombos().filter(c=>c.cls===cls); }

function lkCombos(){ return (node&&node.combos)?node.combos:[]; }
// The frequencies a combo is currently showing in the dialog: the edited ones
// if it has been touched, otherwise what the solve produced.
function lkOrig(c){ return node.actions.map((a,k)=>c.f[k]/100); }
function lkFreq(c){
  const e=lk.edit[c.name];
  if(e) return e;
  return lkOrig(c);
}
// What the grid and the headers are drawn from. Flipping between the two is
// how you see whether an edit did anything -- the reference calls it original vs
// computed strategy, and it is the answer to "I cannot tell if it took".
function lkShown(c){
  const r=document.querySelector('input[name=lkview]:checked');
  return (r && r.value==='orig') ? lkOrig(c) : lkFreq(c);
}
function lkWhyNot(){
  if(!state||!state.solved) return 'Todavia no hay estrategia: pulsa <b>Solve</b> y vuelve.';
  if(!node) return 'Elige un punto del <b>árbol de decisiones</b> (a la derecha) antes de abrir esto.';
  if(!node.actions||!node.actions.length)
    return t('Ese punto no es una decision: es un reparto de carta o un final de mano. ')+
           t('Elige uno donde alguien tenga que actuar.');
  if(!lkCombos().length) return t('Ese nodo no tiene combos que leer.');
  return '';
}
// Always opens. Refusing into the status line was indistinguishable from a
// button that does nothing, which is exactly how it read.
function openLockDlg(){
  lk.open=true; lk.act=0; lk.edit={}; lk.cls=-1;
  lk.s1=new Set(); lk.s2=new Set();
  const why=lkWhyNot();
  document.getElementById('lockDlg').classList.add('on');
  document.getElementById('lockBody').style.display = why?'none':'';
  document.getElementById('lockWhy').style.display  = why?'':'none';
  document.getElementById('lockWhy').innerHTML = why;
  document.getElementById('lockWhere').textContent = why ? ''
    : (' - '+node.ctxLabel+' '+node.path+'  ('+(node.player===0?'OOP':'IP')+')');
  if(!why){ lk.cells=[]; renderLockDlg(); }
}
function closeLockDlg(){
  lk.open=false;
  document.getElementById('lockDlg').classList.remove('on');
}
// Weighted combo count and share for one action, over what is on screen now.
function lkTotals(){
  const A=node.actions.length, tot=new Array(A).fill(0);
  let wsum=0;
  for(const c of lkCombos()){
    const f=lkFreq(c);
    wsum+=c.w;
    for(let k=0;k<A;k++) tot[k]+=c.w*f[k];
  }
  return {tot:tot,wsum:wsum};
}
// The grid is built ONCE and then updated in place. Rebuilding it on every
// brush stroke destroyed the square under the cursor mid-drag, so the browser
// had nothing left to send mouseenter or mouseup to -- which is why painting
// only took after clicking away and back. The cells are kept in lk.cells.
function lkBuildGrid(){
  const g=document.getElementById('lockGrid');
  g.innerHTML='';
  lk.cells=[];
  for(let i=0;i<13;i++)for(let j=0;j<13;j++){
    const idx=i*13+j;
    const c=document.createElement('div');
    c.className='cell';
    c.innerHTML='<div class="fill"></div><div class="pickwash" style="opacity:0"></div>'+
      '<div class="editmark" style="display:none"></div>'+
      '<div class="lab">'+clsName(i,j)+'<div class="wt"></div></div>';
    c.onmousedown=(ev)=>{ev.preventDefault();lk.painting=true;lkPaintStart(idx);};
    c.onmouseenter=()=>{ if(lk.painting) lkPaint(idx); };
    c.onmousemove=(ev)=>showHoverCard(idx,ev);
    c.onmouseleave=hideHoverCard;
    g.appendChild(c);
    lk.cells.push(c);
  }
}

// Everything a stroke changes, written onto the cells that already exist.
function lkRefreshGrid(){
  if(!lk.cells || lk.cells.length!==169) lkBuildGrid();
  const A=node.actions.length;
  const byCls={};
  for(const c of lkCombos()){
    const f=lkShown(c);
    if(!byCls[c.cls]) byCls[c.cls]={w:0,f:new Array(A).fill(0),n:0,ed:0};
    byCls[c.cls].w+=c.w;
    byCls[c.cls].n++;
    if(lk.edit[c.name]) byCls[c.cls].ed++;
    for(let k=0;k<A;k++) byCls[c.cls].f[k]+=c.w*f[k];
  }
  for(let idx=0;idx<169;idx++){
    const cell=lk.cells[idx], agg=byCls[idx];
    const i=Math.floor(idx/13), j=idx%13;
    const fill=cell.querySelector('.fill');
    const wash=cell.querySelector('.pickwash');
    const mark=cell.querySelector('.editmark');
    const wt=cell.querySelector('.wt');
    if(!agg||agg.w<=0){
      fill.style.background='transparent';
      cell.style.opacity='.3';
      cell.style.pointerEvents='none';
      wash.style.opacity='0'; mark.style.display='none'; wt.textContent='';
      cell.title='';
      continue;
    }
    cell.style.opacity=''; cell.style.pointerEvents='';
    const wcls=lkClassW(idx);
    const col=actionColor(node.actions,lk.act);
    const pc=(100*wcls).toFixed(1);
    fill.style.background = wcls>0.001
      ? 'linear-gradient(to top,'+col+' 0%,'+col+' '+pc+'%,transparent '+pc+'%)'
      : 'transparent';
    wash.style.opacity='0';
    mark.style.display = agg.ed ? '' : 'none';
    wt.textContent=(100*wcls).toFixed(0);
    cell.title=clsName(i,j)+'  '+node.actions.map((a,k)=>
      a.code+' '+(100*agg.f[k]/agg.w).toFixed(0)+'%').join('  ')+
      (agg.ed?('   ·  '+agg.ed+' de '+agg.n+' cambiados'):'');
  }
}

// The parts around the grid: the action headers, the brush, the suit rows and
// the combo strip. Cheap to redo, and none of it is under the mouse mid-drag.
function lkRefreshChrome(){
  const T=lkTotals();
  const el=document.getElementById('lockActs');
  el.innerHTML='';
  node.actions.forEach((a,k)=>{
    const d=document.createElement('div');
    d.className='lkact'+(k===lk.act?' on':'');
    d.style.background=actionColor(node.actions,k);
    const pct=T.wsum>0?(100*T.tot[k]/T.wsum):0;
    d.innerHTML=a.label+'<small>'+T.tot[k].toFixed(1)+' combos · '+
                pct.toFixed(2)+'%</small>';
    // On mouseup, not click: a click needs the element to survive from press to
    // release, and anything that redraws in between loses it.
    d.onmouseup=()=>{lk.painting=false;lk.act=k;renderLockDlg();};
    el.appendChild(d);
  });
  document.getElementById('lockActName').textContent=node.actions[lk.act].label;
  document.getElementById('lockWval').textContent=
    document.getElementById('lockSlider').value+'%';
  lockPreview();
  const nt=Object.keys(lk.edit).length;
  document.getElementById('lockTouched').innerHTML =
    '<b>' + nt + '</b>' + t(' combo(s) cambiados') + (nt ? t(' (sin fijar todavia)') : '');

  // The two suit rows: which suit of each card comes in when a square is
  // painted. Nothing is applied through them -- they only decide what a stroke
  // reaches, which is the difference between "AJo" and "the AJo with the heart".
  const su=document.getElementById('lockSuits');
  const order=['c','d','h','s'];      // as every solver prints them, left to right
  su.innerHTML=[1,2].map(row=>'<div class="lksuits">'+order.map(x=>
     '<div class="lksuit'+((row===1?lk.s1:lk.s2).has(x)?' on':'')+
     '" style="color:'+LKSUITCOL[x]+'" onclick="lkToggleSuit('+row+',\''+x+'\')">'+
     LKGLYPH[x]+'</div>').join('')+'</div>').join('');

  const cb=document.getElementById('lockCombos');
  const list=(lk.cls>=0)?lkClassCombos(lk.cls):[];
  cb.innerHTML = list.length
    ? '<div class="lkcmbs">'+list.map(c=>{
        const f=lkShown(c);
        const bars=node.actions.map((a,k)=>
          '<i style="width:'+(100*f[k]).toFixed(1)+'%;background:'+
          actionColor(node.actions,k)+'"></i>').join('');
        const pct=(100*f[lk.act]).toFixed(0);
        return '<div class="lkcmb'+(lk.edit[c.name]?' ed':'')+'" title="'+
               node.actions.map((a,k)=>a.code+' '+(100*f[k]).toFixed(0)+'%').join('  ')+'">'+
               '<span onclick="lkPaintOne(\''+c.name+'\')" style="cursor:pointer" '+
               'title="pintar el peso de la brocha en esta mano">'+
               cardsHTML(c.name)+'</span>'+
               '<span class="mini" onclick="lkPaintOne(\''+c.name+'\')" '+
               'style="cursor:pointer">'+bars+'</span>'+
               '<span class="step" onclick="lkStepOne(\''+c.name+'\',-5)">−</span>'+
               '<span class="lkpct">'+pct+'%</span>'+
               '<span class="step" onclick="lkStepOne(\''+c.name+'\',5)">+</span>'+
               '</div>';
      }).join('')+'</div>'
    : '<div class="lkcmbs"><span style="color:var(--dim)">pincha una casilla de la rejilla</span></div>';
}

function renderLockDlg(){
  if(!lk.open) return;
  lkRefreshChrome();
  lkRefreshGrid();
}

document.addEventListener('mouseup',()=>{lk.painting=false;});
// Releasing outside the window, or the window losing focus, has to stop the
// brush too -- otherwise the next pass over the grid paints without a press.
window.addEventListener('blur',()=>{lk.painting=false;});
document.addEventListener('mouseleave',()=>{lk.painting=false;});
// The brush weight, which is the only number in the dialog that matters.
function lkW(){ return parseFloat(document.getElementById('lockSlider').value||'0')/100; }
function lkSetW(v){
  document.getElementById('lockSlider').value=v;
  lockPreview();
}
// Put a weight on one combo for the selected action; the rest of its mix takes
// or gives up the difference in proportion to what each already had. Every
// path in this dialog ends up here, which is why the arithmetic lives in one
// place and matches what the console and the API do.
function lkSetCombo(c, w){
  const A=node.actions.length, ac=lk.act;
  const f=lkFreq(c).slice();
  let want=(lkMode()==='scale') ? f[ac]*w : w;
  if(want<0) want=0; if(want>1) want=1;
  const moved=want-f[ac];
  let rest=0;
  for(let k=0;k<A;k++) if(k!==ac) rest+=f[k];
  f[ac]=want;
  for(let k=0;k<A;k++){
    if(k===ac) continue;
    f[k]-=(rest>1e-12)?moved*f[k]/rest:moved/(A-1);
    if(f[k]<0) f[k]=0;
  }
  let sum=0; for(let k=0;k<A;k++) sum+=f[k];
  if(sum>1e-12) for(let k=0;k<A;k++) f[k]/=sum;
  lk.edit[c.name]=f;
}
// Clicking a square paints every combo in it. That is what everyone does, and it is
// why there is no Apply button: painting IS the change.
//
// Clicking one that already carries the brush weight takes it back off instead,
// so the same click that adds a hand removes it. The decision is made once, on
// the press, and the whole drag uses it -- otherwise sweeping across a row would
// flip each square against whatever its neighbour just became.
function lkAlready(w, have){ return Math.abs(have - w) < 0.005; }
function lkPaintStart(idx){
  const w=lkW();
  lk.strokeW = (w > 0.0001 && lkAlready(w, lkClassW(idx))) ? 0 : w;
  lkPaint(idx);
}
function lkPaint(idx){
  lk.cls=idx;
  for(const c of lkClassCombos(idx)){
    if(!lkPasses(c)) continue;
    lkSetCombo(c, lk.strokeW);
  }
  const r=document.querySelector('input[name=lkview][value=edited]');
  if(r) r.checked=true;
  lkRefreshChrome();
  lkRefreshGrid();
}
function lkPaintAll(){
  lk.strokeW=lkW();
  const w=lk.strokeW;
  let k=0;
  for(const c of lkCombos()){ if(!lkPasses(c)) continue; lkSetCombo(c, w); k++; }
  status(k+' combo(s) pintados','');
  renderLockDlg();
}
// The weight a square is carrying now, for the selected action: the weighted
// average of its combos, which is the number the reference prints in the cell.
function lkClassW(idx){
  const list=lkClassCombos(idx);
  let wsum=0, acc=0;
  for(const c of list){ const f=lkShown(c); wsum+=c.w; acc+=c.w*f[lk.act]; }
  return wsum>0 ? acc/wsum : 0;
}
function lkMode(){
  const r=document.querySelector('input[name=lkmode]:checked');
  return r?r.value:'fixed';
}
function lkToggleSuit(row,su){
  const set = row===1?lk.s1:lk.s2;
  if(set.has(su)) set.delete(su); else set.add(su);
  renderLockDlg();
}
// One hand in the strip, and the same toggle: a second click takes it off.
function lkPaintOne(name){
  const c=lkCombos().find(x=>x.name===name);
  if(!c) return;
  const w=lkW();
  const have=lkFreq(c)[lk.act];
  lkSetCombo(c, (w > 0.0001 && lkAlready(w, have)) ? 0 : w);
  const r=document.querySelector('input[name=lkview][value=edited]');
  if(r) r.checked=true;
  renderLockDlg();
}
// ...and the two arrows nudge it five points at a time, for the fine work.
function lkStepOne(name, pts){
  const c=lkCombos().find(x=>x.name===name);
  if(!c) return;
  const A=node.actions.length, ac=lk.act;
  const f=lkFreq(c).slice();
  let want=f[ac]+pts/100;
  if(want<0) want=0; if(want>1) want=1;
  const moved=want-f[ac];
  let rest=0;
  for(let k=0;k<A;k++) if(k!==ac) rest+=f[k];
  f[ac]=want;
  for(let k=0;k<A;k++){
    if(k===ac) continue;
    f[k]-=(rest>1e-12)?moved*f[k]/rest:moved/(A-1);
    if(f[k]<0) f[k]=0;
  }
  let sum=0; for(let k=0;k<A;k++) sum+=f[k];
  if(sum>1e-12) for(let k=0;k<A;k++) f[k]/=sum;
  lk.edit[name]=f;
  // Showing the original while editing would look like nothing happened.
  const r=document.querySelector('input[name=lkview][value=edited]');
  if(r) r.checked=true;
  renderLockDlg();
}
// The brush weight changes nothing on screen except its own readout: the grid
// shows the weight each hand HAS, not the one the mouse would paint. Redrawing
// all 169 squares on every pixel of the drag -- each one re-summing every combo
// in the range -- is what made the bar feel broken.
function lockPreview(){
  const v=document.getElementById('lockSlider');
  const out=document.getElementById('lockWval');
  if(v&&out) out.textContent=v.value+'%';
  const nt=document.getElementById('lockModeNote');
  if(nt) nt.textContent = (lkMode()==='fixed')
      ? t('Fijo: esa acción pasa a ser exactamente ese porcentaje, y el resto se reparte lo que queda en proporcion a lo que ya tenía.')
      : t('Proporcional: esa acción se multiplica por ese porcentaje (50% = la mitad de lo que hacia), y el resto recoge la diferencia en proporcion.');
}
// The move itself, and the same arithmetic the console and the API do: the
// target action goes where you put it, and the rest give up or take the
// difference in proportion to what each already had.
// Locking the range exactly as it came out. Everything that is not moved
// afterwards stays where the solve put it, which is the "lock all hands" of
// the usual dialog and the starting point for editing a whole range by hand.
// Clava el rango donde esta, sin cambiarlo.
//
// Usa lkFreq y NO lkOrig, y ahi esta la diferencia: lkFreq devuelve lo que has
// pintado si has pintado, y la frecuencia del solve si no. O sea que se puede
// pintar cuatro manos y luego darle a esto para clavar las demas. Con lkOrig
// borraria lo pintado, que es justo lo contrario de lo que dice el boton.
function lockFreezeAll(){
  let k=0;
  for(const c of lkCombos()){ lk.edit[c.name]=lkFreq(c).slice(); k++; }
  status(k+t(' combo(s) clavados donde estaban -- todavia sin fijar'),'');
  renderLockDlg();
}
function lockUndoAll(){ lk.edit={}; renderLockDlg(); status(t('cambios descartados'),''); }
// Only what was actually touched is sent, so hands you did not edit stay free
// for the solver -- which is what "lock selected combos" means.
async function lockCommit(){
  const names=Object.keys(lk.edit);
  if(!names.length){ status(t('no has cambiado nada'),'err'); return; }
  const parts=names.map(nm=>nm+':'+node.actions.map((a,k)=>
    a.code[0]+'='+lk.edit[nm][k].toFixed(6)).join(','));
  const r=await api('/api/lock',{ctx:nav.ctx,node:nav.node,bulk:parts.join(';')});
  takeState(r,false);
  status(r.note,r.ok?'':'err');
  if(r.ok) closeLockDlg();
}

// ---------------------------------------------------------------- locks
// La tira de locks, en UNA linea.
//
// Antes listaba todos. Un nodelock de verdad son sesenta y pico manos, asi que
// la tira se convertia en un muro de "R R JcJd -> Check=1" que ocupaba media
// pantalla y no se leia: nadie repasa sesenta lineas iguales para enterarse de
// nada. Lo que hace falta saber de un vistazo es cuantos hay y donde, y eso
// cabe en una linea. El detalle queda a un clic, y aun ahi se corta.
let verLocks=false;
function toggleVerLocks(){ verLocks=!verLocks; renderLocks(); }
// La convergencia, arriba y con color.
//
// La explotabilidad ya salia, pero como un numero mas en una linea densa del
// panel de EV. Al 211% del bote eso no es un dato, es un aviso, y se leia igual
// que el 0,2%. Un arbol cargado a medias enseña tablas quietas con toda la pinta
// de ser una solucion, y nada en pantalla dice lo contrario.
//
// Los escalones salen de lo que se dijo al construir esto: del 2 al 5 por ciento
// una solucion ya sirve como guia, que es para lo que se usa. Por debajo del 0,5
// ya no se discute; por encima del 20 no hay estrategia que mirar.
function bandaConv(x){
  if(x < 0.5)  return ['cv0',t('convergida')];
  if(x < 2)    return ['cv1',t('buena')];
  if(x < 5)    return ['cv2',t('utilizable')];
  if(x < 20)   return ['cv3',t('floja')];
  return ['cv4',t('sin converger')];
}
function renderConv(x, resolviendo){
  const el=document.getElementById('convPill');
  if(!el) return;
  if(typeof x!=='number' || x<0){
    el.className='pill';
    el.textContent=t('sin resolver');
    el.title=t('No hay solución todavía: dale a Resolver.');
    return;
  }
  const [cls,pal]=bandaConv(x);
  el.className='pill '+cls;
  // Con locks puestos y sin resolver, este numero es el de ANTES de ellos. La
  // estrategia que se ve tambien, y eso ya lo dice la tira de locks; lo que no
  // puede quedarse callado es la explotabilidad, que es la que se lee como
  // "esto ya esta".
  const pend = state && state.locksPending;
  el.textContent=(resolviendo?'· ':'')+num(x,x<1?3:2)+'% '+pal+
                 (pend?t('  · locks sin aplicar'):'');
  const nl=String.fromCharCode(10);
  el.title=t('Explotabilidad: lo que le sacaria a esta estrategia un rival que la ')+
    t('jugara perfectamente, en % del bote por mano. Cuanto más bajo, más cerca ')+
    t('del equilibrio.')+nl+nl+
    (state && state.locksPending
       ? t('Hay locks puestos que este numero todavia no lleva: es el de la ')+
         t('solucion anterior. Dale a Resolver.')+nl+nl : '')+
    t('menos de 0.5%  convergida')+nl+
    t('0.5 a 2%       buena')+nl+
    t('2 a 5%         utilizable, que es para lo que se usa')+nl+
    t('5 a 20%        floja: mirala con cuidado')+nl+
    t('más de 20%     sin converger, no es una estrategia');
}

function renderLocks(){
  pintaSetupBtn();
  const el=document.getElementById('lockStrip');
  if(!el) return;
  const L=(state && state.locks) ? state.locks : [];
  if(!L.length){
    el.className='lockstrip';
    el.innerHTML=t('sin locks');
    verLocks=false;
    return;
  }
  const nodos=new Set(L.map(l=>l.ctx+' '+l.node)).size;
  el.className='lockstrip has';
  let h='<b>'+L.length+(L.length===1?' lock':' locks')+'</b>'+
        '<span>'+t('en')+' '+nodos+' '+(nodos===1?t('nodo'):t('nodos'))+'</span>'+
        '<button class="sm needsIdle" onclick="doUnlock()">Quitar los de este nodo</button>'+
        '<button class="sm needsIdle" onclick="doUnlockAll()">Quitar todos</button>'+
        '<a href="#" onclick="toggleVerLocks();return false">'+
        t(verLocks?'ocultar':'ver cuales')+'</a>'+
        '<span>se aplican en el siguiente solve; cambiar de board los tira</span>';
  if(verLocks){
    const tope=Math.min(L.length,20);
    h+='<div class="lockdet">'+
       L.slice(0,tope).map(l=>'<span class="mono">'+l.ctx+' '+l.node+' '+l.hands+
                              ' → '+l.mix+'</span>').join('')+
       (L.length>tope?'<span>y '+(L.length-tope)+' más</span>':'')+'</div>';
  }
  el.innerHTML=h;
}
async function doUnlock(){
  const r=await api('/api/unlock',{ctx:nav.ctx,node:nav.node});
  takeState(r,false);
  status(r.note,r.ok?'':'err');
}
async function doUnlockAll(){
  if(state.locks.length && !confirm('Remove all '+state.locks.length+' lock(s)?')) return;
  const r=await api('/api/unlock',{all:1});
  takeState(r,false);
  status(r.note,r.ok?'':'err');
}

// ---------------------------------------------------------------- jugarlo
//
// El entrenador. La pagina no sabe jugar: pregunta al motor y pinta lo que le
// conteste. Cada respuesta es la mesa ENTERA -- board, cartas, bote, acciones,
// historial, marcador -- asi que aqui no hay estado que mantener al dia y no
// hay forma de que la pantalla y el motor se cuenten cosas distintas.
//
// El consejo solo viaja cuando esta encendido: no se manda escondido para
// taparlo con CSS, porque eso no lo esconde de nadie que abra el inspector.
let tr=null;
let boardAntes=[];

function trainOpen(){
  document.body.classList.add('playing');
  trainSend('state').then(()=>{ if(tr && !tr.on) trainNew(); });
}
function trainClose(){
  document.body.classList.remove('playing');
  trainSend('stop');
}
function trainNew(){ trainSend('new'); }
function trainRepeat(){ trainSend('repeat'); }
function trainReset(){ trainHist=[]; trainSend('reset').then(()=>trainSend('new')); }
function trainAdvice(){
  trainSend('state',{advice:document.getElementById('tAdvice').checked?'on':'off'});
}
function trainAct(code){ return trainSend('act',{action:code}); }


// ------------------------------------------------ donde empieza la mano
//
// El arbol ya esta entero en `state.tree`: contextos, nodos, acciones y a que
// nodo lleva cada una. Asi que esto no le pregunta nada al motor hasta que
// eliges: se baja por lo que ya hay en la pagina.
let pickCtx=0, pickNode=-1, pickPasos=[];

function startPick(){
  if(!state || !state.tree || !state.tree.length){
    status(t('resuelve primero'),'err'); return;
  }
  if(tr && !tr.atRoot && typeof tr.startCtx==='number'){
    pickCtx=tr.startCtx;
    pickNode=(tr.startNode>=0)?tr.startNode:ctxOf(pickCtx).root;
  } else { pickCtx=0; pickNode=ctxOf(0).root; }
  pickPasos=[];
  document.getElementById('startDlg').classList.add('on');
  startRender();
}
function startClose(){ document.getElementById('startDlg').classList.remove('on'); }
function startRoot(){
  pickCtx=0; pickNode=ctxOf(0).root; pickPasos=[];
  trainSend('new',{startCtx:'-1'}).then(()=>{ startClose(); });
}
function startUse(){
  trainSend('new',{startCtx:String(pickCtx),startNode:String(pickNode)})
    .then(()=>{ startClose(); });
}
function startAtras(k){
  const p=pickPasos[k];
  if(!p) return;
  pickCtx=p.ctx; pickNode=p.node; pickPasos=pickPasos.slice(0,k);
  startRender();
}
function startBaja(ctx,node,etiqueta){
  pickPasos.push({ctx:pickCtx,node:pickNode,label:etiqueta});
  pickCtx=ctx; pickNode=node;
  startRender();
}

function startRender(){
  const c=ctxOf(pickCtx), n=c.nodes[pickNode];
  const camino=document.getElementById('startPath');
  let h='<b onclick="startAtras(0)">'+t('el principio')+'</b>';
  pickPasos.forEach((p,i)=>{
    if(i===0) return;
    h+='<span>›</span><b onclick="startAtras('+i+')">'+p.label+'</b>';
  });
  if(pickPasos.length) h+='<span>›</span><b>'+
     (pickPasos[pickPasos.length-1].label||'')+'</b>';
  h+='<span style="margin-left:auto">'+t(c.streetName)+' · '+t('bote')+' '+num(n.pot,1)+'</span>';
  camino.innerHTML=h;

  const ac=document.getElementById('startActs');
  const usa=document.getElementById('startUse');
  ac.innerHTML='';
  if(n.type===NT_CONT){
    const b=document.createElement('button');
    b.className='sm';
    b.textContent=t('sale una carta')+' → '+t(ctxOf(n.contCtx).streetName);
    b.onclick=()=>startBaja(n.contCtx, ctxOf(n.contCtx).root, t('carta'));
    ac.appendChild(b);
    usa.disabled=false;
  } else if(n.type===NT_DECISION){
    (n.actions||[]).forEach(a=>{
      const b=document.createElement('button');
      b.className='sm';
      b.textContent=(n.player===0?'OOP ':'IP ')+t(a.label);
      b.onclick=()=>startBaja(pickCtx, a.child, (n.player===0?'OOP ':'IP ')+a.label);
      ac.appendChild(b);
    });
    usa.disabled=false;
  } else {
    ac.innerHTML='<span class="advoff">'+t('aquí se acaba la mano: no hay nada que jugar')+'</span>';
    usa.disabled=true;
  }
}

let trainLado=0;
function trainSide(n){
  trainLado=(n===1)?1:0;
  trainSend('new',{side:String(trainLado)});
}
// Las manos jugadas, de esta sesion. Viven en la pagina: el motor juega una
// mano cada vez y no tiene por que llevar la cuenta de lo que ya paso.
let trainHist=[], trainAutoT=null, trainUltimo='';

// El teclado, jugando.
//
// Cincuenta manos con el raton cansan, y lo que se hace cincuenta veces se hace
// con una tecla. Solo mientras juegas, y NUNCA si estas escribiendo en un campo
// -- una semilla, un nombre -- que es como un atajo se convierte en una
// sorpresa desagradable.
document.addEventListener('keydown', ev=>{
  if(!document.body.classList.contains('playing')) return;
  if(ev.ctrlKey || ev.altKey || ev.metaKey) return;
  const el=ev.target;
  const tag=(el && el.tagName || '').toLowerCase();
  if(tag==='input' || tag==='textarea' || tag==='select' ||
     (el && el.isContentEditable)) return;
  if(!tr || !tr.on) return;
  const k=(ev.key||'').toLowerCase();

  // Mano terminada: N reparte la siguiente, R repite esta.
  if(tr.over){
    if(k==='n'){ ev.preventDefault(); trainNew(); }
    else if(k==='r'){ ev.preventDefault(); trainRepeat(); }
    return;
  }

  // El consejo se enciende y se apaga sin soltar el teclado.
  if(k==='a'){
    ev.preventDefault();
    const c=document.getElementById('tAdvice');
    c.checked=!c.checked;
    trainAdvice();
    return;
  }

  const acciones=tr.actions||[];
  if(!acciones.length) return;
  let idx=-1;
  if(k==='f' || k==='x' || k==='c'){
    idx=acciones.findIndex(a=>(a.code||'')[0].toLowerCase()===k);
  } else if(k>='1' && k<='9'){
    // El numero cuenta apuestas y subidas, igual que lo que pone el boton.
    const n=parseInt(k,10);
    let visto=0;
    for(let j=0;j<acciones.length;j++){
      if(!apuestaDe(acciones[j].code)) continue;
      if(++visto===n){ idx=j; break; }
    }
  }
  if(idx<0) return;
  ev.preventDefault();
  trainAct(acciones[idx].code);
});

// La mano que acaba de terminar, en una linea. Se queda a la vista mientras
// juegas la siguiente, que es lo unico que sirve cuando van solas.
function trainResumenMano(){
  const h=trainHist[0];
  if(!h) return '';
  const nota={v0:t('PERFECTA'),v1:t('MUY BIEN'),v2:t('BIEN'),
              v3:t('MEJORABLE'),v4:t('CARA')}[h.cls]||'';
  return '<span class="mk '+h.cls+'"></span>'+
         t('mano')+' <b>'+h.hand+'</b> · <b>'+nota+'</b> · '+
         (h.perdido<=1e-9 ? t('sin perder EV')
                          : '-'+num(h.perdido,2)+' '+t('de EV'))+
         ' · '+(h.res>=0?'+':'')+num(h.res,1)+' '+t('fichas');
}

function trainApunta(){
  if(!tr || !tr.over || !tr.seed) return;
  if(trainHist.length && trainHist[0].seed===tr.seed &&
     trainHist[0].hand===tr.hand) return;          // ya apuntada
  let perdido=0;
  (tr.steps||[]).forEach(p=>{ perdido+=(p.loss||0); });
  const bote=(tr.pot0>0)?tr.pot0:20;
  const pct=100*perdido/bote;
  let cls='v0';
  if(perdido>1e-9) cls=(pct<0.5)?'v1':(pct<2)?'v2':(pct<5)?'v3':'v4';
  trainHist.unshift({hand:tr.hand, seed:tr.seed, side:tr.side,
                     startCtx:(tr.atRoot?-1:tr.startCtx),
                     startNode:(tr.atRoot?-1:tr.startNode),
                     cards:(tr.hero||[]).join(' '), res:tr.result||0,
                     perdido:perdido, cls:cls});
  if(trainHist.length>50) trainHist.pop();
}

function trainPintaHist(){
  const caja=document.getElementById('tHistBox');
  const el=document.getElementById('tHist');
  if(!trainHist.length){ caja.style.display='none'; return; }
  caja.style.display='';
  el.innerHTML=trainHist.map((h,i)=>
    '<div class="hrow" onclick="trainRejuega('+i+')" title="'+t('volver a jugarla')+'">'+
      '<span class="n">'+h.hand+'</span>'+
      '<span class="cc">'+h.cards+'</span>'+
      '<span class="res '+(h.res>=0?'up':'dn')+'">'+(h.res>=0?'+':'')+num(h.res,1)+'</span>'+
      '<span class="mk '+h.cls+'" title="'+(h.perdido<=1e-9?t('perfecta'):
          ('-'+num(h.perdido,2)+' '+t('de EV')))+'"></span>'+
    '</div>').join('');
}

function trainRejuega(i){
  const h=trainHist[i];
  if(!h) return;
  const f={seed:String(h.seed), side:String(h.side)};
  if(h.startCtx>=0){ f.startCtx=String(h.startCtx); f.startNode=String(h.startNode); }
  else f.startCtx='-1';
  trainSend('new',f);
}

function trainSend(op,extra){
  // Cualquier cosa que hagas cancela la cuenta atras: si le das a un boton es
  // que no quieres esperar.
  if(trainAutoT){ clearTimeout(trainAutoT); trainAutoT=null; }
  const f=Object.assign({op:op}, extra||{});
  if(op==='new'||op==='repeat'||op==='state') f.side=String(trainLado);
  return api('/api/train',f).then(r=>{
    if(!r || r.ok===false){
      const m=document.getElementById('tMsg');
      m.textContent=(r&&(r.error||r.note))||t('no se pudo');
      m.style.display='';
      status(m.textContent,'err');
      return;
    }
    document.getElementById('tMsg').style.display='none';
    tr=r; renderTrain();
  });
}

function tcard(txt,cls){
  const d=document.createElement('div');
  if(!txt || txt==='?'){ d.className='tcard back '+(cls||''); d.textContent='?'; return d; }
  d.className='tcard '+txt[1]+' '+(cls||'');
  d.innerHTML='<span>'+txt[0]+'</span><span class="su">'+SUIT_GLYPH[txt[1]]+'</span>';
  return d;
}

function renderTrain(){
  if(!tr) return;
  const set=(id,v)=>{ const e=document.getElementById(id); if(e) e.textContent=v; };

  set('tHand', tr.hand||'-');
  set('tSeed', tr.seed||'-');
  document.getElementById('tAdvice').checked=!!tr.advice;
  trainLado=(tr.side===1)?1:0;
  document.getElementById('tSideO').className='sm'+(tr.side===0?' act':'');
  document.getElementById('tSideI').className='sm'+(tr.side===1?' act':'');
  const sb=document.getElementById('tStartBtn');
  if(sb) sb.textContent=t('Empezar en: ')+(tr.atRoot? t('el principio') : (tr.startLine||''));

  // Los dos asientos. El de arriba es el rival, y sus cartas solo se ven
  // cuando la mano ha terminado en showdown.
  const yoOOP = (tr.side===0);
  set('tHeroWho', (yoOOP?'OOP':'IP')+(tr.you?' · '+tr.you:''));
  set('tVillWho', yoOOP?'IP':'OOP');
  set('tHeroStack', num(tr.heroStack||0,0));
  set('tVillStack', num(tr.villStack||0,0));
  // A quien le toca, marcado: en una mesa lo dice el boton que parpadea.
  const meToca=(tr.on && !tr.over && (tr.actions||[]).length>0);
  document.getElementById('tHeroPlate').className='chapa'+(meToca?' act':'');
  document.getElementById('tVillPlate').className='chapa';
  // El boton lo lleva IP, que en heads-up postflop es quien actua el ultimo.
  document.getElementById('tHeroD').style.display=yoOOP?'none':'';
  document.getElementById('tVillD').style.display=yoOOP?'':'none';
  // Las fichas de delante.
  // Una pila de fichas: una por cada trozo del bote, hasta cinco. Es lo que se
  // ve de reojo -- "ha metido mucho" -- antes de leer el numero.
  const fichas=(el,v)=>{
    const e=document.getElementById(el);
    if(!(v>0)){ e.innerHTML=''; return; }
    const cuantas=Math.max(1,Math.min(5,Math.round(v/Math.max((tr.pot0||20)/4,1))));
    let p='';
    for(let i=0;i<cuantas;i++) p+='<i></i>';
    e.innerHTML='<span class="pila">'+p+'</span><span class="amt">'+num(v,1)+'</span>';
  };
  fichas('tHeroFront', tr.heroFront||0);
  fichas('tVillFront', tr.villFront||0);

  const vh=document.getElementById('tVill'); vh.innerHTML='';
  if(tr.villain){ tr.villain.forEach(c=>vh.appendChild(tcard(c))); }
  else { vh.appendChild(tcard(null)); vh.appendChild(tcard(null)); }

  const hh=document.getElementById('tHero'); hh.innerHTML='';
  (tr.hero||[]).forEach(c=>hh.appendChild(tcard(c)));

  // El board, con hueco para las cartas que faltan: se ve de un vistazo en que
  // calle estas sin leer nada.
  // Las cartas que no estaban antes caen girando; las que ya estaban, quietas.
  const bd=document.getElementById('tBoard');
  const cartas=tr.board||[];
  const nuevas=(cartas.length>boardAntes.length &&
                boardAntes.every((c,i)=>c===cartas[i])) ? boardAntes.length : cartas.length;
  boardAntes=cartas.slice();
  bd.innerHTML='';
  cartas.forEach((c,i)=>bd.appendChild(tcard(c, i>=nuevas?'nueva':'')));
  for(let i=cartas.length;i<5;i++){
    const g=document.createElement('div'); g.className='gap'; bd.appendChild(g);
  }
  set('tPot', num((typeof tr.potMid==='number'?tr.potMid:tr.pot),1));

  // La situacion.
  set('tsStreet', tr.street? t(tr.street) : '-');
  set('tsSide', yoOOP?'OOP':'IP');
  set('tsPot', num(tr.pot,1));
  set('tsCall', (!tr.over && tr.toCall>0)? num(tr.toCall,1) : '-');
  // Stack entre bote: lo que decide si una pareja floja juega por el bote
  // o por una apuesta. Los stacks de cada uno estan en su chapa.
  const mid=(typeof tr.potMid==='number'&&tr.potMid>0)?tr.potMid:(tr.pot||1);
  set('tsSPR', tr.on? num((tr.heroStack||0)/mid,1) : '-');
  set('tsYou', tr.you||'-');

  // El historial.
  const lg=document.getElementById('tLog'); lg.innerHTML='';
  if(!(tr.log||[]).length){
    const li=document.createElement('li');
    li.className='vacio';
    li.textContent=t('todavía no ha pasado nada');
    lg.appendChild(li);
  }
  (tr.log||[]).forEach(l=>{
    const li=document.createElement('li');
    // El motor escribe la posicion; aqui se lee el nombre, que es lo que se ve
    // en una sala. La posicion se queda al lado, que es lo que se estudia.
    const mio=(yoOOP && l.indexOf('OOP')===0) || (!yoOOP && l.indexOf('IP')===0);
    const suyo=l.indexOf('OOP')===0 || l.indexOf('IP')===0;
    if(suyo){
      const pos=l.indexOf('OOP')===0?'OOP':'IP';
      li.textContent=(mio?'Hero':'DCFR Bot')+' ('+pos+') '+l.slice(pos.length+1);
    } else {
      li.textContent=l;
    }
    if(mio) li.className='you';
    lg.appendChild(li);
  });

  // Los botones.
  const ac=document.getElementById('tActions'); ac.innerHTML='';
  if(!tr.over && tr.on){
    (tr.actions||[]).forEach((a,i)=>{
      const b=document.createElement('button');
      b.className=(a.code[0]||'').toLowerCase();
      let h='<span class="key">'+teclaDe(a,i)+'</span><span>'+t(a.label)+'</span>';
      if(typeof a.freq==='number')
        h+='<span class="adv">'+num(100*a.freq,1)+'% · EV '+num(a.ev,2)+'</span>';
      b.innerHTML=h;
      b.onclick=()=>trainAct(a.code);
      ac.appendChild(b);
    });
  }

  // El consejo del lateral.
  const ad=document.getElementById('tAdv');
  if(!tr.advice){
    ad.innerHTML='<div class="advoff">'+
      t('apagado: marca «ver el consejo» arriba para ver las frecuencias y el EV de cada acción mientras juegas')+
      '</div>';
  } else if(tr.over || !tr.actions || !tr.actions.length){
    ad.innerHTML='<div class="advoff">'+t('no te toca decidir ahora mismo')+'</div>';
  } else {
    let mejor=-1, top=0;
    tr.actions.forEach((a,i)=>{ if(typeof a.ev==='number' && (mejor<0||a.ev>top)){mejor=i;top=a.ev;} });
    ad.innerHTML=tr.actions.map((a,i)=>
      '<div class="advrow'+(i===mejor?' best':'')+'">'+
        '<span class="bar"><i style="width:'+Math.round(100*Math.min(a.freq||0,1))+'%"></i>'+
        '<span>'+t(a.label)+'</span></span>'+
        '<span class="fq">'+num(100*(a.freq||0),1)+'%</span>'+
        '<span class="ev">'+num(a.ev||0,2)+'</span>'+
      '</div>').join('')+
      '<div class="advoff" style="margin-top:6px">'+
      t('EV en fichas, para TU mano exacta. Dos acciones que valen lo mismo son las dos correctas.')+'</div>';
  }

  // El final de la mano.
  const fin=document.getElementById('tEnd');
  if(tr.over){
    const g=tr.result>=0;
    fin.style.display='';
    fin.innerHTML=trainVeredicto()+
      '<div class="big '+(g?'win':'lose')+'">'+
        (g?'+':'')+num(tr.result,1)+'</div>'+
      '<div>'+(tr.showdown
        ? 'Hero <b>'+(tr.heroMade||'')+'</b> · DCFR Bot <b>'+(tr.villMade||'')+'</b>'
        : t('la mano se acabó sin showdown'))+'</div>'+
      '<div style="margin-top:8px"><button class="sm" onclick="trainNew()">'+
        t('Otra mano')+'</button> <button class="sm" onclick="trainRepeat()">'+
        t('Repetir esta mano')+'</button></div>'+
      (document.getElementById('tAuto').checked
        ? '<div class="advoff" style="margin-top:6px">'+t('siguiente mano en 2 s')+'</div>'
        : '');
  } else {
    fin.style.display='none';
  }

  // El marcador.
  const sc=tr.score||{};
  set('tmHands', sc.hands||0);
  set('tmDec', sc.decisions||0);
  set('tmLost', num(sc.lost||0,2));
  set('tmPer', (sc.decisions? num((sc.lost||0)/sc.decisions,3):'0')+
               '  ('+num(sc.lossPct||0,2)+'%)');
  set('tmWon', ((sc.won||0)>=0?'+':'')+num(sc.won||0,1));
  document.getElementById('tmGrade').innerHTML=trainGrade(sc);

  // La mano terminada se apunta, y si has pedido seguir solo, la siguiente se
  // reparte sola despues de dos segundos: los justos para leer el veredicto.
  if(tr.over){
    trainApunta();
    trainUltimo=trainResumenMano();
    const auto=document.getElementById('tAuto');
    if(auto && auto.checked && !trainAutoT){
      trainAutoT=setTimeout(()=>{ trainAutoT=null; trainNew(); }, 2200);
    }
  }
  const ult=document.getElementById('tLast');
  if(trainUltimo){ ult.style.display=''; ult.innerHTML=trainUltimo; }
  else ult.style.display='none';
  trainPintaHist();

  // El repaso, al terminar la mano.
  const rb=document.getElementById('tRevBox');
  const rv=document.getElementById('tRev');
  if(tr.over && tr.steps && tr.steps.length){
    rb.style.display='';
    rv.innerHTML='<tr><th>'+t('calle')+'</th><th>'+t('hiciste')+'</th><th>'+
      t('lo mejor')+'</th><th style="text-align:right">'+t('coste')+'</th></tr>'+
      tr.steps.map(p=>{
        const hizo=p.acts[p.chosen]||{}, mej=p.acts[p.best]||{};
        const mal=p.loss>0.01*(tr.pot0||1);
        return '<tr class="'+(mal?'bad':'good')+'">'+
          '<td>'+t(p.street)+'</td>'+
          '<td>'+t(hizo.label||'')+'</td>'+
          '<td>'+t(mej.label||'')+'</td>'+
          '<td class="n">'+(p.loss>0.0005? '-'+num(p.loss,2) : '0')+'</td></tr>';
      }).join('');
  } else {
    rb.style.display='none';
  }
}

// El veredicto de LA MANO que acaba de terminar.
//
// Lo primero que se mira al acabar una mano es si ganaste, y eso es justo lo
// que no dice nada: el bot te paga con la peor mano de su rango y liga una vez
// de cada seis. Lo que depende de ti es lo que dejaste por el camino, asi que
// eso es lo que va arriba y en grande.
//
// Jugada perfecta es perfecta: cero, sin matices. No "casi": si en cada
// decision elegiste una accion que valia tanto como la mejor, no dejaste nada,
// y decirte otra cosa seria inventarse un error para tener algo que contar.
// La tecla de cada accion.
//
// La inicial donde no hay duda -- F de fold, X de check, C de call -- y el
// numero para las apuestas y las subidas, que pueden ser varias: con dos
// tamanos, "B" no dice cual, y 1 y 2 si.
//
// Y los numeros cuentan APUESTAS, no botones: frente a una apuesta los botones
// son Fold, Call y Raise, y que la subida saliera con un 3 porque va tercera no
// lo entiende nadie. La primera apuesta o subida es el 1, siempre.
function apuestaDe(code){
  const k=(code||'')[0];
  return k!=='F' && k!=='X' && k!=='C';
}
function teclaDe(a, i){
  const k=(a.code||'')[0];
  if(k==='F') return 'F';
  if(k==='X') return 'X';
  if(k==='C') return 'C';
  let n=1;
  const l=(tr && tr.actions) ? tr.actions : [];
  for(let j=0;j<i && j<l.length;j++) if(apuestaDe(l[j].code)) n++;
  return String(n);
}

function trainVeredicto(){
  const pasos=(tr && tr.steps) ? tr.steps : [];
  if(!pasos.length)
    return '<div class="vered v1">'+t('no te tocó decidir')+'</div>';
  let perdido=0;
  pasos.forEach(p=>{ perdido += (p.loss||0); });
  const bote=(tr.pot0>0)?tr.pot0:20;
  const pct=100*perdido/bote;
  let cls='v0', txt=t('PERFECTA');
  if(perdido>1e-9){
    if(pct<0.5){ cls='v1'; txt=t('MUY BIEN'); }
    else if(pct<2){ cls='v2'; txt=t('BIEN'); }
    else if(pct<5){ cls='v3'; txt=t('MEJORABLE'); }
    else { cls='v4'; txt=t('CARA'); }
  }
  const detalle=(perdido<=1e-9)
    ? t('no dejaste nada de EV en la mesa')
    : '-'+num(perdido,2)+' '+t('de EV')+'  ·  '+num(pct,2)+t('% del bote');
  return '<div class="vered '+cls+'">'+txt+' <small>'+detalle+'</small></div>';
}

// La nota del marcador. Los escalones son los mismos que los de la
// explotabilidad, y por la misma razon: por debajo del 0,5% del bote por
// decision no hay nada que discutir, y por encima del 5% hay una fuga.
function trainGrade(sc){
  if(!sc.decisions) return '<span class="advoff">'+t('juega unas manos y aquí sale tu nota')+'</span>';
  const x=sc.lossPct||0;
  let cls='cv0', txt=t('impecable');
  if(x>=0.5){ cls='cv1'; txt=t('muy bien'); }
  if(x>=2)  { cls='cv2'; txt=t('bien'); }
  if(x>=5)  { cls='cv3'; txt=t('hay fugas'); }
  if(x>=20) { cls='cv4'; txt=t('a repasar'); }
  return '<span class="pill '+cls+'">'+txt+'</span> <span class="advoff">'+
         num(x,2)+t('% del bote por decisión')+'</span>';
}

// Si estabas jugando una mano y refrescas, sigues en la mesa.
//
// El motor lleva la mano, no la pagina: refrescando volvias al estudio con una
// mano a medias abierta por debajo, y para seguirla habia que darle a Jugar y
// adivinar que seguia ahi. Se le pregunta al arrancar y se entra si toca.
(function(){
  api('/api/train',{op:'state'}).then(r=>{
    if(r && r.ok && r.on){ document.body.classList.add('playing'); tr=r; renderTrain(); }
  }).catch(()=>{});
})();

arrancaIdioma();
aplicaSetup();
refresh();
refreshSaves();
</script>
</body>
</html>
)HTMLPAGE";
