#include "can_bus.h"
static Stream* g_ser = nullptr;
static char g_line[64];
static int g_pos = 0;

void canbridge_begin(Stream& serial){ g_ser=&serial; g_pos=0; }

static int hexval(int c){
  if(c>='0'&&c<='9') return c-'0';
  if(c>='A'&&c<='F') return 10+(c-'A');
  if(c>='a'&&c<='f') return 10+(c-'a');
  return -1;
}

bool canbridge_read(CanFrame& out){
  if(!g_ser) return false;
  while(g_ser->available()){
    int c=g_ser->read();
    if(c=='\r') continue;
    if(c=='\n'){
      g_line[g_pos]=0; out.valid=false;
      if(g_pos>=11 && g_line[0]=='T'){
        uint32_t id=0;
        for(int i=1;i<=8;i++){ int v=hexval(g_line[i]); if(v<0){ g_pos=0; return false;} id=(id<<4)|v; }
        int L=hexval(g_line[9]); if(L<0||L>8){ g_pos=0; return false; }
        out.id=id; out.len=(uint8_t)L;
        int expect=10+L*2;
        if(g_pos>=expect){
          for(int i=0;i<L;i++){
            int hi=hexval(g_line[10+i*2]); int lo=hexval(g_line[11+i*2]);
            if(hi<0||lo<0){ g_pos=0; return false; }
            out.data[i]=(uint8_t)((hi<<4)|lo);
          }
          out.valid=true;
        }
      }
      g_pos=0; if(out.valid) return true; return false;
    }else{
      if(g_pos < (int)sizeof(g_line)-1) g_line[g_pos++]=(char)c; else g_pos=0;
    }
  }
  return false;
}

uint32_t n2k_pgn(uint32_t id){ return (id>>8)&0x1FFFF; }
