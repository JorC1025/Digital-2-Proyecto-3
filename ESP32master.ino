//Jorge Cruz - 23502
//Rene Gonzalez - 23360

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>


/* Put your SSID & Password */
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

#define NucleoSegmentos 0x35
#define NucleoLCD 0x18

#define I2C_SDA 21
#define I2C_SCL 22
uint8_t error=0;
uint8_t comando=1;

uint8_t rxBuffer[8];

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  i2cScanner();

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  // Envío de datos de parqueo
server.on("/status",  HTTP_GET, []() {
  // Envío de datos de parqueo


  bool espacios[8];
  espacios[0] = rxBuffer[0];  // Esclavo 1
  espacios[1] = rxBuffer[1];  // Esclavo 2
  espacios[2] = rxBuffer[2];  // Esclavo 3
  espacios[3] = rxBuffer[3];  // Esclavo 4
  espacios[4] = rxBuffer[4];  // Esclavo 5
  espacios[5] = rxBuffer[5];  // Esclavo 6
  espacios[6] = rxBuffer[6];  // Esclavo 7
  espacios[7] = rxBuffer[7];  // Esclavo 8

  // Armar JSON
  String json = "{\"spaces\":[";
  for (int i = 0; i < 8; i++) {
    json += espacios[i] ? "true" : "false";
    if (i < 7) json += ",";
  }
  json += "]}";

  server.send(200, "application/json", json);
});


  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
   server.handleClient();
  if(LED1status)
  {digitalWrite(LED1pin, HIGH);}
  else
  {digitalWrite(LED1pin, LOW);}
  
  if(LED2status)
  {digitalWrite(LED2pin, HIGH);}
  else
  {digitalWrite(LED2pin, LOW);}


  delay(1000);
  //Escribir a las nucleos...
  
  Wire.beginTransmission(NucleoSegmentos);
  Wire.write(comando);
  Serial.println("se envio: "+String(comando));
  error= Wire.endTransmission(true);
  Serial.println(error);

  //Leerlas...
  uint8_t sensoresDisplay= 0;
  if(Wire.requestFrom(NucleoSegmentos, 1)){
    sensoresDisplay = Wire.read();
    Serial.print("Respuesta segmentos: ");
    Serial.println(sensoresDisplay);
  }else{
    Serial.println("No se recibio nada.");



  }


  //Escribir a Jorchhh el estado de cada sensor...
  Wire.beginTransmission(NucleoLCD);
  Wire.write(sensoresDisplay);
  error = Wire.endTransmission(true);
  Serial.print("Estado reenviado a LCD: 0b");
  Serial.println(sensoresDisplay, BIN);
  
  //leer la de Jorge.
  uint8_t sensoresjorge=0;
  if(Wire.requestFrom(NucleoLCD, 1)){
    sensoresjorge= Wire.read();
    Serial.print("Sensores: ");
    Serial.println(sensoresjorge, BIN);
  }else {
    Serial.println("No se recibio nada de jorch");
  }
  // CONTAR OCUPADOS
  comando = 0;
  for(int i = 0; i < 4; i++){
    if(sensoresjorge & (1 << i)){
      comando++;
    }
  }
  Serial.print("Ocupados: ");
  Serial.println(comando);

//************************************
  //Variable para el servidor o la RED
  //**********************************
  uint8_t sensoresTotales = 0;

  sensoresTotales = ((sensoresjorge & 0x0F) << 4) | (sensoresDisplay & 0x0F);

  Serial.print("Sensores Totales: 0b");
  Serial.println(sensoresTotales, BIN);
}


void i2cScanner()
{
  byte error, address;
  int nDevices;
  Serial.println("Scaneando...");
  nDevices =0;
  for (address=1; address<127; address++)
  {
    Wire.beginTransmission(address);
    error= Wire.endTransmission();
    if (error ==0 ){
      Serial.print("Dispositivo I2C encontrado en la direccion 0x");
      if(address<16){
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    }else if(error==4){
      Serial.print("Error desconocido en la direccion 0x");
      if(address<16)
      { 
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }
  if(nDevices == 0)
  {
    Serial.println("No se encontro el dispositivo I2C\n");
  }else{} 
  
}

void handle_led1on() {
  LED1status = HIGH;
  Serial.println("GPIO4 Status: ON");
  server.send(200, "text/html", SendHTML(true,LED2status)); 
}

void handle_led1off() {
  LED1status = LOW;
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", SendHTML(false,LED2status)); 
}

void handle_led2on() {
  LED2status = HIGH;
  Serial.println("GPIO5 Status: ON");
  server.send(200, "text/html", SendHTML(LED1status,true)); 
}

void handle_led2off() {
  LED2status = LOW;
  Serial.println("GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,false)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}


String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String pagina = "<!DOCTYPE html>\n";
pagina += "<html lang=es>\n";
pagina += "<head>\n";
pagina += "<meta charset=UTF-8>\n";
pagina += "<meta name=viewport content=\"width=device-width, initial-scale=1.0\">\n";
pagina += "<title>Parqueomatic</title>\n";
pagina += "<style>@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@400;700;900&display=swap');:root{--bg:#1a1d23;--surface:#22262f;--border:#2e3440;--green:#39ff6e;--red:#ff3b3b;--blue:#4a90d9;--text:#c8d0e0;--accent:#f0c040}*{box-sizing:border-box;margin:0;padding:0}body{background:var(--bg);font-family:'Exo 2',sans-serif;color:var(--text);min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 16px;background-image:radial-gradient(ellipse at 20% 10%,rgba(74,144,217,0.08) 0,transparent 60%),radial-gradient(ellipse at 80% 90%,rgba(57,255,110,0.05) 0,transparent 60%)}header{width:100%;max-width:760px;display:flex;justify-content:space-between;align-items:center;margin-bottom:24px;padding-bottom:14px;border-bottom:1px solid var(--border)}header h1{font-size:1.4rem;font-weight:900;letter-spacing:.12em;text-transform:uppercase;color:#fff}header h1 span{color:var(--accent)}#status-dot{width:10px;height:10px;border-radius:50%;background:var(--green);box-shadow:0 0 8px var(--green);display:inline-block;margin-right:8px;animation:pulse 2s infinite}@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}#live-label{font-family:'Share Tech Mono',monospace;font-size:.75rem;color:var(--green);display:flex;align-items:center}.main-wrap{width:100%;max-width:760px;display:flex;gap:20px;align-items:flex-start}#parking-bg{flex:1;position:relative}#parking-bg img.bg{width:100%;display:block;border-radius:10px}#overlay{position:absolute;top:0;left:0;width:100%;height:100%}.space{position:absolute;display:flex;flex-direction:column;align-items:center;justify-content:flex-start}.indicator{width:100%;height:10px;border-radius:3px;background:var(--green);box-shadow:0 0 8px var(--green);transition:background .4s,box-shadow .4s;margin-bottom:3px;flex-shrink:0}.indicator.occupied{background:var(--red);box-shadow:0 0 8px var(--red)}.car-slot{width:100%;flex:1;position:relative;display:flex;align-items:center;justify-content:center}.car-img{width:90%;height:90%;object-fit:contain;opacity:0;transform:scale(0.7);transition:opacity .5s ease,transform .5s ease}.space.occupied .car-img{opacity:1;transform:scale(1)}#counter-panel{width:130px;background:var(--surface);border-radius:12px;border:1px solid var(--border);padding:20px 14px;display:flex;flex-direction:column;align-items:center;gap:8px;flex-shrink:0;position:relative;overflow:hidden;align-self:center}#counter-panel::after{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:linear-gradient(90deg,var(--green),var(--blue));border-radius:12px 12px 0 0}.counter-label{font-size:.65rem;text-transform:uppercase;letter-spacing:.12em;color:rgba(200,208,224,0.5);text-align:center;line-height:1.5}#available-count{font-family:'Share Tech Mono',monospace;font-size:4rem;font-weight:700;line-height:1;color:var(--green);text-shadow:0 0 20px rgba(57,255,110,0.4);transition:color .4s,text-shadow .4s}#available-count.low{color:var(--accent);text-shadow:0 0 20px rgba(240,192,64,0.5)}#available-count.full{color:var(--red);text-shadow:0 0 20px rgba(255,59,59,0.5)}.counter-sub{font-size:.6rem;color:rgba(200,208,224,0.3);text-align:center}.divider{width:100%;height:1px;background:var(--border);margin:4px 0}.total-row{display:flex;justify-content:space-between;width:100%;font-family:'Share Tech Mono',monospace;font-size:.7rem}.total-row span:last-child{color:#fff}footer{margin-top:16px;font-family:'Share Tech Mono',monospace;font-size:.65rem;color:rgba(200,208,224,0.25);letter-spacing:.08em}#last-update{color:rgba(200,208,224,0.45)}</style>\n";
pagina += "</head>\n";
pagina += "<body>\n";
pagina += "<header>\n";
pagina += "<h1>PARQUEOMATIC <span></span></h1>\n";
pagina += "<div id=live-label><span id=status-dot></span>EN VIVO</div>\n";
pagina += "</header>\n";
pagina += "<div class=main-wrap>\n";
pagina += "<div id=parking-bg>\n";
pagina += "<img class=bg src=data:image/png;base64,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 alt=Parqueo>\n";
pagina += "<div id=overlay>\n";
pagina += "</div>\n";
pagina += "</div>\n";
pagina += "<div id=counter-panel>\n";
pagina += "<div class=counter-label>Espacios<br>disponibles</div>\n";
pagina += "<div id=available-count>8</div>\n";
pagina += "<div class=counter-sub>de 8 totales</div>\n";
pagina += "<div class=divider></div>\n";
pagina += "<div class=total-row><span>Libres</span><span id=free-num>8</span></div>\n";
pagina += "<div class=total-row><span>Ocup.</span><span id=occ-num>0</span></div>\n";
pagina += "</div>\n";
pagina += "</div>\n";
pagina += "<footer>Última actualización: <span id=last-update>—</span></footer>\n";
pagina += "<img id=car-template src=data:image/png;base64,/9j/4AAQSkZJRgABAQAAAQABAAD/4gHYSUNDX1BST0ZJTEUAAQEAAAHIAAAAAAQwAABtbnRyUkdCIFhZWiAH4AABAAEAAAAAAABhY3NwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAA9tYAAQAAAADTLQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAlkZXNjAAAA8AAAACRyWFlaAAABFAAAABRnWFlaAAABKAAAABRiWFlaAAABPAAAABR3dHB0AAABUAAAABRyVFJDAAABZAAAAChnVFJDAAABZAAAAChiVFJDAAABZAAAAChjcHJ0AAABjAAAADxtbHVjAAAAAAAAAAEAAAAMZW5VUwAAAAgAAAAcAHMAUgBHAEJYWVogAAAAAAAAb6IAADj1AAADkFhZWiAAAAAAAABimQAAt4UAABjaWFlaIAAAAAAAACSgAAAPhAAAts9YWVogAAAAAAAA9tYAAQAAAADTLXBhcmEAAAAAAAQAAAACZmYAAPKnAAANWQAAE9AAAApbAAAAAAAAAABtbHVjAAAAAAAAAAEAAAAMZW5VUwAAACAAAAAcAEcAbwBvAGcAbABlACAASQBuAGMALgAgADIAMAAxADb/2wBDAAUDBAQEAwUEBAQFBQUGBwwIBwcHBw8LCwkMEQ8SEhEPERETFhwXExQaFRERGCEYGh0dHx8fExciJCIeJBweHx7/2wBDAQUFBQcGBw4ICA4eFBEUHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh7/wAARCAAmABwDASIAAhEBAxEB/8QAGQAAAgMBAAAAAAAAAAAAAAAABQYAAwQH/8QAMBAAAgEDAwEFBQkAAAAAAAAAAQIDAAQRBRIhMRNBUYGhBhQVYpEiI0JhcaPR0uL/xAAXAQEBAQEAAAAAAAAAAAAAAAAGBAUH/8QAKREAAgECBAQGAwAAAAAAAAAAAQIDBAUAESFBBnGBsRMxMkJRYRJSkf/aAAwDAQACEQMRAD8A5RrAM99DASVVF3Hvzu3eo2nn5qwrpkUqiQz3ILjdgOOM+VW6tdEXkk8KcJtiDycKWG7I8fxYz04PNChc6yoAVJgBwB2PT0o/VRTTVDmNwAMhqfrrjr9irrZb7PTx1lO7s35MQqndiATqo1A0+sFLVPdbu3kR2ID7CDjLAsE5IH5g+XnTDSnZ3Fy20TQlniYOVUYfG8MSQf0OB6U1RSJLEksZyjqGU+INXWwMsbI5zIPfBXjhoJ6uKpp0Ko6DzBGqkgjXcZAHC57URx+8ycRo7pGNxGMnL9/kPoK2yXNvG5SS4iRh1DOARVOvRvLqcaRlVb7sgkZxjtTmhyySKMKqAZzxeH+1ZdzTxZivxy3A+eWG/BlfHa7ZHO+niDL0s3oeQ+0H9x55ddrnjjbWZVkEb7niO0jPG5Ac/UimmlS2jZbq1chArBVUK+7aBKnGcnj+T4CmutS1nOI/Ry/gAwG44QLXowGjKX398jvvkd9wOQwJ9oYhGgvsnagCygE5I3AqR3ZB8euTQf4z8/7H+6lSp7hBG0oYjU42uD7tWRURiSQhVOnfuSeuNmjEaledrn7EJVpMgqWIztAAPQHJ5J5piqVKvoo1jhAUYJcT1k9XcXaZsyMgOWWfck9cf//Z style=display:none alt>\n";
pagina += "<script>const TOTAL=8;const CAR_SRC=document.getElementById('car-template').src;const SPACE_DEFS=[{id:1,left:8,top:10,w:7,h:3,row:'top'},{id:2,left:17,top:10,w:7,h:3,row:'top'},{id:3,left:26,top:10,w:7,h:3,row:'top'},{id:4,left:36,top:10,w:7,h:3,row:'top'},{id:5,left:8,top:92,w:7,h:3,row:'bottom'},{id:6,left:17,top:92,w:7,h:3,row:'bottom'},{id:7,left:26,top:92,w:7,h:3,row:'bottom'},{id:8,left:36,top:92,w:7,h:3,row:'bottom'},];function buildGrid(){const overlay=document.getElementById('overlay');SPACE_DEFS.forEach(def=>{const wrap=document.createElement('div');wrap.className='space';wrap.id='space-'+def.id;wrap.style.left=def.left+'%';wrap.style.top=def.top+'%';wrap.style.width=def.w+'%';wrap.style.height=def.h+'%';wrap.style.flexDirection=def.row==='top'?'column':'column-reverse';const ind=document.createElement('div');ind.className='indicator';ind.id='ind-'+def.id;const slot=document.createElement('div');slot.className='car-slot';const car=document.createElement('img');car.className='car-img';car.src=CAR_SRC;car.alt='Auto';if(def.row==='bottom')car.style.transform='scale(0.7) rotate(180deg)';slot.appendChild(car);wrap.appendChild(ind);wrap.appendChild(slot);overlay.appendChild(wrap);});}\n";
pagina += "function applyState(occupied){let freeCount=0;for(let i=1;i<=TOTAL;i++){const isOcc=occupied[i-1];const spEl=document.getElementById('space-'+i);const indEl=document.getElementById('ind-'+i);const carImg=spEl.querySelector('.car-img');if(isOcc){spEl.classList.add('occupied');indEl.classList.add('occupied');const def=SPACE_DEFS[i-1];carImg.style.transform=def.row==='bottom'?'scale(1) rotate(180deg)':'scale(1)';}else{spEl.classList.remove('occupied');indEl.classList.remove('occupied');const def=SPACE_DEFS[i-1];carImg.style.transform=def.row==='bottom'?'scale(0.7) rotate(180deg)':'scale(0.7)';freeCount++;}}\n";
pagina += "const countEl=document.getElementById('available-count');countEl.textContent=freeCount;countEl.className=freeCount===0?'full':freeCount<=2?'low':'';document.getElementById('free-num').textContent=freeCount;document.getElementById('occ-num').textContent=TOTAL-freeCount;document.getElementById('last-update').textContent=new Date().toLocaleTimeString('es-GT');}\n";
pagina += "const POLL_MS=2000;async function fetchStatus(){try{const res=await fetch('/status',{cache:'no-store'});if(!res.ok)throw new Error('HTTP '+res.status);const data=await res.json();let occupied;if(Array.isArray(data.spaces)){occupied=data.spaces.slice(0,TOTAL).map(Boolean);}else if(typeof data.mask==='number'){occupied=Array.from({length:TOTAL},(_,i)=>!!(data.mask&(1<<i)));}else{throw new Error('Formato JSON inesperado');}\n";
pagina += "applyState(occupied);setOnline(true);}catch(e){setOnline(false);console.warn('[Parqueo] ESP32 sin respuesta:',e.message);}}\n";
pagina += "function setOnline(online){const dot=document.getElementById('status-dot');const label=document.getElementById('live-label');dot.style.background=online?'var(--green)':'var(--red)';dot.style.boxShadow=online?'0 0 8px var(--green)':'0 0 8px var(--red)';label.style.color=online?'var(--green)':'var(--red)';label.childNodes[1].textContent=online?'EN VIVO':'SIN SEÑAL';}\n";
pagina += "buildGrid();fetchStatus();setInterval(fetchStatus,POLL_MS);</script>\n";
pagina += "</body>\n";
pagina += "</html>";
  return pagina;
}

