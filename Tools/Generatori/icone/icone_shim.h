// Valdorso - I tipi di Unreal finti, quanto basta per compilare i disegni degli oggetti fuori da Unreal (icone_estrai.cpp).
#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <initializer_list>
#include <algorithm>
typedef int int32; typedef unsigned char uint8;
typedef char16_t TCHAR_;
#define TEXT(x) x
typedef char TCHAR;
struct FVector2D { double X=0,Y=0; FVector2D(){} FVector2D(double x,double y):X(x),Y(y){} FVector2D& operator*=(double k){X*=k;Y*=k;return *this;} FVector2D operator+(const FVector2D&o)const{return {X+o.X,Y+o.Y};} FVector2D operator*(double k)const{return {X*k,Y*k};}};
template<class T> struct TArray : std::vector<T> { using std::vector<T>::vector; TArray(){} TArray(std::initializer_list<T> l):std::vector<T>(l){} void Add(const T&v){this->push_back(v);} void Reserve(int n){this->reserve(n);} int Num()const{return (int)this->size();} };
struct FString : std::string { using std::string::string; FString(){} FString(const std::string&s):std::string(s){} bool StartsWith(const char*p)const{return rfind(p,0)==0;} bool EndsWith(const char*p)const{std::string q(p); return size()>=q.size() && compare(size()-q.size(),q.size(),q)==0;} };
struct FName { std::string S; FName(){} FName(const char*s):S(s){} std::string ToString()const{return S;} bool operator==(const FName&o)const{return S==o.S;} bool operator!=(const FName&o)const{return S!=o.S;} };
inline bool operator==(const FString&a,const char*b){return std::string(a)==b;}
inline bool operator!=(const FString&a,const char*b){return std::string(a)!=b;}
namespace FMath { template<class T> T Lerp(T a,T b,float t){return a+(b-a)*t;} inline double DegreesToRadians(double d){return d*M_PI/180;} inline double Cos(double a){return cos(a);} inline double Sin(double a){return sin(a);} inline double Sqrt(double a){return sqrt(a);} template<class T> T Max(T a,T b){return a>b?a:b;} template<class T> T Min(T a,T b){return a<b?a:b;} inline double Abs(double a){return fabs(a);} }
enum class EValdorsoUso { Nessuno, Mangia, Bevi, Cura, Leggi, Impugna, Accendi, Documento, Raccogli };
enum class EValdorsoPosto { Nessuno, ManoDestra, ManoSinistra };
struct FValdorsoTipoOggetto { FName Id; std::string Nome, Descrizione, Lettura; float Peso=0; int32 Pila=1; EValdorsoUso Uso=EValdorsoUso::Nessuno; EValdorsoPosto Posto=EValdorsoPosto::Nessuno; float Salute=0, Stamina=0, Equilibrio=0, Calore=0, DannoArma=0, EquilibrioArma=0, StaminaArma=0, PortataArma=0, Parata=0; int32 Cariche=0, TipoColpo=0; bool bRicordo=false; bool bDueMani=false; FName Vuoto; };
