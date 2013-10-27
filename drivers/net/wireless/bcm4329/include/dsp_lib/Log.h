#ifndef LOG
#define LOG
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <iostream>
class Log
{   
 protected:
	const int frac_bits; // Fractional bits = Address bits for LUT
	const double scale;  // calculate scaling during construction to avoid recalculating for each call
	const int Max_val;   // Maximum value for LUT
	const int LUT_size;  // LUT size
	const int mask;
	int* LUT; // LUT

 public:                            
    //! Constructor - specify the number of integer bits and fractional bits
	Log(int f) : frac_bits(f),
		LUT_size(1<<f), scale(1.0/(double)(1<<frac_bits)),
		Max_val(1<<frac_bits),
		mask(~(~0<<frac_bits)) {
		LUT = new int[LUT_size];
		for (int k=0;k<LUT_size;k++) {
			LUT[k] = int(floor(Max_val*log2(1.0+((k+0.5)/Max_val))+0.5));
			if (LUT[k] > Max_val-1) LUT[k] = Max_val-1;
		}
	}
	// print out Lookup table
	void print() {
		std::cout << " For Log LUT, "  
				  << " fractional bits = " << frac_bits << "\n";
		std::cout << " LUT[] = {";
		for (int k=0;k<LUT_size;k++) {
			std::cout << LUT[k];
			if (k < LUT_size-1) std::cout << ",";
		}
		std::cout << "}\n";
	}
	// Destructor
	~Log() {
		delete [] LUT;
	}
	// Function object handling of Log for fixed point numbers represented as integers
	int inline operator ()(int x) { 
		int res, LUT_index;
		int int_part = (int)log2(x) - frac_bits;
		if (int_part < 0) {
			LUT_index =  ( x << -int_part ) & mask;
		} else {
			LUT_index =  ( x >> int_part) & mask;
		}
		res = (int_part << frac_bits) + LUT[LUT_index];
#ifdef DEBUG
		std::cout << " x = " << x
				  << " int_part = " << int_part 
				  << " LUT_index = " << LUT_index 
				  << " LUT[] = " << LUT[LUT_index]
				  << " i << f= " << (int_part << frac_bits)
				  << " Res =  " << res << "\n";
#endif
		return(res);
	}
	// Function object handling of Log 
	double inline operator ()(double x) {return(Max_val*log2(scale*x));}
	float inline operator ()(float x) { return(Max_val*log2(scale*x));}
};
#endif
