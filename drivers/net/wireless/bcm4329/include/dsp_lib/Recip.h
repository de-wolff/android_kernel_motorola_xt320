#ifndef RECIP
#define RECIP
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke
#include <dsp_lib/dsp_lib.h>
#include <iostream>
// Lookup table class for reciprocal function
// Generates and puts data into array LUT for future access
// assumes index 0 of LUT corresponds to 0.5 and last index
// corresponds to approx 1.0
class Recip
{   
 protected:
	const int frac_bits; // Fractional bits = Address bits for LUT
	const int Max_val;   // Maximum value for LUT
	const int LUT_size;  // LUT size
	int* LUT; // LUT

 public:                            
    //! Constructor - specify the number of fractional bits
	Recip(int f) : frac_bits(f),
		LUT_size(1<<f),	Max_val(1<<frac_bits) {
		LUT = new int[LUT_size];
		for (int k=0;k<LUT_size;k++) {
			LUT[k] = int(floor(4.0*Max_val/(1.0+((k+0.0)/Max_val))+0.5));
		}
	}
	// print out Lookup table
	void print() {
		std::cout << " For Reciprocal LUT, "  
				  << " fractional bits = " << frac_bits << "\n";
		std::cout << " LUT[] = {";
		for (int k=0;k<LUT_size;k++) {
			std::cout << LUT[k];
			if (k < LUT_size-1) std::cout << ",";
		}
		std::cout << "}\n";
	}
	// Destructor
	~Recip() {
		delete [] LUT;
	}
	// Function object handling of Recip for fixed point numbers represented as integers
	int inline operator ()(int x) { 
		if (x>LUT_size-1) return(LUT[LUT_size-1]); // error 
		int res = LUT[x];
		return(res);
	}
};
#endif


