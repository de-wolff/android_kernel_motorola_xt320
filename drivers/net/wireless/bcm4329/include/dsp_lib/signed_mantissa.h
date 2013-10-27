#ifndef SIGNED_MAN_H_
#define SIGNED_MAN_H_

#include <dsp_lib/dsp_lib.h>
#include <dsp_lib/dsp_functions.h>

// This class is for use within a floating point class such as aml_flt
// Although it is basically a two's complement integer, it has the same interfaces as the signed magnitude class
// currently used in aml_flt so that it could be substituted in the future with a #define and/or typedef
// Also with another #define, the Right shifter operator >> can be made to do rounding instead of truncation if desired.
// Define RSHIFT_USE_ROUND if you want to round instead of truncate with Right Shift >> operator

class signed_mantissa {

 public:
	int64_t mantissa;
	int  Bits;

	signed_mantissa() {
		mantissa = 0;
		Bits = 0;
	}

	// This constructor initializes the # of Bits only! 
	// Does NOT set the mantissa value. use with caution!
	signed_mantissa(int x) { Bits = x; }

	signed_mantissa(const signed_mantissa& x) {
		mantissa = x.mantissa;
		Bits = x.Bits;
	}


	signed_mantissa& operator =(const int x) {
		set_mantissa(x);
		return *this;
	}
	// Copy everything except Bits
	signed_mantissa& operator =(const signed_mantissa &x) {
		mantissa = LIMIT_BITS64(x.mantissa,Bits-1);
		return *this;
	}


	/* Get/Set Functions for Member Variables */
	int get_signbit() const {	
		int signbit = (mantissa < 0) ? 1 : 0;
		return signbit;
	}
	uint64_t get_unsigned_mantissa() const {	
		if (mantissa < 0) return -mantissa;
		else return mantissa;	
	}
	int64_t get_mantissa() const { return mantissa; }
	// Most -ve number will overflow signbit!
	int64_t get_mantissa_sm() const {
		int64_t signbit = get_signbit();
		int64_t umantissa = get_unsigned_mantissa();
		return((signbit << (Bits-1)) | umantissa);
	}

	int64_t get_max_mantissa() const {	return( (int64_t(1) << (Bits-1)) - 1); }
	void set_max_mantissa() {	mantissa = ( (int64_t(1) << (Bits-1)) - 1); }

	// Most -ve number could overflow range
	void set_signbit(int x) { 
		int64_t umantissa = get_unsigned_mantissa();
		if(x) mantissa = -umantissa;
		else  mantissa = umantissa;
	}
	// BYPASS Limit setting!
	void force_unsigned_mantissa(uint64_t x) { mantissa = (int64_t)x; }
	void set_unsigned_mantissa(uint64_t x) { 
		int signbit = get_signbit();
		if(signbit) mantissa = -(int64_t)x;
		else		mantissa = (int64_t)x;
		mantissa = LIMIT_BITS64((int64_t)x,Bits-1);
	}
	void set_mantissa(int64_t x) { 	mantissa = LIMIT_BITS64(x,Bits-1);	}
	// Multiplication
	signed_mantissa operator *(const signed_mantissa& x) const {
		signed_mantissa tmp;
		tmp.Bits      = Bits + x.Bits - 1;
		tmp.mantissa = mantissa * x.get_mantissa();
		return tmp;
	}
	signed_mantissa operator -() const {
		signed_mantissa tmp(*this);
		tmp.mantissa *= -1;
		return(tmp);
	}
	// addition
	signed_mantissa	operator +(const signed_mantissa& x) const {
		signed_mantissa tmp;
		uint64_t man_1 = mantissa;
		uint64_t man_2 = x.get_mantissa();
		tmp.Bits = MAX(x.Bits,Bits)+1;
		tmp.set_mantissa(man_1 + man_2);
		return tmp;
	}
	signed_mantissa	operator -(const signed_mantissa& x) const {
		uint64_t man_1 = mantissa;
		uint64_t man_2 = x.get_mantissa();
		signed_mantissa tmp;
		tmp.Bits = MAX(x.Bits,Bits)+1;
		tmp.set_mantissa(man_1 - man_2);
		return tmp;
	}

	// various accumulators, i.e., += or -= from here below.
	signed_mantissa&operator +=(const signed_mantissa& x) {
		uint64_t man_1 = mantissa; 
		uint64_t man_2 = x.get_mantissa();
		set_mantissa(man_1 + man_2);
		return *this;
	}

	signed_mantissa operator -=(const signed_mantissa& x) {
		uint64_t man_1 = mantissa; 
		uint64_t man_2 = x.get_mantissa();
		set_mantissa(man_1 - man_2);
		return *this;
	}

	// Left Shifting
	signed_mantissa operator <<=(int x) {
		mantissa <<= x;
		return *this;
	}
	// Right shifting
	signed_mantissa operator >>=(int x) {
#ifdef RSHIFT_USE_ROUND
		mantissa = ROUND(mantissa,x);
#else
		mantissa >>= x;
#endif
		return *this;
	}

	// Left Shifting
	signed_mantissa operator <<(const int x) const {
		signed_mantissa tmp(*this);
		tmp.Bits += x;
		tmp <<= x;
		return tmp;
	}

	// Right shifting
	signed_mantissa operator >>(const int x) const {
		signed_mantissa tmp(*this);
		tmp.Bits -= x;
		tmp >>= x;
		return tmp;
	}

	// Most -ve number could overflow range
	signed_mantissa abs() {
		signed_mantissa tmp(*this);
		tmp.mantissa = get_unsigned_mantissa();
		return tmp;
	}
	bool operator ==(const signed_mantissa& x) const { return (mantissa == x.mantissa); }
	bool operator !=(const signed_mantissa& x) const { return (mantissa != x.mantissa); }



	friend std::ostream& 
		operator <<(std::ostream& os, signed_mantissa &r) {
		return os << r.mantissa;
	}
	friend std::istream& 
		operator >>(std::istream& os, signed_mantissa &r) {
		int64_t x;
		return os >> x;
		r.set_mantissa(x);
	}
}; // END OF CLASS SIGNED_MANTISSA

// forward declaration
#include <dsp_lib/complex.h>

inline complex<signed_mantissa>  operator <<(complex<signed_mantissa> &r, const int32_t shift) {
	r.i = r.i << shift;
	r.q = r.q << shift;
	complex<signed_mantissa> res = complex<signed_mantissa>(r.i, r.q);
	return(res);
}
//! Right shift
inline complex<signed_mantissa>  operator >>(complex<signed_mantissa> &r, const int32_t shift) {
	r.i = r.i >> shift;
	r.q = r.q >> shift;
	complex<signed_mantissa> res = complex<signed_mantissa>(r.i, r.q);
	return(res);
}


#endif /*SIGNED_MANTISSA_H_*/
