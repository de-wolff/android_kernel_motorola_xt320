#ifndef LOOPFILTER_H
#define LOOPFILTER_H
// 2nd order Loop filter as template class
// must have function "ROUND" defined for whatever data type used
template <class Numeric> class loop_filter
{
 public:
	//! enable multipliers
	int en_mult;
	//! enable accumulators
	int en_acc;
	//! First order gain
	Numeric alpha; 
	//! second order gain
	Numeric beta; 
	//! Accumulator for beta branch (should not be written to)
	Numeric beta_acc;
	
 protected:
	Numeric loop_out;
	Numeric beta_prod, alpha_prod;
	
	const int ROUND_BITS;

 public:
	//! Constructor - initialize with number of bits to round in multipliers
	loop_filter(int R) : ROUND_BITS(R) {
		alpha = beta = beta_acc = 0; en_mult = en_acc = 0;
	}
	//! Reset
	void reset(void) { beta_acc = beta_prod = alpha_prod = loop_out = 0; }
	//! Normal call with input, returns output.
	Numeric update(Numeric error) {
		if (en_mult) {
			alpha_prod = ROUND((error*alpha),ROUND_BITS);
			beta_prod  = ROUND((error*beta),ROUND_BITS);
		} else {
			alpha_prod  = beta_prod =  (Numeric)0;
		}
		loop_out   = beta_acc + alpha_prod; // Use last beta_acc!
		if (en_acc) beta_acc  += beta_prod; 
		return(loop_out);
	}
};
#endif
