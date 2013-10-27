#ifndef NR_RECIP
#define NR_RECIP
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <dsp_lib/Recip.h>
#include <iostream>
// Newton Raphson Approximation for Reciprocal
// Uses 1 LUT for initial guess (class Recip)
// Then iterations to refine result
class nr_recip
{
 protected:
	const int frac_bits;
	const int ROUND_BITS; // based on difference between frac_bits and Lut_bits
	const int HALF;       // Offset to subtract from input
	const int EXTRA_BITS; // Allow extra bits after 1st round
	const int I_BITS;
	const int FRB;
	const int ITERATIONS;
	Recip R; // uses LUT for initial guess (assumes input and output precision is the same)

 public:
    //! Constructor - specify the number of LUT bits, fractional bits, extra bits and Iterations
	nr_recip(int l, int f, int X, int IT) : R(l), frac_bits(f),
		ROUND_BITS(f-l-1),	HALF(1 << (f-1)), EXTRA_BITS(X),
		I_BITS(f-EXTRA_BITS), FRB(f+EXTRA_BITS), ITERATIONS(IT) {
	}
	// Destructor
	~nr_recip() {	}
	// Function object handling of Nr_Recip for fixed point numbers represented as integers
	int inline operator ()(int x) {
		long x1,x2,x3;
		int z  = x - HALF; // 1st subtract 1/2
		int zr = ROUND(z,ROUND_BITS); // round to correct size for LUT
		int guess = (R(zr) << ROUND_BITS); // use LUT and then rescale back
		// now do iterations
		for (int i=0;i<ITERATIONS;i++) {
			x1 = (2*((long)1 << 2*frac_bits) - (long)x*(long)guess);
			x2 = guess*ROUND(x1,I_BITS);
			guess  = ROUND(x2,FRB);
		}
		return(guess);
	}
	// Returning the number of fractional bits.
	int getFracBits() {
		return(frac_bits);
	}
};
#endif
