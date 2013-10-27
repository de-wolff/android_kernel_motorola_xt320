#ifndef DSP_FLOAT_H_
#define DSP_FLOAT_H_

#include <iostream>

#ifndef DMAX
#define DMAX(a, b) ((a) > (b)) ?  (a) : (b)
#endif

#include <dsp_lib/signed_mantissa.h>
#include <dsp_lib/fifo.h>


// Undefine this if separate mantissa & exponent fifos not needed
// and you want to make debugging output much more concise when looking at an float_s variable
//#define GSEP_FIFO

/*
 * class float_s - pseudo-floating
 * Template Arguments
 * - NMAN - number of bits allocated to the mantissa
 * Assumption:
 * 1. NMAN >= 2
 * 2. any operator between two dsp_float will have the same template parameters
 * - NEXP - number of bits allocated to the exponent
 */
template <typename MANT_TYPE, int32_t NMAN, int32_t NEXP> class dsp_float {

public:
	MANT_TYPE sm_mantissa;
	int32_t exponent;

	dsp_float() {
		set_mantissa(0);
		set_mantissa_bits(NMAN);
		set_min_exponent();
	}
	dsp_float(float a) { to_dsp_float((double)a); }
	dsp_float(double a) { to_dsp_float(a); }
	dsp_float(int a) { to_dsp_float((double)a); }
    dsp_float(long a) { to_dsp_float((double)a); }

	int log2int(double x) {
		double abs_x = fabs(x);
		return (int) floor((log(abs_x)/log((double) 2)));
	}

	void to_dsp_float(double x) {
		double man_dbl;
		set_mantissa_bits(NMAN);
		// Account for special case (log2 of 0 is inf)
		if(x == 0){
			set_min_exponent();
			set_mantissa(0);
		} else {
			set_exponent(log2int(x) - NMAN + 2);
			if(x>0) {
				man_dbl = x*pow(2.0,-((double)exponent)) + 0.5;
			} else {
				man_dbl = x*pow(2.0,-((double)exponent)) - 0.5;
			}
			set_mantissa(((int64_t)man_dbl));
		}
	}

	void setVal(double a) { to_dsp_float(a); }
	void setVal(float a)  { to_dsp_float((double)a); }
    void setVal(long a)   {	to_dsp_float((double)a); 	}
	void setVal(int a)    { to_dsp_float((double)a); 	}

	/* Used for debugging */
	double to_double() const {
		double val_dbl = double(get_mantissa())*pow(2,double(exponent));
		return(val_dbl);
	}

	double to_unsigned_double() const {
		double val_dbl = double(get_unsigned_mantissa())*pow(2,double(exponent));
		return(val_dbl);
	}

	/* Get/Set Functions for Member Variables */
	int get_mantissa_bits() const { return(NMAN); }

	int64_t get_unsigned_mantissa() const {return(sm_mantissa.get_unsigned_mantissa());}
	int64_t get_mantissa() const {return(sm_mantissa.get_mantissa());	}
	int32_t get_signbit() const {	return(sm_mantissa.get_signbit());	}
	int64_t get_mantissa_sm() const { return(sm_mantissa.get_mantissa_sm());	}
	int32_t get_exponent() const {return exponent;}

	void set_max_mantissa() {sm_mantissa.set_max_mantissa(); }
	void set_mantissa_bits(int x) {sm_mantissa.Bits = x; }
	void set_mantissa(int64_t x) {sm_mantissa.set_mantissa(x); }
	void force_unsigned_mantissa(uint64_t x) {sm_mantissa.force_unsigned_mantissa(x);}
	void set_signbit(int x) {	sm_mantissa.set_signbit(x);	}
	void set_exponent(int32_t x) {exponent = x;	}
	void set_min_exponent() {	exponent = -1 << (NEXP-1);}
	void set_max_exponent() {	exponent = (1 << (NEXP-1)) - 1;}

    void init_max() {
    	set_max_mantissa();
    	set_max_exponent();
    	set_signbit(0);
    }

	// Dummy align() function for complex block floating point.
	void align() {	;	}

	// EXPONENT ALIGNMENT: find max exponent
	int32_t findMaxExp(int32_t comExp) {
		if (get_exponent() > comExp) comExp = get_exponent();
		return comExp;
	}

	// EXPONENT ALIGNMENT: find min exponent
	int32_t findMinExp(int32_t comExp) {
		if (get_exponent() < comExp) comExp = get_exponent();
		return comExp;
	}

	// EXPONENT ALIGNMENT: Set common exponent
	int32_t setComExp(int32_t comExp) {
		if (get_exponent() < comExp) {
			sm_mantissa = sm_mantissa >> (comExp - get_exponent());
		} else {
			sm_mantissa = sm_mantissa << (get_exponent() - comExp);
		}
		set_exponent(comExp);
	}

	// Only operator & fifo stuff below here


	// Multiplication(multiplication) for BLOCK floating point numbers of the same or different precisions
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> dsp_float<MANT_TYPE1,NMAN+NMAN1-1,DMAX(NEXP,NEXP1)+1> 
		operator *(const dsp_float<MANT_TYPE1,NMAN1,NEXP1>& x) const {
		
		dsp_float<MANT_TYPE1, NMAN+NMAN1-1,DMAX(NEXP,NEXP1)+1> tmp;
		tmp.exponent = exponent + x.exponent;
		tmp.sm_mantissa = sm_mantissa * x.sm_mantissa;
		return tmp;
	}

	// Left Shifting
	dsp_float<MANT_TYPE,NMAN, NEXP> operator <<(const int x) {
		dsp_float<MANT_TYPE, NMAN,NEXP> tmp;
		tmp.exponent = get_exponent() + x;
		tmp.sm_mantissa  = sm_mantissa;
		return tmp;
	};

	dsp_float<MANT_TYPE,NMAN, NEXP> operator <<=(const int x) {
		exponent += x;
		return *this;
	};

	// Right shifting
	dsp_float<MANT_TYPE,NMAN, NEXP> operator >>(const int x) {
		dsp_float<MANT_TYPE,NMAN, NEXP> tmp;
		tmp.exponent = get_exponent() - x;
		tmp.sm_mantissa  = sm_mantissa;
		return tmp;
	};

	dsp_float<MANT_TYPE,NMAN, NEXP> operator >>=(const int x) {
		exponent -= x;
		return *this;
	};

    // various assignments from here below.
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> dsp_float<MANT_TYPE1,NMAN,NEXP>& 
		operator =(dsp_float <MANT_TYPE1,NMAN1,NEXP1> &x) {
		if (NMAN > NMAN1) {
			sm_mantissa = x.sm_mantissa << (NMAN - NMAN1);
		} else {
			sm_mantissa = x.sm_mantissa >> (NMAN1 - NMAN);
		}
		exponent = x.exponent + NMAN1 - NMAN;
		set_signbit(x.get_signbit());
		set_mantissa_bits(NMAN);
		put_all_fifo();
		return *this;
	}

	dsp_float<MANT_TYPE,NMAN,NEXP>& operator =(const dsp_float<MANT_TYPE,NMAN,NEXP> &x) {
		set_mantissa(x.get_mantissa());
		exponent = x.exponent;
		set_signbit(x.get_signbit());
		set_mantissa_bits(NMAN);
		put_all_fifo();
		return *this;
	}

	dsp_float& operator = (const double x) {
		to_dsp_float(x);
		set_mantissa_bits(NMAN);
		put_all_fifo();
		return *this;
	}

	dsp_float operator -() const {
		dsp_float tmp(*this);
		tmp.set_signbit(get_signbit() ^ 1);
		return(tmp);
	}
	// Block floating point addition and subtraction
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> 
		dsp_float<MANT_TYPE1, DMAX(NMAN, NMAN1) + 1, DMAX(NEXP, NEXP1)+1> 
		operator +(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		
		dsp_float<MANT_TYPE1, DMAX(NMAN, NMAN1) + 1, DMAX(NEXP, NEXP1)+1> tmp;

		// If inputs aren't pre-aligned properly, the following routine aligns the operands
		int64_t exp_1 = exponent;
		int64_t exp_2 = x.exponent;
		int32_t man_shift;
		MANT_TYPE sman_1 = sm_mantissa;
		MANT_TYPE sman_2 = x.sm_mantissa;

		if(x.exponent > exponent) {
			sman_1 = sman_1  >> (x.exponent - exponent);
			exp_1 = exp_2;
			man_shift = (DMAX(NMAN, NMAN1) + 1) - (NMAN1 + 1);
		} else {
			if(exponent > x.exponent) {
				sman_2 = sman_2 >> (exponent - x.exponent);
				exp_2 = exp_1;
				man_shift = (DMAX(NMAN, NMAN1) + 1) - (NMAN + 1);
			} else {
				man_shift = 0;
			}
		}
		tmp.sm_mantissa = (sman_1 + sman_2) << (man_shift);
		tmp.exponent = exp_2 - man_shift;
		return tmp;
	}
	// Block floating point addition and subtraction
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> 
		dsp_float<MANT_TYPE1, DMAX(NMAN, NMAN1) + 1, DMAX(NEXP, NEXP1)+1> 
		operator -(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		
		dsp_float<MANT_TYPE1, DMAX(NMAN, NMAN1) + 1, DMAX(NEXP, NEXP1)+1> tmp;

		// If inputs aren't pre-aligned properly, the following routine aligns the operands
		int64_t exp_1 = exponent;
		int64_t exp_2 = x.exponent;
		int32_t man_shift;
		MANT_TYPE sman_1 = sm_mantissa;
		MANT_TYPE sman_2 = x.sm_mantissa;

		if(x.exponent > exponent) {
			sman_1 = sman_1  >> (x.exponent - exponent);
			exp_1 = exp_2;
			man_shift = (DMAX(NMAN, NMAN1) + 1) - (NMAN1 + 1);
		} else {
			if(exponent > x.exponent) {
				sman_2 = sman_2 >> (exponent - x.exponent);
				exp_2 = exp_1;
				man_shift = (DMAX(NMAN, NMAN1) + 1) - (NMAN + 1);
			} else {
				man_shift = 0;
			}
		}
		tmp.sm_mantissa = (sman_1 - sman_2) << (man_shift);
		tmp.exponent = exp_2 - man_shift;
		return tmp;
	}


	// various accumulators, i.e., += or -= from here below.
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> 
		dsp_float<MANT_TYPE1,NMAN,NEXP> & operator +=(const dsp_float<MANT_TYPE1,NMAN1,NEXP1>& x) {
		int64_t exp_1 = exponent;
		int64_t exp_2 = x.exponent;
		MANT_TYPE sman_1 = sm_mantissa; 
		MANT_TYPE sman_2 = x.sm_mantissa;
		if(exp_2 > exponent) {
			sman_1 = sman_1 >> (exp_2 - exponent);
			exp_1 = exp_2;
		} else {
			if(exponent > exp_2) {
				sman_2 = sman_2 >> (exponent - exp_2);
				exp_2 = exponent;
			} else {
			}
		}
		sm_mantissa = (sman_1 + sman_2);
		if ( (x.exponent - exponent) > NMAN ) {
			set_signbit(x.get_signbit());
		}
		if ((exponent - x.exponent) >= NMAN ) {
			sm_mantissa = sman_1;
		}
		exponent=exp_2;
		put_all_fifo();
		return *this;
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> dsp_float<MANT_TYPE1,NMAN,NEXP> & 
		operator -=(const dsp_float<MANT_TYPE1,NMAN1,NEXP1>& x) {
		int64_t exp_1 = exponent;
		int64_t exp_2 = x.exponent;
		MANT_TYPE sman_1 = sm_mantissa;  
		MANT_TYPE sman_2 = x.sm_mantissa;
		if(exp_2 > exponent) {
			sman_1 = sman_1 >> (exp_2 - exponent);
			exp_1 = exp_2;
		} else {
			if(exponent > exp_2) {
				sman_2 = sman_2 >> (exponent - exp_2);
				exp_2 = exponent;
			} else {
			}
		}
		sm_mantissa = (sman_1 - sman_2);
		exponent = exp_2;
		put_all_fifo();
		return *this;
	}

	// Casting operator
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> 
		operator dsp_float<MANT_TYPE1,NMAN1,NEXP1>() const {
		dsp_float<MANT_TYPE1,NMAN1,NEXP1> tmp;
		int32_t manwidth1 = NMAN; int32_t manwidth2 = NMAN1;
		if (NMAN > NMAN1) {
			tmp.sm_mantissa = sm_mantissa >> (NMAN-NMAN1);
			tmp.exponent = exponent + (NMAN-NMAN1);
		} else {
			tmp.sm_mantissa = sm_mantissa << (NMAN1-NMAN);
			tmp.exponent = exponent - (NMAN1-NMAN);
		}
		return tmp;
	}

	// Boolean operators for floating type numbers with same precision
	bool operator ==(const dsp_float& x) const { return ((get_mantissa() == x.get_mantissa())&&(exponent == x.get_exponent())&&(get_signbit() == x.get_signbit())); }
	bool operator !=(const dsp_float& x) const { return ((get_mantissa() != x.get_mantissa())||(exponent != x.get_exponent())||(get_signbit() != x.get_signbit())); }
	bool operator  <(const dsp_float& x) const { return ((to_double() < x.to_double())); }
	bool operator  >(const dsp_float& x) const { return ((to_double() > x.to_double())); }
	bool operator <=(const dsp_float& x) const { return ((to_double() <= x.to_double())); }
	bool operator >=(const dsp_float& x) const { return ((to_double() >= x.to_double())); }

	// Boolean operators for floating type numbers with different precisions
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator ==(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() == x.to_double()));
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator !=(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() != x.to_double()));
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator <(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() < x.to_double()));
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator <=(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() <= x.to_double()));
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator >(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() > x.to_double()));
	}
	template <typename MANT_TYPE1, int32_t NMAN1, int32_t NEXP1> bool operator >=(const dsp_float<MANT_TYPE1,NMAN1, NEXP1>& x) const {
		return ((to_double() >= x.to_double()));
	}

	void put_all_fifo() {
#ifdef COSIM_ASSIGNMENT_PUT
		put_fifo_data();
		// these do nothing unless SEP_FIFO is defined
		put_fifo_mantissa();
		put_fifo_exponent();
#endif
	}


	// COSIM & fifo & I/O stuff below til' end of class def
#ifdef COSIM
	fifo<long>	fifo_data;
#ifdef SEP_FIFO
	fifo<int>	fifo_mantissa;
	fifo<int>	fifo_exponent;
#endif

	void init_fifo_all(int len) {
		// setting fifo memory depth.
		fifo_data.set_size(len);
		// these do nothing unless SEP_FIFO is defined
		fifo_mantissa.set_size(len);
		fifo_exponent.set_size(len);
	}
	void init_fifo(int len) {fifo_data.set_size(len);}
	void init_fifo_data(int len) {fifo_data.set_size(len);}
	long get_fifo_data() { return(fifo_data.get_data());	}
	void put_fifo_data() {
		int ebits = exponent & ( (1<<NEXP) - 1);
		set_mantissa_bits(NMAN);
		fifo_data.put((get_mantissa_sm() << NEXP) + ebits);
	}
#ifdef SEP_FIFO
	int get_fifo_mantissa() {		return(fifo_mantissa.get_data());	}
	int get_fifo_exponent() {		return(fifo_exponent.get_data());	}
#endif

	void init_fifo_mantissa(int len) {
#ifdef SEP_FIFO
		fifo_mantissa.set_size(len);
#endif
	}
	void init_fifo_exponent(int len) {
#ifdef SEP_FIFO
		fifo_exponent.set_size(len);
#endif
	}
	void put_fifo_mantissa() {	
#ifdef SEP_FIFO
		fifo_mantissa.put(get_mantissa_sm());	
#endif
	}
	void put_fifo_exponent() {
#ifdef SEP_FIFO
		int ebits = exponent & ( (1<<NEXP) - 1);
		fifo_exponent.put(ebits);
#endif
	}
#endif

	// IO functions
	friend std::ostream& 
		operator <<(std::ostream& os, dsp_float<MANT_TYPE,NMAN,NEXP> &r) {
		return os << r.to_double();
	}
	friend std::istream& 
		operator >>(std::istream& os, dsp_float<MANT_TYPE,NMAN,NEXP> &r) {
		return os >> r; // ???
	}

}; // END OF CLASS DSP_FLOAT
#endif /*DSP_FLOAT_H_*/
