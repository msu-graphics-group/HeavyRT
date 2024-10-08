#include <cmath>

#include "qmc_sobol_niederreiter.h"

typedef long long int INT64;


////////////////////////////////////////////////////////////////////////////////
// Table generation functions
////////////////////////////////////////////////////////////////////////////////
// Internal 64(63)-bit table
static INT64 cjn[63][QRNG_DIMENSIONS];

static int GeneratePolynomials(int buffer[QRNG_DIMENSIONS], bool primitive)
{
    int i, j, n, p1, p2, l;
    int e_p1, e_p2, e_b;

    //generate all polynomials to buffer
    for(n = 1, buffer[0] = 0x2, p2 = 0, l = 0; n < QRNG_DIMENSIONS; ++n){
        //search for the next irreducable polynomial
        for(p1 = buffer[n - 1] + 1; ; ++p1){
            //find degree of polynomial p1
            for(e_p1 = 30; (p1 & (1 << e_p1)) == 0; --e_p1) {} 

            // try to divide p1 by all polynomials in buffer
            for(i = 0; i < n; ++i){
                // find the degree of buffer[i]
                for(e_b = e_p1; (buffer[i] & (1 << e_b)) == 0; --e_b) {} 

                // divide p2 by buffer[i] until the end
                for(p2 = (buffer[i] << ((e_p2 = e_p1) - e_b)) ^ p1; p2 >= buffer[i]; p2 = (buffer[i] << (e_p2 - e_b)) ^ p2){
                    for( ; (p2 & (1 << e_p2)) == 0; --e_p2) {} 
                }// compute new degree of p2

                // division without remainder!!! p1 is not irreducable
                if(p2 == 0){
                    break; 
                }
            }

            //all divisions were with remainder - p1 is irreducable
            if(p2 != 0){
                e_p2 = 0;
                if(primitive){
                    //check that p1 has only one cycle (i.e. is monic, or primitive)
                    j = ~(0xffffffff << (e_p1 + 1));
                    e_b = (1 << e_p1) | 0x1;
                    for(p2 = e_b, e_p2 = (1 << e_p1) - 2; e_p2 > 0; --e_p2){
                        p2 <<= 1;
                        i = p2 & p1;
                        i = (i & 0x55555555) + ((i >> 1) & 0x55555555);
                        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
                        i = (i & 0x07070707) + ((i >> 4) & 0x07070707);
                        p2 |= (i % 255) & 1;
                        if ((p2 & j) == e_b) break;
                    }
                }
                //it is monic - add it to the list of polynomials
                if(e_p2 == 0){
                    buffer[n] = p1;
                    l += e_p1;
                    break;
                }
            }
        }
    }
    return l + 1;
}


////////////////////////////////////////////////////////////////////////////////
//  @misc{Bratley92:LDS,
//    author = "B. Fox and P. Bratley and H. Niederreiter",
//    title = "Implementation and test of low discrepancy sequences",
//    text = "B. L. Fox, P. Bratley, and H. Niederreiter. Implementation and test of
//      low discrepancy sequences. ACM Trans. Model. Comput. Simul., 2(3):195--213,
//      July 1992.",
//    year = "1992" }
////////////////////////////////////////////////////////////////////////////////
static void GenerateCJ(){
    int buffer[QRNG_DIMENSIONS];
    int *polynomials;
    int n, p1, l, e_p1;

    // Niederreiter (in contrast to Sobol) allows to use not primitive, but just irreducable polynomials
    l = GeneratePolynomials(buffer, false);

    // convert all polynomials from buffer to polynomials table
    polynomials = new int[l + 2 * QRNG_DIMENSIONS + 1];
    for(n = 0, l = 0; n < QRNG_DIMENSIONS; ++n){
        //find degree of polynomial p1
        for(p1 = buffer[n], e_p1 = 30; (p1 & (1 << e_p1)) == 0; --e_p1) {} 

        //fill polynomials table with values for this polynomial
        polynomials[l++] = 1;
        for(--e_p1; e_p1 >= 0; --e_p1){
            polynomials[l++] = (p1 >> e_p1) & 1;
        }
        polynomials[l++] = -1;
    }
    polynomials[l] = -1;

    // irreducable polynomial p
    int *p = polynomials, e, d;
    // polynomial b
    int b_arr[1024], *b, m;
    // v array
    int v_arr[1024], *v;
    // temporary polynomial, required to do multiplication of p and b
    int t_arr[1024], *t;
    // subsidiary variables
    int i, j, u, m1, ip, it;

    // cycle over monic irreducible polynomials
    for(d = 0; p[0] != -1; p += e + 2){
        // allocate memory for cj array for dimention (ip + 1)
        for(i = 0; i < 63; ++i){
            cjn[i][d] = 0; 
        }

        // determine the power of irreducable polynomial
        for(e = 0; p[e + 1] != -1; ++e) {} 
        // polynomial b in the beginning is just '1'
        (b = b_arr + 1023)[m = 0] = 1;
        // v array needs only (63 + e - 2) length
        v = v_arr + 1023 - (63 + e - 2);

        // cycle over all coefficients
        for(j = 63 - 1, u = e; j >= 0; --j, ++u){
            if(u == e){
                u = 0;
                // multiply b by p (polynomials multiplication)
                for(i = 0, t = t_arr + 1023 - (m1 = m); i <= m; ++i){
                    t[i] = b[i]; 
                }
                b = b_arr + 1023 - (m += e);

                for(i = 0; i <= m; ++i){
                    b[i] = 0;
                    for(ip = e - (m - i), it = m1; ip <= e && it >= 0; ++ip, --it){
                        if(ip >= 0){
                            b[i] ^= p[ip] & t[it];
                        }
                    }
                }
                // multiplication of polynomials finished

                // calculate v
                for(i = 0; i < m1; ++i){
                    v[i] = 0;
                }
                for(; i < m; ++i){
                    v[i] = 1;
                }
                for(; i <= 63 + e - 2; ++i){
                    v[i] = 0;
                    for (it = 1; it <= m; ++it){
                        v[i] ^= v[i - it] & b[it]; 
                    }
                }
            }

            // copy calculated v to cj
            for(i = 0; i < 63; i++){
                cjn[i][d] |= (INT64)v[i + u] << j; 
            }
        }
        ++d;
    }

    delete []polynomials;
}


////////////////////////////////////////////////////////////////////////////////
// Initialization (table setup)
////////////////////////////////////////////////////////////////////////////////
void initQuasirandomGenerator(unsigned int table[QRNG_DIMENSIONS][QRNG_RESOLUTION])
{
  GenerateCJ();

  for(int dim = 0; dim < QRNG_DIMENSIONS; dim++)
    for(int bit = 0; bit < QRNG_RESOLUTION; bit++)
      table[dim][bit] = (int)((cjn[bit][dim] >> 32) & 0x7FFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////
//Generate 63-bit quasirandom number for given index and dimension and normalize
////////////////////////////////////////////////////////////////////////////////
double getQuasirandomValue63(INT64 i, int dim)
{
    const double INT63_SCALE = (1.0 / (double)0x8000000000000001ULL);
    INT64 result = 0;

    for(int bit = 0; bit < 63; bit++, i >>= 1)
        if(i & 1) result ^= cjn[bit][dim];

    return (double)(result + 1) * INT63_SCALE;
}

#include <cstdint>
#include <algorithm>
#include <iostream>

/**
\brief  transform  sample in rect [-1,1]x[-1,1] to disc centered at (0,0) with radius == 1. 
\param  xy - inout sample in rect [-1,1]x[-1,1]
*/
static inline void MapSamplesToDisc(float xy[2])
{
  float x = xy[0];
  float y = xy[1];

  float r = 0;
  float phi = 0;

  if (x>y && x>-y)
  {
    r = x;
    phi = 0.25f*3.141592654f*(y / x);
  }

  if (x < y && x > -y)
  {
    r = y;
    phi = 0.25f*3.141592654f*(2.0f - x / y);
  }

  if (x < y && x < -y)
  {
    r = -x;
    phi = 0.25f*3.141592654f*(4.0f + y / x);
  }

  if (x >y && x<-y)
  {
    r = -y;
    phi = 0.25f*3.141592654f*(6 - x / y);
  }

  //float sin_phi, cos_phi;
  //sincosf(phi, &sin_phi, &cos_phi);
  float sin_phi = sin(phi);
  float cos_phi = cos(phi);

  xy[0] = r*sin_phi;
  xy[1] = r*cos_phi;
}

// generate samples
//
struct Sample
{
  float data[4];
  uint32_t x; uint32_t y;

  bool operator<(const Sample& rhs) const
  {
    const uint64_t key1 = uint64_t(x)     | (uint64_t(y) << 32);
    const uint64_t key2 = uint64_t(rhs.x) | (uint64_t(rhs.y) << 32);
    return (key1 < key2);
  }
};

std::vector<float> MakeSortedByPixel_QRND_2D_DISK(int a_width, int a_height, int a_randsPerPixel)
{
  // init
  //
  unsigned int table[QRNG_DIMENSIONS][QRNG_RESOLUTION];
  initQuasirandomGenerator(table);
  
  std::vector<Sample> samples(a_width*a_height*a_randsPerPixel);

  #pragma omp parallel for
  for(int i=0; i<int(samples.size()); i++)
  {
    Sample s;
    s.data[0] = rndQmcSobolN(i, 0, &table[0][0]);
    s.data[1] = rndQmcSobolN(i, 1, &table[0][0]);
    s.data[2] = rndQmcSobolN(i, 2, &table[0][0]);
    s.data[3] = rndQmcSobolN(i, 3, &table[0][0]);
    s.x = uint32_t(s.data[0]*float(a_width));
    s.y = uint32_t(s.data[1]*float(a_height));
    samples[i] = s;
  }

  std::sort(samples.begin(), samples.end()); // #TODO: sort samples in parallel

  //if (!VerifySamples(samples, a_width, a_height, a_randsPerPixel))
    //std::cout << "[MakeSortedByPixel_QRND_2D_DISK]: verification has FAILED!" << std::endl;

  // pack resulting data in single vector and return it
  //
  std::vector<float> res(a_width*a_height*a_randsPerPixel*2);

  for(int i=0; i<int(samples.size()); i++)
  {
    res[i*2+0] = samples[i].data[2]*2.0f - 1.0f; 
    res[i*2+1] = samples[i].data[3]*2.0f - 1.0f; 
    MapSamplesToDisc(samples[i].data+2);       // #TODO: check if this works
  }

  return res;
}


std::vector<uint32_t> Compress8SamplesToUint32x4(const std::vector<float>& samplesF, int a_tex_width, int a_tex_height)
{
  std::vector<uint32_t> samplesCompressed(a_tex_width*a_tex_height*4);
  
  constexpr int TEX_SAMPLES_PP = 8;

  #pragma omp parallel for
  for (int y = 0; y < a_tex_height; y++)
  {
    for (int x = 0; x < a_tex_width; x++)
    {
      float     f[TEX_SAMPLES_PP * 2];
      uint32_t  ui[TEX_SAMPLES_PP * 2];

      for (int k = 0; k < TEX_SAMPLES_PP * 2; k++)
      {
        f[k]  = (samplesF[(y*a_tex_width + x) * TEX_SAMPLES_PP * 2 + k] + 1.0f)*0.5f*255.0f;
        ui[k] = uint32_t(fmin(f[k], 255.0f));
      }

      samplesCompressed[(y*a_tex_width + x) * 4 + 0] = (ui[3] << 24)  | (ui[2] << 16)  | (ui[1] << 8)  | ui[0];
      samplesCompressed[(y*a_tex_width + x) * 4 + 1] = (ui[7] << 24)  | (ui[6] << 16)  | (ui[5] << 8)  | ui[4];
      samplesCompressed[(y*a_tex_width + x) * 4 + 2] = (ui[11] << 24) | (ui[10] << 16) | (ui[9] << 8)  | ui[8];
      samplesCompressed[(y*a_tex_width + x) * 4 + 3] = (ui[15] << 24) | (ui[14] << 16) | (ui[13] << 8) | ui[12];
    }
  }
  
  return samplesCompressed;
}