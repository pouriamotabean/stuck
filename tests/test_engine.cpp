#include "../Source/StutterEngine.h"
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
static void writeWav(const char* fn, const std::vector<float>& L, const std::vector<float>& R, int sr){
  FILE* f=fopen(fn,"wb"); uint32_t n=L.size(), data=n*4, sz=36+data; uint16_t ch=2,bps=16,fmt=1,ba=4; uint32_t br=sr*4,s=sr,sixteen=16;
  fwrite("RIFF",1,4,f);fwrite(&sz,4,1,f);fwrite("WAVEfmt ",1,8,f);fwrite(&sixteen,4,1,f);fwrite(&fmt,2,1,f);fwrite(&ch,2,1,f);fwrite(&s,4,1,f);fwrite(&br,4,1,f);fwrite(&ba,2,1,f);fwrite(&bps,2,1,f);fwrite("data",1,4,f);fwrite(&data,4,1,f);
  for(uint32_t i=0;i<n;i++){int16_t a=(int16_t)std::lround(std::max(-1.f,std::min(1.f,L[i]))*32000),b=(int16_t)std::lround(std::max(-1.f,std::min(1.f,R[i]))*32000);fwrite(&a,2,1,f);fwrite(&b,2,1,f);}
  fclose(f);}
int main(){
  const int sr=48000; const double bpm=133; const int N=sr*12;
  std::vector<float> L(N),R(N),dryL(N);
  // "vocal-ish" test: melody of sung-like notes with vibrato + harmonics, syllable envelopes
  double ph=0; int notes[]={60,62,64,67,69,67,64,62,60,64,67,72};
  for(int i=0;i<N;i++){ double t=(double)i/sr; int k=(int)(t/ (60.0/bpm)); int m=notes[k%12];
    double f=440*pow(2,(m-69)/12.0)*(1+0.006*sin(2*M_PI*5.5*t)); ph+=2*M_PI*f/sr;
    double beat=fmod(t,60.0/bpm)/(60.0/bpm); double env= std::min(1.0,beat*20)*std::exp(-beat*1.5);
    double v=0; for(int h=1;h<8;h++) v+=sin(ph*h)/(h*h*0.7+0.3);
    L[i]=R[i]=dryL[i]=(float)(0.25*v*env); }
  StutterEngine e; e.prepare(sr,512);
  // automation: 0..2s off, 2..8s ramp 0->1, 8..9 hold, 9..10 ramp down to 0, rest off
  auto knobAt=[&](int i){ double t=(double)i/sr; if(t<2) return 0.0; if(t<8) return (t-2)/6; if(t<9) return 1.0; if(t<10) return 1-(t-9); return 0.0;};
  const int B=480; std::vector<float> loopMs;
  for(int s=0;s<N;s+=B){ int n=std::min(B,N-s); float* ch[2]={&L[s],&R[s]};
    e.process(ch,2,n,(float)knobAt(s),(float)knobAt(s+n),bpm); loopMs.push_back(e.currentLoopMs.load()); }
  writeWav("/home/claude/stuck/tests/out.wav",L,R,sr);
  std::vector<float> d(N); for(int i=0;i<N;i++) d[i]=dryL[i]; writeWav("/home/claude/stuck/tests/dry.wav",d,d,sr);
  // click metric: max |2nd difference| in output vs dry
  double mo=0,md=0; for(int i=2;i<N;i++){ mo=std::max(mo,(double)fabs(L[i]-2*L[i-1]+L[i-2])); md=std::max(md,(double)fabs(dryL[i]-2*dryL[i-1]+dryL[i-2])); }
  printf("max 2nd diff out=%.4f dry=%.4f\n",mo,md);
  for(int s: {1,3,4,6,7,8,9}) printf("t=%ds loop=%.1f ms\n",s,loopMs[s*sr/B]);
  // pass-through check before 2s
  double err=0; for(int i=0;i<2*sr;i++) err=std::max(err,(double)fabs(L[i]-dryL[i])); printf("bypass err=%g\n",err);
  double err2=0; for(int i=(int)(10.1*sr);i<N;i++) err2=std::max(err2,(double)fabs(L[i]-dryL[i])); printf("after err=%g\n",err2);
}
