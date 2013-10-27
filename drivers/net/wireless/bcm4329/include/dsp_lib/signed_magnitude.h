#ifndef SIGNED_MAG_H_
#define SIGNED_MAG_H_

#include <iostream>
#include <dsp_lib/dsp_lib.h>
#include <dsp_lib/dsp_functions.h>

#ifdef CLEAR_ZERO_SIGN_BITS 
#define CLEAR_SIGNBIT(m,s) if(m == 0) s = 0;
#define RETURN_IF_ZERO_MANTISSA(m) if(m == 0) return(0);
#else
#define CLEAR_SIGNBIT(m,s) 
#define RETURN_IF_ZERO_MANTISSA(m) 
#endif


class signed_magnitude {

 public:
	uint64_t umantissa;
	int  Bits;
	int signbit;

	signed_magnitude() {
		umantissa = 0;
		Bits = 0;
		signbit = 0;
	}

	operator const long() { return( get_mantissa() ); }

	// This constructor initializes the # of Bits only! 
	// Does NOT set the mantissa value. use with caution!
	signed_magnitude(int x) { Bits = x; }

	signed_magnitude(const signed_magnitude& x) {
		umantissa = x.umantissa;
		Bits = x.Bits;
		signbit = x.signbit;
		CLEAR_SIGNBIT(umantissa,signbit)
	}


	signed_magnitude& operator =(const int x) {
		set_mantissa(x);
		return *this;
	}
	// Copy everything except Bits
	signed_magnitude& operator =(const signed_magnitude &x) {
		umantissa = x.umantissa;
		signbit=x.signbit;
		if (umantissa > get_max_mantissa()) umantissa = get_max_mantissa();
		CLEAR_SIGNBIT(umantissa,signbit);
		return *this;
	}


	/* Get/Set Functions for Member Variables */
	int get_signbit() const {	
		RETURN_IF_ZERO_MANTISSA(umantissa);
		return signbit;
	}
	uint64_t get_unsigned_mantissa() const {	return umantissa;	}
	int64_t get_mantissa() const {
		RETURN_IF_ZERO_MANTISSA(umantissa);
		if(signbit) return -((int64_t)umantissa);
		else        return (int64_t)umantissa;
	}
	int64_t get_mantissa_sm() const {
		RETURN_IF_ZERO_MANTISSA(umantissa);
		return(((int64_t)signbit << (Bits-1)) | (int64_t) umantissa);
	}

	uint64_t get_max_mantissa() const {	return( (uint64_t(1) << (Bits-1)) - 1); }
	void set_max_mantissa() {	umantissa = ( (uint64_t(1) << (Bits-1)) - 1); }

	void set_signbit(int x) { 
		signbit = x;
		CLEAR_SIGNBIT(umantissa,signbit);
	}
	// BYPASS Limit setting!
	void force_unsigned_mantissa(uint64_t x) { umantissa = x; }
	void set_unsigned_mantissa(uint64_t x) { 
		umantissa = x;
		if (x > get_max_mantissa()) {
			umantissa = get_max_mantissa();
		}
	}
	void set_mantissa(int64_t x) { 
		signbit=(x<0) ? 1 : 0;
		if(signbit) umantissa = (uint64_t)(-x);
		else		umantissa = (uint64_t)x;
		if (umantissa > get_max_mantissa()) umantissa = get_max_mantissa();
		CLEAR_SIGNBIT(umantissa,signbit);
	}
	// Multiplication
	signed_magnitude operator *(const signed_magnitude& x) const {
		signed_magnitude tmp;
		tmp.Bits      = Bits + x.Bits - 1;
		tmp.signbit   = signbit   ^ x.signbit;
		tmp.umantissa = umantissa * x.get_unsigned_mantissa();
		CLEAR_SIGNBIT(tmp.umantissa,tmp.signbit);
		return tmp;
	}
	signed_magnitude operator -() const {
		signed_magnitude tmp(*this);
		tmp.signbit ^= 1;
		CLEAR_SIGNBIT(tmp.umantissa,tmp.signbit);
		return(tmp);
	}
	// addition
	signed_magnitude	operator +(const signed_magnitude& x) const {
		signed_magnitude tmp;

		uint64_t man_1 = umantissa;
		uint64_t man_2 = x.get_unsigned_mantissa();
		int sign_1 = (signbit==0) ? 1 : -1;
		int sign_2 = (x.signbit==0) ? 1 : -1;
		int sign_dif = signbit ^ x.signbit;
		tmp.Bits = MAX(x.Bits,Bits)+1;
		tmp.set_mantissa(sign_1*man_1 + sign_2*man_2);
		// for 0 mantissa, signbit shouldn't matter. However RTL signed_magnitude class
		// will take signbit from LHS fo "+/-" , so this is what we do here.
		if((tmp.get_mantissa() == 0) && (sign_dif == 0)) tmp.signbit = signbit;
		CLEAR_SIGNBIT(tmp.umantissa,tmp.signbit);
		return tmp;
	}
	signed_magnitude	operator -(const signed_magnitude& x) const {
		uint64_t man_1 = umantissa;
		uint64_t man_2 = x.get_unsigned_mantissa();
		int sign_1 = (signbit==0) ? 1 : -1;
		int sign_2 = (x.signbit==0) ? 1 : -1;
		int sign_dif = signbit ^ x.signbit;
		signed_magnitude tmp;
		tmp.Bits = MAX(x.Bits,Bits)+1;
		tmp.set_mantissa(sign_1*man_1 - sign_2*man_2);
		// for 0 mantissa, signbit shouldn't matter. However RTL signed_magnitude class
		// will take signbit from LHS fo "+/-" , so this is what we do here.
		if((tmp.get_mantissa() == 0)) tmp.signbit = signbit;
		CLEAR_SIGNBIT(tmp.umantissa,tmp.signbit);
		return tmp;
	}

	// various accumulators, i.e., += or -= from here below.
	signed_magnitude&operator +=(const signed_magnitude& x) {
		uint64_t man_1 = umantissa; 
		uint64_t man_2 = x.get_unsigned_mantissa();

		int sign_1 = (signbit==0) ? 1 : -1;
		int sign_2 = (x.signbit==0) ? 1 : -1;
		int sign_dif = signbit ^ x.signbit;

		set_mantissa(sign_1*man_1 + sign_2*man_2);
		if((get_mantissa() == 0) && (sign_dif == 0)) signbit = x.signbit;
		CLEAR_SIGNBIT(umantissa,signbit)
		return *this;
	}

	signed_magnitude operator -=(const signed_magnitude& x) {
		uint64_t man_1 = umantissa; 
		uint64_t man_2 = x.get_unsigned_mantissa();

		int sign_1 = (signbit==0) ? 1 : -1;
		int sign_2 = (x.signbit==0) ? 1 : -1;
		int sign_dif = signbit ^ x.signbit;

		set_mantissa(sign_1*man_1 - sign_2*man_2);
		CLEAR_SIGNBIT(umantissa,signbit)
		return *this;
	}

	// Left Shifting
	signed_magnitude operator <<=(int x) {
		umantissa <<= x;
		return *this;
	}
	// Right shifting
	signed_magnitude operator >>=(int x) {
		umantissa >>= x;
		return *this;
	}

	// Left Shifting
	template <typename T> signed_magnitude operator <<(const T x) const {
		signed_magnitude tmp(*this);
		tmp.Bits += (int)x;
		tmp <<= (int)x;
		return tmp;
	}

	// Right shifting
	template <typename T> signed_magnitude operator >>(const T x) const {
		signed_magnitude tmp(*this);
		tmp.Bits -= (int)x;
		tmp >>= (int)x;
		return tmp;
	}

	signed_magnitude abs() {
		signed_magnitude tmp(*this);
		tmp.signbit = 0;
		return tmp;
	}
	bool operator ==(const signed_magnitude& x) const { return ((umantissa == x.umantissa)&&(signbit == x.signbit)); }
	bool operator !=(const signed_magnitude& x) const { return ((umantissa != x.umantissa)||(signbit != x.signbit)); }

	friend std::ostream& 
		operator <<(std::ostream& os, signed_magnitude &r) {
		return os <<  r.signbit << " " << r.umantissa;
	}
	friend std::istream& 
		operator >>(std::istream& os, signed_magnitude &r) {
		int64_t x;
		return os >> x; //?
	}

}; // END OF CLASS SIGNED_MAGNITUDE

#include <dsp_lib/complex.h>

inline complex<signed_magnitude> operator <<(complex<signed_magnitude> &r, const int shift) 
{
	r.i <<= shift;
	r.q <<= shift;
	complex<signed_magnitude> res = complex<signed_magnitude>(r.i, r.q);
	return(res);
}
//! Right shift
inline complex<signed_magnitude>  operator >>(complex<signed_magnitude> &r, const int shift) {
	r.i >>= shift;
	r.q >>= shift;
	complex<signed_magnitude> res = complex<signed_magnitude>(r.i, r.q);
	return(res);
}


#endif /*SIGNED_MAGNITUDE_H_*/
