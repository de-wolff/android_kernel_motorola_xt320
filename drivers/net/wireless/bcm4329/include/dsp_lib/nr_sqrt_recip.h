#ifndef NR_SQRT_RECIP
#define NR_SQRT_RECIP
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <dsp_lib/Sqrt_recip.h>
#include <iostream>
// Newton Raphson Approximation for Sqrt_Reciprocal
// Uses 1 LUT for initial guess (class Sqrt_Recip)
// Then iterations to refine result
class nr_sqrt_recip
{   
 protected:
	const int frac_bits;
	const int ROUND_BITS; // based on difference between frac_bits and Lut_bits
	const int HALF;       // Offset to subtract from input
	const int EXTRA_BITS; // Allow extra bits after 1st round
	const int I_BITS; 
	const int FRB;
	const int ITERATIONS;
	Sqrt_recip R; // uses LUT for initial guess (assumes input and output precision is the same)

 public:                            
    //! Constructor - specify the number of LUT bits, fractional bits, extra bits and Iterations
	nr_sqrt_recip(int l, int f, int X, int IT) : R(l), frac_bits(f), 
		ROUND_BITS(f-l-1),	HALF(1 << (f-1)), EXTRA_BITS(X),
		I_BITS(2*f+1-EXTRA_BITS), FRB(f+EXTRA_BITS), ITERATIONS(IT) {
	}
	// Destructor
	~nr_sqrt_recip() {	}
	// Function object handling of Nr_Sqrt_Recip for fixed point numbers represented as integers
	int inline operator ()(int x) { 
		long x1,x2,x3;
		int z  = x - HALF; // 1st subtract 1/2 
		int zr = ROUND(z,ROUND_BITS); // round to correct size for LUT
		int guess = (R(zr) << ROUND_BITS); // use LUT and then rescale back
		long guess2 = (long)guess * (long) guess;
		// now do iterations
		for (int i=0;i<ITERATIONS;i++) {
			// formula is x = x*(3 - a*x*x)/2
			x1 = (3*((long)1 << (3*frac_bits)) - (long)x*guess2);
			// 1 extra bit of rounding to account for 1/2 in above formula
			x2 = guess*ROUND(x1,I_BITS);
			guess   = ROUND(x2,FRB);
			guess2  = (long)guess * (long)guess;
		}
		return(guess);
	}
    int getFracBits() {
		return(frac_bits);
	}
};
#endif
