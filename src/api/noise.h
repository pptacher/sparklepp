#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
const double _pi = std::acos(-1);


template<class T> inline T sqr(const T &x) { return x*x; }

template<class T>
inline T smooth_step(T r)
{
   if(r<0) return 0;
   else if(r>1) return 1;
   return r*r*r*(10+r*(-15+r*6));
}

template<class T>
inline T smooth_step(T r, T r_lower, T r_upper, T value_lower, T value_upper)
{
    return value_lower + smooth_step((r-r_lower)/(r_upper-r_lower)) * (value_upper-value_lower);
}

template<class T> inline T ramp(T r)
{
    return smooth_step((r+1)/2)*2-1;
}

struct Noise3
{
   Noise3(unsigned int seed=171717);
   void reinitialize(unsigned int seed);
   float operator()(float x, float y, float z) const;
   float operator()(const glm::vec3 &x) const { return (*this)(x[0], x[1], x[2]); }

   protected:
   static const unsigned int n=128;
   glm::vec3 basis[n];
   int perm[n];

   unsigned int hash_index(int i, int j, int k) const
   { return perm[(perm[(perm[i%n]+j)%n]+k)%n]; }
};

struct FlowNoise3: public Noise3
{
   FlowNoise3(unsigned int seed=171717, float spin_variation=0.2);
   void set_time(float t); // period of repetition is approximately 1

   protected:
   glm::vec3 original_basis[n];
   float spin_rate[n];
   glm::vec3 spin_axis[n];
};

// transforms even the sequence 0,1,2,3,... into reasonably good random numbers
// challenge: improve on this in speed and "randomness"!
inline unsigned int randhash(unsigned int seed)
{
   unsigned int i=(seed^12345391u)*2654435769u;
   i^=(i<<6)^(i>>26);
   i*=2654435769u;
   i+=(i<<5)^(i>>12);
   return i;
}

// returns repeatable stateless pseudo-random number in [0,1]
inline double randhashd(unsigned int seed)
{ return randhash(seed)/(double)UINT_MAX; }
inline float randhashf(unsigned int seed)
{ return randhash(seed)/(float)UINT_MAX; }
inline double randhashd(unsigned int seed, double a, double b)
{ return (b-a)*randhash(seed)/(double)UINT_MAX + a; }
inline float randhashf(unsigned int seed, float a, float b)
{ return ( (b-a)*randhash(seed)/(float)UINT_MAX + a); }
