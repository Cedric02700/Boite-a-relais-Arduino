#include <EtherCard.h>

#define Nb 8                                   //defini le nombre de relais à piloter
#define STATIC 0                               //configuration IP: 0: DHCP, 1: manuelle

#if STATIC                                     //configuration IP manuelle(necessaire seulement si STATIC 1)
static byte myip[] = { 192,168,1,222 };        //adresse Ip si config manuelle             
static byte gwip[] = { 192,168,1,254 };        //adresse de du routeur (Box internet) si config manuelle
#endif

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };     //Adresse MAC de l'ENC28J60, elle doit etre unique sur le reseau local

const char* Nom[]=                
{
  "Relais 1",                                              //nom des boutons sur la page Web
  "Relais 2",
  "Relais 3", 
  "Relais 4", 
  "Relais 5", 
  "Relais 6",
  "Relais 7", 
  "Relais 8",
  "Relais 9", 
  "Relais 10",
  "Relais 11", 
  "Relais 12", 
  "Relais 13", 
  "Relais 14",
  "Relais 15", 
  "Relais 16" 
};

int RelaisPin[] = {9,8,7,6,5,4,3,2};                        //definit les pins sur lesquels sont connectés les relais
char Comparateur[7];
char Commande[7];

byte Ethernet::buffer[(280+((Nb-1)*110))];                  //Taille buffer page internet, 280 pour le premier bouton. Ajouter 110 pour chaque bouton supplementaire.
BufferFiller bfill;

const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

boolean Status[(Nb+1)];

void setup ()
{
  Serial.begin(9600);
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  #if STATIC
    ether.staticSetup(myip, gwip);
  #else
    if (!ether.dhcpSetup())
      Serial.println("DHCP failed");
  #endif
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
  for(int i = 0; i < Nb; i++)              //Definit les pins relais en sortie et desactive tous les relais
  {
    pinMode (RelaisPin[i], OUTPUT);
    digitalWrite(RelaisPin[i], HIGH);
  }
}
 
static word homePage()                //Création de la page Web              
{
  bfill = ether.tcpOffset();
  bfill.emit_p(http_OK);
  bfill.emit_p(PSTR(
    "<title>Controle observatoire</title>"    
    "<h1>Controle observatoire<h1>"));        
    for (int i = 11; i < (Nb+11); i++)
    {
      if(((i-11)%2) == 0)
      {
        if(Status[(i-10)] == 0)
        {
          bfill.emit_p(PSTR("<input type=submit value='$S OFF' style=width:200px;height:75px onClick=location.href='/?$DONN'>"),Nom[(i-11)] ,i);
        }
        if(Status[(i-10)] == 1)
        {
          bfill.emit_p(PSTR("<input type=submit value='$S ON' style=width:200px;height:75px onClick=location.href='/?$DOFF'>"),Nom[(i-11)] ,i); 
        }
      }
      if(((i-11)%2) == 1)
      {
        if(Status[(i-10)] == 0)
        {
          bfill.emit_p(PSTR("<input type=submit value='$S OFF' style=width:200px;height:75px onClick=location.href='/?$DONN'><br/>"),Nom[(i-11)] ,i);
        }
        if(Status[(i-10)] == 1)
        {
          bfill.emit_p(PSTR("<input type=submit value='$S ON' style=width:200px;height:75px onClick=location.href='/?$DOFF'><br/>"),Nom[(i-11)] ,i); 
        }
      }
      
    }
  return bfill.position();
}

void InterpreteCommande()   //Actualise l'état des relais
{
  for(int i = 11; i < (Nb+11); i++)
  {
    sprintf(Comparateur, "?%dONN",i);
    if (strcmp(Comparateur, Commande) == 0) 
    {
      digitalWrite(RelaisPin[(i-11)], LOW);
      Status[(i-10)] = 1;
    }
    sprintf(Comparateur, "?%dOFF",i);
    if (strcmp(Comparateur, Commande) == 0) 
    {
      digitalWrite(RelaisPin[(i-11)], HIGH);
      Status[(i-10)] = 0;
    }
  }  
}


void loop () 
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if (pos) 
  {
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) 
    {
      bfill.emit_p(http_Unauthorized);
    }
    data += 5;
    if(strncmp("?", data, 1) == 0)
    {
      for(int i = 0; i<6; i++)
      {
        Commande[i] = data[i];
      }
    }
    InterpreteCommande();
    ether.httpServerReply(homePage());
  }
}
