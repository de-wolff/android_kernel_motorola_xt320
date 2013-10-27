#ifndef SPUC_COMPLEX_GEN
#define SPUC_COMPLEX_GEN
/* Copyright (c) 1993-2005 Tony Kirke
  If the software is to be used in a commercial product,
  or incorporated into anything that is to be sold or given to a third party,
  then a commercial license must be purchased from Tony Kirke. Otherwise
  Broadcom Corporation is entitled to use this software for internal use,
  provided that this copyright notice is retained in the code. Tony Kirke has
  and will retain all ownership rights in the software and its documentation
  and all related, copyrights, trademarks, service marks, and other proprietary
  rights. No representations are made about the suitability of this for any purpose.
  It is provided "as is" without expressed or implied warranty. Under no circumstances
  including negligence, shall Tony Kirke be liable to Broadcom for any incidental,
  indirect special or consequential damages arising out of the use, misuse or inability
  to use the software or related documentation.
*/
#include <iostream>

#ifndef _DSP_TYPES_H_
struct cint16;
struct cint32;
#endif

//#include "dsp/types.h"


//#ifdef AML_MOD
//#include "../../dsp/mimo/src/demod_aml/aml_com.h"
//#endif
/*
#include "../../dsp/mimo/src/demod_aml/aml_dbl.h"
#include "../../dsp/mimo/src/demod_aml/aml_globals.h"
#include "../../dsp/mimo/src/demod_aml/aml_acc.h"
*/
#ifndef PI
#define PI	3.141592653589793238462643
#endif
#ifndef TWOPI
#define TWOPI	6.28318530717958647692
#endif
#define HALFPI	1.57079632679489661923
#define QUARTPI 0.78539816339744830962

// macros
/*
#ifdef AML_MOD
#ifndef MAX
#define MAX(x,y)	((x) >= (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y)	((x) <= (y) ? (x) : (y))
#endif
#ifndef ABS
#define ABS(x) ( (x < 0) ? (-x) : (x))
#endif
#endif
*/

#include <cmath>
#ifdef COUNTOPS
#define ASP_C(x) extern long x; x++;
#else
#define ASP_C(x)
#endif
namespace SPUC {
//! \brief Spuc template complex class.
//!	Basically the same as STL complex class but allows easier customization.
template <typename T> class complex {
public:
	T i;
	T q;

	complex() {
		i = 0;
		q = 0;
	}

	complex(T r, T i=0) :i(r), q(i) {}

	template <typename T1> complex(const complex<T1>& r) {
		i = r.i;
		q = r.q;
	}
	template <typename T1> complex(const T1& y, const T1& z) {
		i = y;
		q = z;
	}

	inline T &real() {
		return(i);
	}
	inline T &imag() {
		return(q);
	}

	complex<T> operator -() {
		complex<T> x;
		x.i = -i;
		x.q = -q;
		return x;
	}

	inline complex& operator=(const T &r) {
		i = r;
		q = 0;
		return *this;
	}

	template <typename T1> complex<T>& operator= (const T1 &y) {
		i = y;
		q = 0;
		return *this;
	}
	//  template <> complex(const complex<long>& y) : re(y.re), im(y.q) {;}
	template <typename T1> complex<T>& operator= (const complex<T1> &y) {
		i = static_cast<T>(y.i);
		q = static_cast<T>(y.q);
		return(*this);
	}
/*
	inline complex<T>  operator =(const complex<T>& y)   {
		i = y.i;
		q = y.q;
		return *this;
	}
*/
#ifdef AML_COM_H_
	inline complex<T>&  operator =(const aml_com_cx& y)   {
		i = y.i;
		q = y.q;
		return *this;
	}
#endif

	void setVal(float a) {
		i.setVal(a);
		q.setVal(a);
	}
	void setVal(double a) {
		i.setVal(a);
		q.setVal(a);
	}
	void setVal(int a) {
		i.setVal(a);
		q.setVal(a);
	}
	void setVal(long a) {
		i.setVal(a);
		q.setVal(a);
	}
#ifdef _DSP_TYPES_H_
	void setCint16(cint16 v, int nint, int nfrc) {
		i.setInt32((long)v.i,nint,nfrc);
		q.setInt32((long)v.q,nint,nfrc);
	}
	void setCint32(cint32 v, int nint, int nfrc) {
		i.setInt32(v.i,nint,nfrc);
		q.setInt32(v.q,nint,nfrc);
	}
	cint16 getCint16(int nint, int nfrc) {
		cint16 tmp;
		tmp.i=i.getInt16(nint,nfrc);
		tmp.q=q.getInt16(nint,nfrc);
		return(tmp);
	}
	cint32 getCint32(int nint, int nfrc) {
		cint32 tmp;
		tmp.i=i.getInt32(nint,nfrc);
		tmp.q=q.getInt32(nint,nfrc);
		return(tmp);
	}
#endif

	inline complex  operator *=(const complex<T>& y) {
		ASP_C(cmult_count)
		T r = i*y.i - q*y.q;
		q  = i*y.q + q*y.i;
		i = r;
		return *this;
	}
	inline complex  operator +=(const complex<T>& y) {
		ASP_C(cadd_count)
		i += y.i;
		q += y.q;
		return *this;
	}
	inline complex  operator -=(const complex<T>& y) {
		ASP_C(cadd_count)
		i -= y.i;
		q -= y.q;
		return *this;
	}

	inline void align() {
		int comExp=-1000;		// Convention: Common Exp = -1000 if the goal is to find Max exponent
								//			  			  =  1000 if the goal is to find Min exponent
		comExp=i.findMaxExp(comExp);
		comExp=q.findMaxExp(comExp);
		i.setComExp(comExp);
		q.setComExp(comExp);
		if(i.to_double()==0 && q.to_double()==0) {
			i.setVal(0);
			q.setVal(0);
		}
	}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	operator const complex<long> ()   const {
		return(complex<long>((long)i,(long)q));
	}
	operator const complex<double> () const {
		return(complex<double>((double)i,(double)q));
	}
#endif
};

// template_instantiations: long, double
// IO functions
template <class T> std::ostream& operator <<(std::ostream& os, complex<T> &r) {
	return os << "{" << r.i << "," << r.q << "}";
};
template <class T> std::istream& operator >>(std::istream& os, complex<T> &r) {
	return os >> r.i >> r.q;
};

#ifdef AML_FXP_SS
// MODIFIED FOR AML TO RETURN AML_FXP_SS by default except aml_dbl
template <typename T1, typename T2> inline complex<AML_FXP_SS> operator *(const complex<T1> &r, const complex<T2> &l) {
	ASP_C(cmult_count)
	ASP_C(cadd_count)
	complex<AML_FXP_SS> x;
	x.i = ((r.i*l.i) - (r.q*l.q));
	x.q = (r.i*l.q + r.q*l.i);
	return x;
}
#else
template <typename T1, typename T2> inline complex<T1> operator *(const complex<T1> &r, const complex<T2> &l) {
	ASP_C(cmult_count)
	ASP_C(cadd_count)
	complex<T1> x;
	x.i = ((r.i*l.i) - (r.q*l.q));
	x.q = (r.i*l.q + r.q*l.i);
	return x;
}
#endif

/*
#ifdef AML_MOD
inline complex<aml_dbl> operator *(complex<aml_dbl> r, complex<aml_dbl> l) {
	ASP_C(cmult_count)
	ASP_C(cadd_count)
	complex<aml_dbl> x;
	x.i = ((r.i*l.i) - (r.q*l.q));
	x.q = (r.i*l.q + r.q*l.i);
	return x;
}

//!
template <typename T> inline complex<AML_FXP_SS> operator *(complex<T> r, T l) {
	ASP_C(hcmult_count)
	complex<AML_FXP_SS> x;
	x.i = r.i*l;
	x.q = r.q*l;
	return x;
}
template <typename T> inline complex<AML_FXP_SS> operator *(T l, complex<T> r) {
	ASP_C(hcmult_count)
	complex<AML_FXP_SS> x;
	x.i = r.i*l;
	x.q = r.q*l;
	return x;
}
inline complex<aml_dbl> operator *(complex<aml_dbl> r, aml_dbl l) {
	ASP_C(hcmult_count)
	complex<aml_dbl> x;
	x.i = r.i*l;
	x.q = r.q*l;
	return x;
}
inline complex<aml_dbl> operator *(aml_dbl l, complex<aml_dbl> r) {
	ASP_C(hcmult_count)
	complex<aml_dbl> x;
	x.i = r.i*l;
	x.q = r.q*l;
	return x;
}

template <typename T> inline complex<AML_FXP_SS> operator /(complex<T> l, T r) {
	ASP_C(hdiv_count)
	complex<AML_FXP_SS> x(0,0);
	if (r != T(0)) {
		x.i = l.i/r;
		x.q = l.q/r;
	}
	//! else is an error condition!
	return x;
}

inline complex<aml_dbl> operator /(complex<aml_dbl> l, aml_dbl r) {
	ASP_C(hdiv_count)
	complex<aml_dbl> x(0,0);
	if (r != aml_dbl(0)) {
		x.i = l.i/r;
		x.q = l.q/r;
	}
	//! else is an error condition!
	return x;
}
//!

//! Magnitude Squared of complex vector
template <typename T> inline AML_FXP_SS norm(complex<T> x) {
	ASP_C(hcmult_count)
	ASP_C(hcadd_count)
	AML_FXP_SS y;
	y = (x.i*x.i + x.q*x.q);
	return y;
}

//! Magnitude Squared of complex vector
inline aml_dbl norm(complex<aml_dbl> x) {
	ASP_C(hcmult_count)
	ASP_C(hcadd_count)
	aml_dbl y;
	y = (x.i*x.i + x.q*x.q);
	return y;
}
// END OF MODIFIED
#endif
*/

//!
template <typename T1, typename T2> inline complex<T1> operator +(const complex<T1> &r, const complex<T2> &l) {
	ASP_C(cadd_count)
	complex<T1> x;
	x.i = r.i + l.i;
	x.q = r.q + l.q;
	return x;
}
//!
template <typename T> inline complex<T> operator +(const complex<T> &r, const T &l) {
	complex<T> x;
	x.i = r.i + l;
	x.q = r.q;
	return x;
}
//!
template <typename T> inline complex<T> operator +(const T &r, const complex<T> &l) {
	complex<T> x;
	x.i = r + l.i;
	x.q = l.q;
	return x;
}

//!
template <typename T1, typename T2> inline complex<T1> operator -(const complex<T1> &r, const complex<T2> &l) {
	ASP_C(cadd_count)
	complex<T1> x;
	x.i = r.i - l.i;
	x.q = r.q - l.q;
	return x;
}
//!
template <typename T> inline complex<T> operator -(const complex<T> &r, const T &l) {
	complex<T> x;
	x.i = r.i - l;
	x.q = r.q;
	return x;
}
//!
template <typename T> inline complex<T> operator -(const T &r, const complex<T> &l) {
	complex<T> x;
	x.i = r - l.i;
	x.q = -l.q;
	return x;
}
//!
template <typename T> inline complex<T> operator &(complex<T> &r, T &l) {
	complex<T> x;
	x.i = r.i & l;
	x.q = r.q & l;
	return x;
}
//!
template <typename T> inline complex<T> operator &(T &r, complex<T> &l) {
	complex<T> x;
	x.i = r & l.i;
	x.q = r & l.q;
	return x;
}
template <typename T> inline complex<T> operator %(complex<T> &r, T &l) {
	complex<T> x;
	x.i = r.i % l;
	x.q = r.q % l;
	return x;
}
//!
template <typename T> inline complex<T> operator ^(complex<T> &r, T &l) {
	complex<T> x;
	x.i = r.i ^ l;
	x.q = r.q ^ l;
	return x;
}
//!
template <typename T> inline complex<T> operator ^(T &r, complex<T> &l) {
	complex<T> x;
	x.i = r ^ l.i;
	x.q = r ^ l.q;
	return x;
}
template <typename T> inline complex<T> operator |(complex<T> &r, T &l) {
	complex<T> x;
	x.i = r.i | l;
	x.q = r.q | l;
	return x;
}
//!
template <typename T> inline complex<T> operator |(T &r, complex<T> &l) {
	complex<T> x;
	x.i = r | l.i;
	x.q = r | l.q;
	return x;
}
//! Left shift
template <typename T> inline complex<T>  operator <<(complex<T> &r, const long shift) {
	complex<long> res = complex<long>(r.i << shift, r.q << shift);
	return(res);
}
//! Right shift
template <typename T> inline complex<T>  operator >>(complex<T> &r, const long shift) {
	complex<long> res = complex<long>(r.i >> shift, r.q >> shift);
	return(res);
}

/* This should be inside the class. Akira
//!
template <typename T> inline complex<T> operator -(complex<T> &r) {
	complex<T> x;
	x.i = -r.i;
	x.q = -r.q;
	return x;
}
*/

///!
template <typename T> complex<T> operator /=(complex<T> &l, T &r) {
	if (r != 0) {
		l.i = l.i/r;
		l.q = l.q/r;
	}
	//! else is an error condition!
	return l;
}

template <typename T1, typename T2> complex<T1> operator /(complex<T1> &r, complex<T2> &l) {
	ASP_C(div_count)
	complex<T1> x;
	T2 den;
	den = magsq(l);
	x = (r * conj(l))/den;
	return x;
}

template <typename T1, typename T2> complex<T1> operator /=(complex<T1> &r, complex<T2> &l) {
	T2 den;
	den = magsq(l);
	r = (r * conj(l))/den;
	return r;
}

//!
template <typename T1, typename T2> inline bool operator ==(complex<T1> &r, complex<T2> &l) {
	return ((r.i == l.i) && (r.q == l.q));
}

///!
template <typename T1, typename T2> inline bool operator <=(complex<T1> &r, complex<T2> &l) {
	return ((r.i <= l.i) && (r.q <= l.q));
}
///!
template <typename T1, typename T2> inline bool operator <(complex<T1> &r, complex<T2> &l) {
	return ((r.i < l.i) && (r.q < l.q));
}
///!
template <typename T1, typename T2> inline bool operator >=(complex<T1> &r, complex<T2> &l) {
	return ((r.i >= l.i) && (r.q >= l.q));
}
///!
template <typename T1, typename T2> inline bool operator >(complex<T1> &r, complex<T2> &l) {
	return ((r.i > l.i) && (r.q > l.q));
}

//!
template <typename T1, typename T2> inline bool operator !=(complex<T1> &r, complex<T2> &l) {
	return ((r.i != l.i) || (r.q != l.q));
}

//! Complex value (0,1)
template <typename T> inline complex<T> complexj(void) {
	return(complex<T>(0,1));
}

template <typename T> inline T real(const complex<T> &y) {
	T x;
	x = y.i;
	return(x);
}

template <typename T> inline T imag(const complex<T> &y) {
	T x;
	x = y.q;
	return(x);
}

template <typename T> inline T re(complex<T> &x) {
	T y;
	y = x.i;
	return y;
}

template <typename T> inline T im(complex<T> &x) {
	T y;
	y = x.q;
	return y;
}

//! Conjugate
template <typename T> inline complex<T> conj(const complex<T> &x) {
	complex<T> y;
	y.i = x.i;
	y.q = -x.q;
	return y;
}

//! Normalized vector (magnitude = 1)
template <typename T> inline complex<double> normalized(complex<T> &x) {
	T y;
	y = ::sqrt(x.i*x.i + x.q*x.q);
	return (complex<double>(x.i/y,x.q/y));
}
template <typename T> inline T magsq(complex<T> &x) {
	T y = (x.i*x.i + x.q*x.q);
	return (y);
}

template <typename T> inline complex<T> real_mult(complex<T> &r, complex<T> &l) {
	ASP_C(hcmult_count)
	ASP_C(hcadd_count)
	complex<T> z;
	z.i = ((r.i*l.i) - (r.q*l.q));
	z.q = 0;
	return (z);
}
template <typename T> inline complex<T> reals_only_mult(complex<T> &r, complex<T> &l) {
	ASP_C(rmult_count)
	complex<T> z;
	z.i = (r.i*l.i);
	z.q = 0;
	return (z);
}


template <typename T> inline T approx_mag(complex<T> &x) {
	T xa = ABS(x.i);
	T ya = ABS(x.q);
	return(MAX(xa,ya) + MIN(xa,ya)/4);
}

template <typename T> inline complex<T> minimum(complex<T> &x1, complex<T> &x2) {
	return(complex<T>(MIN(x1.i,x2.i),MIN(x1.q,x2.q)));
}
template <typename T> inline complex<T> maximum(complex<T> &x1, complex<T> &x2) {
	return(complex<T>(MAX(x1.i,x2.i),MAX(x1.q,x2.q)));
}
//! Return phase angle (radians) of complex number
template <typename T> inline double arg(const complex<T> &x) {
	double TMPANG;
	if (real(x) == 0) {
		if (imag(x) < 0) return(3.0*PI/2.0);
		else return(PI/2.0);
	} else {
		TMPANG=atan((double)imag(x)/(double)real(x));
		if (real(x) < 0) TMPANG -= PI;
		if (TMPANG < 0) TMPANG += TWOPI;
	}
	return(TMPANG);
}
//! Convert to complex<double>
template <typename T> inline complex<double> rational(complex<T> &l) {
	return(complex<double>((double)l.i,(double)l.q));
}

template <typename T> complex<T> signbit(complex<T> &in) {
	return(complex<T>(SGN(in.i),SGN(in.q)));
}

template <typename T> inline complex<double> reciprocal(complex<T> &x) {
	T y;
	y = (x.i*x.i + x.q*x.q);
	return (complex<double>(x.i/y,-x.q/y));
}

//! squaring function
template <typename T> inline complex<T> sqr(complex<T> &x) {
	return (x*x);
}

//--EXPLICIT SPECIALIZATIONS----------------------------------------------------------------
//! Left shift
template <> inline complex<double>  operator <<(complex<double> &r, const long shift) {
	double scale = (double)(1<<shift);
	complex<double> res(scale*r.i,scale*r.q);
	return(res);
}
//! Right shift
template <> inline complex<double>  operator >>(complex<double> &r, const long shift) {
	double scale = 1.0/(double)(1<<shift);
	complex<double> res(scale*r.i,scale*r.q);
	return(res);
}
} // namespace SPUC
#endif
