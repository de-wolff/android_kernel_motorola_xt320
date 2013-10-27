#ifndef LOGSUM
#define LOGSUM
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <iostream>
#include <cassert>
#define MIN_LOGSUM_TABLE_BITS 3
class Logsum
{   
 protected:
	const int int_bits; // Address bits for LUT unless lower than MIN_LOGSUM_TABLE_BITS
	const int frac_bits; // Fractional bits in the input
	const int frac_bits_output; // Fractional bits in the output
	const int lut_bits; // address bits for LUT
	const int Max_val; // Maximum value for LUT
	const int LUT_size;  // LUT size
	const int lscale; 
	const double scale;  // calculate scaling during construction to avoid recalculating for each call
	const double xscale; // scale factor for LUT intervals
	const int mscale; 
	int* LUT; // LUT

 public:                            
    //! Constructor - specify the number of bits for LUT address, # integer bits and fractional bits
	Logsum(int i, int f) : int_bits(i), frac_bits(f), frac_bits_output(f), 
		lut_bits((i>MIN_LOGSUM_TABLE_BITS)? i:MIN_LOGSUM_TABLE_BITS),
		LUT_size(1<<lut_bits), 	lscale(i+f-lut_bits), 
		scale(1.0/(double)(1<<frac_bits)),
		Max_val(1<<frac_bits_output),
		xscale((1 << MIN_LOGSUM_TABLE_BITS)/((double)(1<<lut_bits))),
		mscale(0) {
		init();
	}
	Logsum(int i, int f, int fo) : int_bits(i), frac_bits(f), frac_bits_output(fo), 
		lut_bits((i>MIN_LOGSUM_TABLE_BITS)? i:MIN_LOGSUM_TABLE_BITS),
		LUT_size(1<<lut_bits), 	lscale(MIN_LOGSUM_TABLE_BITS+f-lut_bits), 
		scale(1.0/(double)(1<<frac_bits)),
		Max_val(1<<frac_bits_output),
		xscale((1 << MIN_LOGSUM_TABLE_BITS)/((double)(1<<lut_bits))),
		mscale(f-fo) {
		init();
	}
	void init() {
		LUT = new int[LUT_size];
		for (int k=0;k<LUT_size;k++) {
			LUT[k] = int(floor(Max_val*
							   log2(1.0+pow(2.0,-(k+0.0)*xscale))+0.5));
		}
		// lscale should never be negative i.e lut_bits should never by more than
		// input fractional bits
		assert(lscale >= 0);
		//		print();
	}
	// print out Lookup table
	void print() {
		std::cout << " For Logsum LUT params, lut_bits = "  << lut_bits
				  << " int_bits = " << int_bits
				  << " lscale = " << lscale
				  << " mscale = " << mscale
				  << " fractional in bit = " << frac_bits
				  << " and fractional out bits = " << frac_bits_output << "\n";
		std::cout << " LUT[] = {";
		for (int k=0;k<LUT_size;k++) {
			std::cout << LUT[k];
			if (k < LUT_size-1) std::cout << ",";
		}
		std::cout << "}\n";
	}
	// Destructor
	~Logsum() {
		delete [] LUT;
	}
	// Function object handling of Logsum for 2 fixed point numbers represented as integers
	int inline operator ()(int x, int y) { 
		int sum, LUT_index;
		LUT_index = ROUND(ABS(x-y),lscale);
		if (mscale < 0) {
			sum = MAX(x,y) << -mscale;
		} else {
			sum = MAX(x,y) >> mscale;
		}
		if (LUT_index < LUT_size) sum += LUT[LUT_index];
#ifdef DEBUG
		std::cout << " LOGSUM: x = " << x
				  << " y = " << y 
				  << " LUT_index = " << LUT_index 
				  << " LUT[] = " << LUT[LUT_index]
				  << " MAX = " << MAX(x,y)
				  << " Res =  " << sum << "\n";

#endif
		return(sum);
	}
	// Function object handling of Logsum for 2 double inputs
	double inline operator ()(double x, double y) { 
		return(Max_val*log2(pow(2.0,scale*x) + pow(2.0,scale*y)));
	}
	float inline operator ()(float x, float y) { 
		return(Max_val*  
			   log2(pow(2.0,scale*x) + pow(2.0,scale*y)));
	}
};
#endif
