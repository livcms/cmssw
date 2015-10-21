#ifndef liblogintpack_h
#define liblogintpack_h

#include <cmath>

namespace logintpack
{
        constexpr int8_t smallestPositive = 0;
        constexpr int8_t smallestNegative = -128; // this value is free, so use it to denote the smallest (by absolute value) negative number

        int8_t pack8logCeil(double x,double lmin, double lmax, uint8_t base=128)
        {
                if(base>128) base=128;
                float l =log(fabs(x));
                float centered = (l-lmin)/(lmax-lmin)*base;
                int8_t  r=ceil(centered);
                if(centered >= base-1) r=base-1;
                if(centered < 0) r=0;
                if(x<0) {
                  if(r==0) r=smallestNegative;
                  else     r=-r;
                }
                return r;
        }

	int8_t pack8log(double x,double lmin, double lmax, uint8_t base=128)
	{
	        if(base>128) base=128;
		float l =log(fabs(x));
		float centered = (l-lmin)/(lmax-lmin)*base;
		int8_t  r=centered;
		if(centered >= base-1) r=base-1;
		if(centered < 0) r=0;
                if(x<0) {
                  if(r==0) r=smallestNegative;
                  else     r=-r;
                }
		return r;
	}

        /// pack a value x distributed in [-1,1], with guarantee that -1 and 1 are preserved exactly in packing and unpacking.
        /// tries to keep the best precision for x close to the endpoints, sacrifying that in the middle
	int8_t pack8logClosed(double x,double lmin, double lmax, uint8_t base=128)
	{
	        if(base>128) base=128;
		float l =log(fabs(x));
		float centered = (l-lmin)/(lmax-lmin)*(base-1);
		int8_t  r=round(centered);
		if(centered >= base-1) r=base-1;
		if(centered < 0) r=0;
                if(x<0) {
                  if(r==0) r=smallestNegative;
                  else     r=-r;
                }
		return r;
	}


	double unpack8log(int8_t i,double lmin, double lmax, uint8_t base=128)
	{
	        if(base>128) base=128;
	        float basef=base;
                //int8_t num = i == smallestNegative ? 0 : std::abs(i);
		float l=lmin+abs(i)/basef*(lmax-lmin);
                if(i==smallestNegative) l = lmin;
		float val=exp(l);
		if(i<0) return -val; else return val;
	}

        /// reverse of pack8logClosed
	double unpack8logClosed(int8_t i,double lmin, double lmax, uint8_t base=128)
	{
	        if(base>128) base=128;
	        float basef=base-1;
		float l=lmin+abs(i)/basef*(lmax-lmin);
                if (abs(i) == base-1) l = lmax;
                else if (i==smallestNegative) l = lmin;
		float val=exp(l);
		if(i<0) return -val; else return val;
	}

}
#endif
