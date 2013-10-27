#ifndef DLYL_H
#define DLYL_H
//! Template class for a Delay line (primitive used in other classes)
//!  Allows user to check at various points in delay line,
//!  but default use is a pure delay.
template <class Numeric> class delay 
{
 public: 
  long num_taps;
 protected:
  Numeric* z; 
      
 public: 
  //! Constructor
  delay(long n=0) : num_taps(n+1) {
	int i=0;
	z = new Numeric[num_taps];
	for (i=0;i<num_taps;i++) z[i] = 0;
  }
  //! Assignment
  delay& operator=(const delay &rhs) {
	num_taps = rhs.num_taps;
	for (int i=0;i<num_taps;i++) z[i] = rhs.z[i];
	return(*this);
  }
  //! Destructor
  ~delay(void) {	if (num_taps>0) delete [] z;}
  void reset(void) { for (int i=0;i<num_taps;i++) z[i] = 0; }
  //! Get delay at tap i
  Numeric check(long i) { return(z[i]); }
  //! Look back in delay line by i samples
  Numeric checkback(long i) { return(z[num_taps-1-i]); }
  //! Get last tap
  Numeric last() { return(z[num_taps-1]);}
  //! Set size of delay
  void set_size(long n=2) {
	int i=0;
	if (num_taps>0) delete [] z;
	num_taps = n+1;
	z = new Numeric[num_taps];
	for (i=0;i<num_taps;i++) z[i] = 0;
  }  
  //! Clock in new input sample  
  Numeric input(Numeric in) {
	int i;                                           
	// Update history of inputs
	for (i=num_taps-1;i>0;i--) z[i] = z[i-1];  
	// Add new input
	z[0] = in;   
	return(z[num_taps-1]);
  }
  //! Clock in new sample and get output from delay line
  inline Numeric update(Numeric in) {
	input(in);
	return(z[num_taps-1]);
  }	
};
#endif
