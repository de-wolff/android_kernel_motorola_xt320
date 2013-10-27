#ifndef ALOG
#define ALOG
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <iostream>
class Alog
{   
 protected:
	const int frac_bits; // Fractional bits = LUT address bits
	const double scale;  // calculate scaling during construction to avoid recalculating for each call
	const int Max_val; // Maximum value for LUT
	const int LUT_size;  // LUT size
	int* LUT; // LUT

 public:                            
    //! Constructor - specify the number of fractional bits
	Alog(int f) : frac_bits(f),
		LUT_size(1<<f), scale(1.0/(double)(1<<frac_bits)),
		Max_val(1<<frac_bits) {
		LUT = new int[LUT_size];
		for (int k=0;k<LUT_size;k++) {
			LUT[k] = int(floor(0.5+Max_val*pow(2.0,((double)k/Max_val))));
		}
	}
	// print out Lookup table
	void print() {
		std::cout << " For Alog LUT, fractional bits = " << frac_bits << "\n";
		std::cout << " LUT[] = {";
		for (int k=0;k<LUT_size;k++) {
			std::cout << LUT[k];
			if (k < LUT_size-1) std::cout << ",";
		}
		std::cout << "}\n";
	}
	// Destructor
	~Alog() {
		delete [] LUT;
	}
	// Function object handling of alog for fixed point numbers represented as integers
	int inline operator ()(int x) { 
		int res, LUT_index, shift;
		if (x >= 0) {
			int int_part = x >> frac_bits;
			LUT_index = x - (int_part << frac_bits); 
			res = LUT[LUT_index] << int_part;
#ifdef DEBUG
		std::cout << " x = " << x
				  << " int_part = " << int_part 
				  << " LUT_index = " << LUT_index 
				  << " LUT[] = " << LUT[LUT_index]
				  << " Res =  " << res << "\n";
#endif
			return(res);
		} else { 
			return(0);
		}
	}
	// Function object handling of alog 
	double inline operator ()(double x) {return(Max_val*pow(2.0,scale*x));}
	float inline operator ()(float x) { return(Max_val*pow(2.0,scale*x));}
};
#endif
